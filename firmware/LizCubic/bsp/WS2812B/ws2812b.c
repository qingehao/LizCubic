#include "rtthread.h"
#include "board.h"
#include "ws2812b.h"

TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim3_ch2;

static void tim3_pwm_init(void);
static void tim3_pwm_dma_config(void);
static void ws_write_pixel(int index, uint8_t n_R, uint8_t n_G, uint8_t n_B);
static void ws_write_all(uint8_t n_R, uint8_t n_G, uint8_t n_B);
static void ws_clear(void);

static uint16_t *led_data_buf = RT_NULL;
static uint16_t buffer_size = 0;
static struct ws2812_rgbled rgb_led = {0};
static enum RGB_LED_MODE led_mode = RGB_LED_MODE_NONE;
static union ws_color now_color;

static int color_set_index = 0; //颜色改变的目标

static rt_timer_t timer_led_blink; //闪烁定时器
static rt_timer_t timer_led_breathe; //呼吸定时器

static struct rt_event led_sta_change_event;

#define LED_MODE_CHANGE_EVENT (1<<1)
#define LED_COLOR_CHANGE_EVENT (1<<2)
#define LED_BLINK_PERIOD_CHANGE_EVENT (1<<3)
#define LED_BREATHE_PERIOD_CHANGE_EVENT (1<<4)

/**
* @brief tim3 配置为通过dma+pwm
tim3 时钟在APB1上
* @param None
* @retval None
*/
static void tim3_pwm_init()
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 199;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  tim3_pwm_dma_config();
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    // Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    // Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    // Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    // Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    // Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}
/**
* @brief tim3 dma配置
* @param None
* @retval None
*/
static void tim3_pwm_dma_config()
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    hdma_tim3_ch2.Instance = DMA1_Stream5;
    hdma_tim3_ch2.Init.Channel = DMA_CHANNEL_5;
    hdma_tim3_ch2.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim3_ch2.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim3_ch2.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim3_ch2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_tim3_ch2.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_tim3_ch2.Init.Mode = DMA_NORMAL;
    hdma_tim3_ch2.Init.Priority = DMA_PRIORITY_LOW;
    hdma_tim3_ch2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_tim3_ch2) != HAL_OK)
    {
    //   Error_Handler();
    }
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    __HAL_LINKDMA(&htim3,hdma[TIM_DMA_ID_CC2],hdma_tim3_ch2);
}
/**
  * @brief This function handles DMA1 stream5 global interrupt.
  */
void DMA1_Stream5_IRQHandler(void)
{
  rt_interrupt_enter();
  HAL_DMA_IRQHandler(&hdma_tim3_ch2);
  rt_interrupt_leave();
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance = TIM3)
  {
    HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_2);
  }
}

static __inline void ws_load(void)
{
	HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_2, (uint32_t *)led_data_buf, buffer_size);
}

static __inline void ws_stop(void)
{
  HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_2);
}

static float min(float a, float b, float c)
{
  float m;
  
  m = a < b ? a : b;
  return (m < c ? m : c); 
}

static float max(float a, float b, float c)
{
  float m;
  
  m = a > b ? a : b;
  return (m > c ? m : c); 
}

static void rgb2hsv(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v)
{
  float red, green ,blue;
  float cmax, cmin, delta;
  
  red = (float)r / 255;
  green = (float)g / 255;
  blue = (float)b / 255;
  
  cmax = max(red, green, blue);
  cmin = min(red, green, blue);
  delta = cmax - cmin;

  /* H */
  if(delta == 0)
  {
    *h = 0;
  }
  else
  {
    if(cmax == red)
    {
      if(green >= blue)
      {
        *h = 60 * ((green - blue) / delta);
      }
      else
      {
        *h = 60 * ((green - blue) / delta) + 360;
      }
    }
    else if(cmax == green)
    {
      *h = 60 * ((blue - red) / delta + 2);
    }
    else if(cmax == blue) 
    {
      *h = 60 * ((red - green) / delta + 4);
    }
  }
  
  /* S */
  if(cmax == 0)
  {
    *s = 0;
  }
  else
  {
    *s = delta / cmax;
  }
  
  /* V */
  *v = cmax;
}
  
