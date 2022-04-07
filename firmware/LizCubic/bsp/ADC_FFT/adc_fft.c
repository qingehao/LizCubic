#include "board.h"
#include "arm_math.h"
#include "stdlib.h"
#include "adc_fft.h"
#include "liz_config.h"
#define DBG_TAG    "liz.bsp.adcfft"
#if defined(LIZ_BSP_ADCFFT_DEBUG)
#define DBG_LVL    DBG_LOG
#else 
#define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim2;

#define FFT_LEN 1024
#define THREHSOLD 100

uint16_t adc_buffer[FFT_LEN];
arm_rfft_fast_instance_f32 S;
float   fft_inputbuf[FFT_LEN];   //FFT输入数组
float   fft_outputbuf[FFT_LEN];    //FFT输出数组

static rt_sem_t adc_fft_sem = RT_NULL;
static uint8_t adc_fft_start_flag=0;

static void adc_init();
static void adc_dma_config();
static void adc_tim_config();
static void get_timer_period_prescaler(TIM_HandleTypeDef* htim,uint32_t _ulfreq);
static void fft_init();
static void adc_data_filiter();
static void fft_gen_thread(void *arg);
void adc_xferCplt_callback();

void (*adc_fft_finish_cb)(float *fft_data, int32_t fft_len) = RT_NULL;

void adc_fft_init(void (*cb)(float *fft_data, int32_t fft_len))
{
    adc_fft_finish_cb=cb;
    fft_init();
    adc_init();
    adc_tim_config();
    rt_thread_mdelay(10);
}
static void fft_init()
{
    arm_rfft_fast_init_f32(&S,FFT_LEN);
}

void fft_cal()
{
    arm_rfft_fast_f32(&S, fft_inputbuf, fft_outputbuf, 0);
    arm_cmplx_mag_f32(fft_outputbuf, fft_inputbuf, FFT_LEN);
}

static void adc_init()
{
    ADC_ChannelConfTypeDef sConfig = {0};
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        LOG_E("ADC ERROR");
    }
    adc_dma_config();
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        LOG_E("ADC ERROR1");
    }
    if (HAL_ADC_Start_DMA(&hadc1,
                          (uint32_t *)adc_buffer,
                          FFT_LEN
                         ) != HAL_OK)
    {
        LOG_E("ADC_DMA Start failed.");
    }
}
//初始化DMA
static void adc_dma_config()
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    hdma_adc1.Instance = ADC_DMA_INSTANCE;
    hdma_adc1.Init.Channel = ADC_DMA_CHANNEL;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR; //DMA_NORMAL  DMA_CIRCULAR
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdma_adc1.XferCpltCallback = adc_xferCplt_callback;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
        LOG_E("ADC DMA ERROR2");
    }
    __HAL_LINKDMA(&hadc1,DMA_Handle,hdma_adc1);
    HAL_NVIC_SetPriority(ADC_DMA_IRQ, 0, 0);
    HAL_NVIC_EnableIRQ(ADC_DMA_IRQ);
}
//初始化定时器
static void adc_tim_config()
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    /* USER CODE BEGIN TIM2_Init 1 */
    /* USER CODE END TIM2_Init 1 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 100-1;
    get_timer_period_prescaler(&htim2,40960);
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        LOG_E("TIM ERROR 1");
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        LOG_E("TIM ERROR 2");
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        LOG_E("TIM ERROR 3");
    }
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
    __HAL_TIM_DISABLE(&htim2);
}

void ADC_DMA_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_DMA_IRQHandler(&hdma_adc1);
    rt_interrupt_leave();
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    __HAL_TIM_DISABLE(&htim2);
    if(adc_fft_start_flag)
        rt_sem_release(adc_fft_sem);  
}
void adc_xferCplt_callback()
{

}
/*
	获得定时器频率比
*/
static void get_timer_period_prescaler(TIM_HandleTypeDef* htim,uint32_t _ulfreq)
{
    uint32_t TIMx_clk=0;
    uint32_t usPeriod;
    uint32_t usPrescaler;
    if( (htim->Instance==TIM1))
    {
        TIMx_clk = HAL_RCC_GetPCLK2Freq() *2;
    }
    else
    {
        TIMx_clk = HAL_RCC_GetPCLK1Freq() *2;
    }

    usPrescaler = 100-1;
    usPeriod = (TIMx_clk/100)/_ulfreq -1;
    htim->Init.Prescaler         = usPrescaler;
    htim->Init.Period            = usPeriod;
}


void adc_fft_start()
{
    adc_fft_start_flag = 1;
    adc_fft_sem = rt_sem_create("adc_fft",0,RT_IPC_FLAG_FIFO);
    rt_thread_t tid=rt_thread_create("adc_fft",fft_gen_thread,NULL,
                                    BSP_ADC_FFT_GEN_THREAD_STACK_SIZE,
                                    BSP_ADC_FFT_GEN_THREAD_PRIO,10);
    if(tid!=RT_NULL)
	    rt_thread_startup(tid);
    __HAL_TIM_ENABLE(&htim2);
}
void adc_fft_stop()
{
    __HAL_TIM_DISABLE(&htim2);
    adc_fft_start_flag = 0;
    rt_sem_delete(adc_fft_sem);
}
static void fft_gen_thread(void *arg)
{
    while(adc_fft_start_flag)
    {
        rt_sem_take(adc_fft_sem,RT_WAITING_FOREVER);
        adc_data_filiter();
        fft_cal();
        if(adc_fft_finish_cb != RT_NULL)
            adc_fft_finish_cb(fft_inputbuf, FFT_LEN); //执行cb
        __HAL_TIM_ENABLE(&htim2);
        // rt_thread_mdelay(50);
    }
}
float Han_Win(int i ,float data)
{
    float Wn;
    Wn = 0.5f-0.5f*arm_cos_f32((2 * PI*(i - 1))/(FFT_LEN - 1)); //w = .5*(1 - cos(2*pi*(1:m)'/(n+1))); 
    Wn = Wn * data;    
    return Wn;
}

static void adc_data_filiter()
{
    int i=0;
    uint8_t randval;
    float temp;
    int16_t adc_val;
    for(i=0; i<FFT_LEN; i++) //生成信号序列
    {
        adc_val = adc_buffer[i] - 1500;
        // rt_kprintf("%d,",adc_val);
        temp = adc_val * 1.2f;
        // temp=adc_buffer[i]; //-1552
        // if(((temp<0) && (temp>-THREHSOLD))  || ((temp>0)&&(temp<THREHSOLD))) 
        //     temp=0;
        temp = Han_Win(i, temp);
        fft_inputbuf[i] = temp;
    }
    // rt_kprintf("\n");
}