static void hsv2rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
  if(s == 0)
  {
    *r=*g=*b=v;
  }
  else
  {
    float H = h / 60;
    int hi = (int)H;
    float f = H - hi;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1- (1 - f) * s);
    switch (hi){
      case 0:
        *r = (int)(v * 255.0 + 0.5);
        *g = (int)(t * 255.0 + 0.5);
        *b = (int)(p * 255.0 + 0.5);
        break;
      case 1:
        *r = (int)(q * 255.0 + 0.5);
        *g = (int)(v * 255.0 + 0.5);
        *b = (int)(p * 255.0 + 0.5);
        break;
      case 2:
        *r = (int)(p * 255.0 + 0.5);
        *g = (int)(v * 255.0 + 0.5);
        *b = (int)(t * 255.0 + 0.5);
        break;
      case 3:
        *r = (int)(p * 255.0 + 0.5);
        *g = (int)(q * 255.0 + 0.5);
        *b = (int)(v * 255.0 + 0.5);
        break;
      case 4:
        *r = (int)(t * 255.0 + 0.5);
        *g = (int)(p * 255.0 + 0.5);
        *b = (int)(v * 255.0 + 0.5);
        break;
      case 5:
        *r = (int)(v * 255.0 + 0.5);
        *g = (int)(p * 255.0 + 0.5);
        *b = (int)(q * 255.0 + 0.5);
        break;
			default:
				break;
    }
  }
}

static void ws_clear(void)
{
	uint16_t i;
  now_color.hex = 0;
	for (i = 0; i < rgb_led.led_num * 24; i++)
		led_data_buf[i] = WS0; // 写入逻辑0的占空比
	for (i = rgb_led.led_num * 24; i < buffer_size; i++)
		led_data_buf[i] = 0; // 占空比比为0，全为低电平
	ws_load();
}

static void ws_write_pixel(int index, uint8_t n_R, uint8_t n_G, uint8_t n_B)
{
  // n_R = (n_R % 2 == 0)? n_R:n_R-1;
  // n_G = (n_G % 2 == 0)? n_G:n_G-1;
  // n_B = (n_B % 2 == 0)? n_B:n_B-1;
  now_color.r = n_R;
  now_color.g = n_G;
  now_color.b = n_B;
  uint32_t color = n_G << 16 | n_R << 8 | n_B;
  rt_memset(led_data_buf,0,buffer_size*2);
  if(index < rgb_led.led_num)
  {
    for(int i=0; i<24; i++)
    {
      led_data_buf[24*index+i] = (((color << i) & 0X800000) ? WS1 : WS0);
    }
  }
}

static void ws_write_all(uint8_t n_R, uint8_t n_G, uint8_t n_B)
{
  
  // n_R = (n_R % 2 == 0)? n_R:n_R-1;
  // n_G = (n_G % 2 == 0)? n_G:n_G-1;
  // n_B = (n_B % 2 == 0)? n_B:n_B-1;
  now_color.r = n_R;
  now_color.g = n_G;
  now_color.b = n_B;
	uint16_t i, j;
	uint8_t dat[24] = {0};
	// 将RGB数据进行转换
	for (i = 0; i < 8; i++)
	{
		dat[i] = ((n_G & 0x80) ? WS1 : WS0);
		n_G <<= 1;
	}
	for (i = 0; i < 8; i++)
	{
		dat[i + 8] = ((n_R & 0x80) ? WS1 : WS0);
		n_R <<= 1;
	}
	for (i = 0; i < 8; i++)
	{
		dat[i + 16] = ((n_B & 0x80) ? WS1 : WS0);
		n_B <<= 1;
	}
	for (i = 0; i < rgb_led.led_num; i++)
	{
		for (j = 0; j < 24; j++)
		{
			led_data_buf[i * 24 + j] = dat[j];
		}
	}
	for (i = rgb_led.led_num * 24; i < buffer_size; i++)
		led_data_buf[i] = 0; // 占空比比为0，全为低电平
}

int ws_set_color(int index,uint32_t c)
{
  color_set_index = index;
  rgb_led.color.hex = c;
  rt_event_send(&led_sta_change_event,LED_COLOR_CHANGE_EVENT);
}

int ws_set_mode(enum RGB_LED_MODE mode)
{
  led_mode = mode;
  rt_event_send(&led_sta_change_event,LED_MODE_CHANGE_EVENT);
  return 0;
}

int ws_set_blink_period(int period)
{
  rgb_led.blink_period = period;
  rt_event_send(&led_sta_change_event,LED_BLINK_PERIOD_CHANGE_EVENT);
}

int ws_set_breathe_period(int period)
{
  rgb_led.breathe_period = period;
  rt_event_send(&led_sta_change_event,LED_BREATHE_PERIOD_CHANGE_EVENT);
}

struct breathe_cfg
{
  uint8_t up_down_flag;
  float h;
  float s;
  float v;
}led_breathe_cfg;

#define BREATHE_V_MIN 0.0f
#define BREATHE_V_MAX 0.9f

static void rgb_led_breathe_loop()
{
  uint8_t r, g, b;
  if(led_breathe_cfg.up_down_flag == 1)
  {
    led_breathe_cfg.v -= 0.01f;
    if(led_breathe_cfg.v <= BREATHE_V_MIN)
    {
      led_breathe_cfg.v = BREATHE_V_MIN;
      led_breathe_cfg.up_down_flag = 0;
    }
  }
  else if(led_breathe_cfg.up_down_flag == 0)
  {
    led_breathe_cfg.v += 0.01f;
    if(led_breathe_cfg.v >= BREATHE_V_MAX)
    {
      led_breathe_cfg.v = BREATHE_V_MAX;
      led_breathe_cfg.up_down_flag = 1;
    }
  }
  else if(led_breathe_cfg.up_down_flag == 2) //渐隐
  {
    led_breathe_cfg.v -= 0.01f;
    if(led_breathe_cfg.v <= BREATHE_V_MIN)
    {
      led_breathe_cfg.v = BREATHE_V_MIN;
      rt_timer_stop(timer_led_breathe); // 关闭呼吸模式定时器
      ws_set_mode(RGB_LED_MODE_HOLDOFF);
      return;
    }
  }
  hsv2rgb( led_breathe_cfg.h, led_breathe_cfg.s, led_breathe_cfg.v, &r, &g, &b);//hsv转rgb
  ws_write_all(r, g, b);
  ws_load();
}

static void rgb_led_blink_loop()
{
  static uint8_t led_on_off_flag = 0;
  if(led_on_off_flag)
  {
    ws_write_all(rgb_led.color.r,rgb_led.color.g,rgb_led.color.b);
    ws_load();
    led_on_off_flag = 0;
  }
  else
  {
    ws_clear();
    led_on_off_flag = 1;
  }
}

void ws2812b_rgb_led_thread(void *arg)
{
  float h,s,v;
  uint32_t e = 0;
  while(1)
  {
    if(rt_event_recv(&led_sta_change_event, (LED_MODE_CHANGE_EVENT | LED_COLOR_CHANGE_EVENT | LED_BLINK_PERIOD_CHANGE_EVENT | LED_BREATHE_PERIOD_CHANGE_EVENT),
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER, &e) == RT_EOK)
    {
      if(e & LED_MODE_CHANGE_EVENT) //发生了模式改变
      {
        
        if(led_mode == RGB_LED_MODE_BREATHE) //转入的模式是呼吸模式
        {
          led_breathe_cfg.up_down_flag=1;
          led_breathe_cfg.v=0.0f; // 亮度值初始化为0;
          rgb2hsv(now_color.r, now_color.g, now_color.b, &h, &s, &v); //得到当前的hsv
          led_breathe_cfg.h = h;
          led_breathe_cfg.s = s;
          rt_timer_stop(timer_led_blink); // 关闭闪烁模式定时器
          rt_timer_start(timer_led_breathe); // 启动呼吸模式定时器
        }
        else if(led_mode == RGB_LED_MODE_FADEOUT) // 转入的模式是渐隐模式
        {
          led_breathe_cfg.up_down_flag = 2;
          rgb2hsv(now_color.r, now_color.g, now_color.b, &h, &s, &v); //得到当前的hsv
          led_breathe_cfg.v = v; // 亮度值初始化为当前;
          led_breathe_cfg.h = h;
          led_breathe_cfg.s = s;
          rt_timer_stop(timer_led_blink); // 关闭闪烁模式定时器         
          rt_timer_start(timer_led_breathe); // 启动呼吸模式定时器
        }
        else if(led_mode == RGB_LED_MODE_BLINK) // 闪烁模式
        {
          now_color.hex = rgb_led.color.hex; // 重置当前颜色
          rt_timer_start(timer_led_blink); // 开启闪烁模式定时器
          rt_timer_stop(timer_led_breathe); // 关闭呼吸模式定时器
        }
        else if(led_mode == RGB_LED_MODE_HOLDLIGHT) // 常亮模式
        {
          now_color.hex = rgb_led.color.hex; // 重置当前颜色
          rt_timer_stop(timer_led_blink); // 关闭闪烁模式定时器
          rt_timer_stop(timer_led_breathe); // 关闭呼吸模式定时器
          ws_set_color(RGB_LED_ALL,rgb_led.color.hex);
        }
        else if(led_mode == RGB_LED_MODE_HOLDOFF) // 常关模式
        {
          now_color.hex = rgb_led.color.hex; // 重置当前颜色
          rt_timer_stop(timer_led_blink); // 关闭闪烁模式定时器
          rt_timer_stop(timer_led_breathe); // 关闭呼吸模式定时器 
          ws_clear(); // 清屏
          rt_thread_mdelay(20);
          ws_stop();  // 停止传输    
        }
        else // 无模式
        {
          rt_timer_stop(timer_led_blink); // 关闭闪烁模式定时器
          rt_timer_stop(timer_led_breathe); // 关闭呼吸模式定时器
        }
      }
      if(e & LED_BLINK_PERIOD_CHANGE_EVENT) // 发生了周期改变
      {
        rt_timer_control(timer_led_blink,RT_TIMER_CTRL_SET_TIME,&rgb_led.blink_period);
      }
      if(e & LED_BREATHE_PERIOD_CHANGE_EVENT) // 发生了周期改变
      {
        rt_timer_control(timer_led_breathe,RT_TIMER_CTRL_SET_TIME,&rgb_led.breathe_period);
      }
      if(e & LED_COLOR_CHANGE_EVENT) // 发生了颜色改变
      {
        now_color.hex = rgb_led.color.hex; // 重置当前颜色
        if(led_mode == RGB_LED_MODE_BREATHE) //如果当前模式是呼吸模式
        {
          /* 亮度不变情况下，更改当前呼吸颜色 */
          rgb2hsv(now_color.r, now_color.g, now_color.b, &h, &s, &v); //得到当前的hsv
          led_breathe_cfg.h = h; 
          led_breathe_cfg.s = s;        
        }
        else if(led_mode == RGB_LED_MODE_BLINK) // 闪烁模式
        { // 
          // 不用管 已经更新了rgb_led.color
        }
        else // 常亮模式和无模式
        {
          ws_clear();
          if(color_set_index == RGB_LED_ALL)
          {
            ws_write_all(rgb_led.color.r,rgb_led.color.g,rgb_led.color.b);
          }
          else if(color_set_index >=0)
          {
            ws_write_pixel(color_set_index,rgb_led.color.r,rgb_led.color.g,rgb_led.color.b);
          }
          ws_load();           
        }
      }
    }
  }
}

ws2812_rgbled_t ws2812b_rgb_led_init(uint16_t led_num)
{
  if(rgb_led.is_init) return &rgb_led;
  rgb_led.led_num = led_num;
  buffer_size = 24*led_num + 300;
  led_data_buf = rt_calloc(1, buffer_size*2 );
  if(led_data_buf == RT_NULL) return RT_NULL;

  rgb_led.set_color = ws_set_color;
  rgb_led.set_mode = ws_set_mode;
  rgb_led.set_blink_period = ws_set_blink_period;
  rgb_led.set_breathe_period = ws_set_breathe_period;
  rgb_led.start = ws_load;
  rgb_led.stop = ws_stop;
  rgb_led.clear = ws_clear;
  
  tim3_pwm_init();
  rgb_led.is_init = 1;
  rgb_led.blink_period = 500;
  rgb_led.breathe_period = 40;
  rgb_led.clear();
  rgb_led.color.hex = 0;
  
  rt_event_init(&led_sta_change_event,"led_sta",RT_IPC_FLAG_PRIO);
  /* 创建闪灯定时器 */
  timer_led_blink = rt_timer_create("led_blink",rgb_led_blink_loop,RT_NULL,rgb_led.blink_period,RT_TIMER_FLAG_PERIODIC);
  /* 创建呼吸灯定时器 */
  timer_led_breathe = rt_timer_create("led_breathe",rgb_led_breathe_loop,RT_NULL,rgb_led.breathe_period,RT_TIMER_FLAG_PERIODIC);

  rt_thread_t tid = rt_thread_create("rgb",ws2812b_rgb_led_thread,RT_NULL,1024,30,10);
  rt_thread_startup(tid);
  return &rgb_led;
}

int rgb(int argc, char *argv[])
{
    ws2812b_rgb_led_init(1);
    if(argc > 2)
    {
      if(rt_strstr(argv[1],"set_color"))
      {
        rgb_led.set_color(0, strtol(argv[2], RT_NULL, 16));
      }
      else if(rt_strstr(argv[1],"set_mode"))
      {
        rgb_led.set_mode( (enum RGB_LED_MODE)atoi(argv[2]) );
      }
      else if(rt_strstr(argv[1],"set_period"))
      {
        if(led_mode == RGB_LED_MODE_BLINK)
        {
          rgb_led.set_blink_period(atoi(argv[2]));
        }
        else if(led_mode == RGB_LED_MODE_BREATHE)
        {
          rgb_led.set_breathe_period(atoi(argv[2]));
        }
      }
    }
    else
    {
      if(rt_strstr(argv[1],"clear"))
      {
        rt_kprintf("rgb clear\n");
        rgb_led.clear();
      }
    }
    rgb_led.start();
}

MSH_CMD_EXPORT(rgb,test);
