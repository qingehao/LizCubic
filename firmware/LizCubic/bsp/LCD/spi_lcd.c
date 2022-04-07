#include "board.h"
#include "spi_lcd.h"
#include "stdint.h"
#include "stm32f4xx_hal.h"
#include "liz_config.h"

/* LCD_RS 寄存器选择 */
#define LCD_DC_GPIO         GPIOB
#define LCD_DC_PIN          GPIO_PIN_2
#define LCD_DC_HIGH()       LCD_DC_GPIO->BSRR=LCD_DC_PIN
#define LCD_DC_LOW()        LCD_DC_GPIO->BSRR=LCD_DC_PIN<<16U

/* LCD_CS SPI片选*/
#define LCD_CS_GPIO         GPIOB
#define LCD_CS_PIN          GPIO_PIN_1
#define LCD_CS_HIGH()       LCD_CS_GPIO->BSRR=LCD_CS_PIN
#define LCD_CS_LOW()        LCD_CS_GPIO->BSRR=LCD_CS_PIN<<16U


/* LCD_RESET 复位 */
#define LCD_RESET_GPIO      GPIOB
#define LCD_RESET_PIN       GPIO_PIN_0
#define LCD_RESET_HIGH()    LCD_RESET_GPIO->BSRR=LCD_RESET_PIN
#define LCD_RESET_LOW()     LCD_RESET_GPIO->BSRR=LCD_RESET_PIN<<16U

/* LCD_BLK 背光 */
#define LCD_BLK_GPIO        GPIOB
#define LCD_BLK_PIN         GPIO_PIN_8
#define LCD_BLK_HIGH()      LCD_BLK_GPIO->BSRR=LCD_BLK_PIN
#define LCD_BLK_LOW()       LCD_BLK_GPIO->BSRR=LCD_BLK_PIN<<16U

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;
TIM_HandleTypeDef htim4;

static void lcd_flush_ready()
{
    return;
}

static void lcd_blk_set(uint8_t en);
static void (*lcd_transmitCplt_callback)() = lcd_flush_ready;
static void lcd_blk_pwm_init();

static void lcd_spi_dma_init()
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        // Error_Handler();
    }
    __HAL_RCC_DMA2_CLK_ENABLE();
    hdma_spi1_tx.Instance = SPI1_TX_DMA_INSTANCE;
    hdma_spi1_tx.Init.Channel = SPI1_TX_DMA_CHANNEL;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi1_tx.Init.Mode = DMA_NORMAL;
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
    {
        // Error_Handler();
    }

    __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);

    HAL_NVIC_SetPriority(SPI1_TX_DMA_IRQ, 1, 1);
    HAL_NVIC_EnableIRQ(SPI1_TX_DMA_IRQ);

    HAL_NVIC_SetPriority(SPI1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
}

static void lcd_gpio_init()
{
    GPIO_InitTypeDef gpio_init;
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;        /* 设置推挽输出 */
    gpio_init.Pull = GPIO_NOPULL;                        /* 上下拉电阻不使能 */
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH; /* GPIO速度等级 */

    gpio_init.Pin = LCD_DC_PIN;
    HAL_GPIO_Init(LCD_DC_GPIO, &gpio_init);

    gpio_init.Pin = LCD_CS_PIN;
    HAL_GPIO_Init(LCD_CS_GPIO, &gpio_init);

    gpio_init.Pin = LCD_RESET_PIN;
    HAL_GPIO_Init(LCD_RESET_GPIO, &gpio_init);

    // gpio_init.Pin = LCD_BLK_PIN;
    // HAL_GPIO_Init(LCD_BLK_GPIO, &gpio_init);
    lcd_blk_pwm_init();
}

static void lcd_blk_pwm_init()
{
    /* USER CODE BEGIN TIM4_Init 0 */

    /* USER CODE END TIM4_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* USER CODE BEGIN TIM4_Init 1 */

    /* USER CODE END TIM4_Init 1 */
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 99;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 999;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
    {
        return;
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
    {
        return;
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        return;
    }
    /* USER CODE BEGIN TIM4_Init 2 */

    /* USER CODE END TIM4_Init 2 */
    HAL_TIM_MspPostInit(&htim4);
    HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_3);
}

void lcd_blk_set_brightness(uint16_t duty)
{
    rt_kprintf("set %d\n",duty);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, duty);
    
}
int lcd_bk(int argc, char **argv)
{
    lcd_blk_set_brightness(atoi(argv[1]));
    return 0;
}
MSH_CMD_EXPORT(lcd_bk, BK);
void SPI1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_SPI_IRQHandler(&hspi1);
    rt_interrupt_leave();
}

void SPI1_DMA_TX_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_DMA_IRQHandler(hspi1.hdmatx);
    rt_interrupt_leave();
}




void lcd_register_transmitCplt_callback(void (*callback)())
{
    lcd_transmitCplt_callback = callback;
}
static void lcd_dma_error(DMA_HandleTypeDef *hdma)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)(((DMA_HandleTypeDef *)hdma)->Parent); /* Derogation MISRAC2012-Rule-11.5 */
    /* Stop the disable DMA transfer on SPI side */
    CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
    SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_DMA);
    hspi->State = HAL_SPI_STATE_READY;
}

static void lcd_spi_dma_halftxcplt_callback(DMA_HandleTypeDef *hdma)
{
    return;
}

void lcd_spi_dma_txcplt_callback(DMA_HandleTypeDef *hdma)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)(((DMA_HandleTypeDef *)hdma)->Parent); /* Derogation MISRAC2012-Rule-11.5 */

    /* DMA Normal Mode */
    if ((hdma->Instance->CR & DMA_SxCR_CIRC) != DMA_SxCR_CIRC)
    {
        /* Disable ERR interrupt */
        __HAL_SPI_DISABLE_IT(hspi, SPI_IT_ERR);

        /* Disable Tx DMA Request */
        CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);

        __HAL_SPI_CLEAR_OVRFLAG(hspi);

        hspi->TxXferCount = 0U;
        hspi->State = HAL_SPI_STATE_READY;

        if (hspi->ErrorCode != HAL_SPI_ERROR_NONE)
        {
            /* Call user error callback */
            HAL_SPI_ErrorCallback(hspi);
            return;
        }
    }

    lcd_transmitCplt_callback();
    LCD_CS_HIGH();
}

static int _spi_transmit_dma(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t size)
{
    int errorcode = HAL_OK;

    /* Check tx dma handle */
    assert_param(IS_SPI_DMA_HANDLE(hspi->hdmatx));

    /* Check Direction parameter */
    assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE(hspi->Init.Direction));
    hspi->Init.DataSize = SPI_DATASIZE_16BIT;
    WRITE_REG(hspi1.Instance->CR1, ((hspi1.Init.Mode & (SPI_CR1_MSTR | SPI_CR1_SSI)) |
                                    (hspi1.Init.Direction & (SPI_CR1_RXONLY | SPI_CR1_BIDIMODE)) |
                                    (hspi1.Init.DataSize & SPI_CR1_DFF) |
                                    (hspi1.Init.CLKPolarity & SPI_CR1_CPOL) |
                                    (hspi1.Init.CLKPhase & SPI_CR1_CPHA) |
                                    (hspi1.Init.NSS & SPI_CR1_SSM) |
                                    (hspi1.Init.BaudRatePrescaler & SPI_CR1_BR_Msk) |
                                    (hspi1.Init.FirstBit  & SPI_CR1_LSBFIRST) |
                                    (hspi1.Init.CRCCalculation & SPI_CR1_CRCEN)));

    /* Process Locked */
    __HAL_LOCK(hspi);

    if (hspi->State != HAL_SPI_STATE_READY)
    {
        errorcode = HAL_BUSY;
        goto error;
    }

    if ((pData == NULL) || (size == 0U))
    {
        errorcode = HAL_ERROR;
        goto error;
    }

    /* Set the transaction information */
    hspi->State       = HAL_SPI_STATE_BUSY_TX;
    hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
    hspi->pTxBuffPtr  = (uint8_t *)pData;
    hspi->TxXferSize  = size;
    hspi->TxXferCount = size;

    /* Init field not used in handle to zero */
    hspi->pRxBuffPtr  = (uint8_t *)NULL;
    hspi->TxISR       = NULL;
    hspi->RxISR       = NULL;
    hspi->RxXferSize  = 0U;
    hspi->RxXferCount = 0U;

    hspi->hdmatx->XferHalfCpltCallback = NULL;
    hspi->hdmatx->XferCpltCallback = lcd_spi_dma_txcplt_callback;
    hspi->hdmatx->XferErrorCallback = NULL;
    hspi->hdmatx->XferAbortCallback = NULL;

    if (HAL_OK != HAL_DMA_Start_IT(hspi->hdmatx, (uint32_t)hspi->pTxBuffPtr, (uint32_t)&hspi->Instance->DR,
                                   hspi->TxXferCount))
    {
        /* Update SPI error code */
        SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_DMA);
        errorcode = HAL_ERROR;

        hspi->State = HAL_SPI_STATE_READY;
        goto error;
    }

    /* Check if the SPI is already enabled */
    if ((hspi->Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
    {
        /* Enable SPI peripheral */
        __HAL_SPI_ENABLE(hspi);
    }

    /* Enable the SPI Error Interrupt Bit */
    __HAL_SPI_ENABLE_IT(hspi, (SPI_IT_ERR));

    /* Enable Tx DMA Request */
    SET_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);

error :
    /* Process Unlocked */
    __HAL_UNLOCK(hspi);
    return errorcode;
}

static void _spi_write_byte(uint8_t data)
{
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    WRITE_REG(hspi1.Instance->CR1, ((hspi1.Init.Mode & (SPI_CR1_MSTR | SPI_CR1_SSI)) |
                                    (hspi1.Init.Direction & (SPI_CR1_RXONLY | SPI_CR1_BIDIMODE)) |
                                    (hspi1.Init.DataSize & SPI_CR1_DFF) |
                                    (hspi1.Init.CLKPolarity & SPI_CR1_CPOL) |
                                    (hspi1.Init.CLKPhase & SPI_CR1_CPHA) |
                                    (hspi1.Init.NSS & SPI_CR1_SSM) |
                                    (hspi1.Init.BaudRatePrescaler & SPI_CR1_BR_Msk) |
                                    (hspi1.Init.FirstBit  & SPI_CR1_LSBFIRST) |
                                    (hspi1.Init.CRCCalculation & SPI_CR1_CRCEN)));
    if ((hspi1.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
    {
        /* Enable SPI peripheral */
        __HAL_SPI_ENABLE(&hspi1);
    }
    while ((SPI1->SR & SPI_FLAG_TXE) == 0);

    *((__IO uint8_t *)&SPI1->DR) = data;

    while ((SPI1->SR & SPI_FLAG_TXE) == 0);

    __HAL_SPI_CLEAR_OVRFLAG(&hspi1);

    SPI1->CR1 &= ~(SPI_CR1_SPE);
}

static int lcd_write_cmd(const uint8_t cmd)
{
    LCD_CS_LOW();
    LCD_DC_LOW();
    _spi_write_byte(cmd);
    LCD_CS_HIGH();
    return 0;
}

static int lcd_write_data(const uint8_t data)
{
    LCD_CS_LOW();
    LCD_DC_HIGH();
    _spi_write_byte(data);
    LCD_CS_HIGH();
    return 0;
}

static int lcd_write_half_word(const uint16_t data)
{
    LCD_CS_LOW();
    LCD_DC_HIGH();
    _spi_write_byte((uint8_t)(data >> 8));
    _spi_write_byte((uint8_t)(data & 0x00ff));
    LCD_CS_HIGH();
    return 0;
}

int spi_lcd_init()
{
    lcd_gpio_init();
    lcd_spi_dma_init();
    LCD_RESET_LOW();
    rt_thread_mdelay(5);
    LCD_RESET_HIGH();
    rt_thread_mdelay(10);
    /* Memory Data Access Control */
    lcd_write_cmd(0x36);
    lcd_write_data(0x40);
    /* RGB 5-6-5-bit  */
    lcd_write_cmd(0x3A);
    lcd_write_data(0x05);
    /* Porch Setting */
    lcd_write_cmd(0xB2);
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);
    /*  Gate Control */
    lcd_write_cmd(0xB7);
    lcd_write_data(0x35);
    /* VCOM Setting */
    lcd_write_cmd(0xBB);
    lcd_write_data(0x19);
    /* LCM Control */
    lcd_write_cmd(0xC0);
    lcd_write_data(0x2C);
    /* VDV and VRH Command Enable */
    lcd_write_cmd(0xC2);
    lcd_write_data(0x01);
    /* VRH Set */
    lcd_write_cmd(0xC3);
    lcd_write_data(0x12);
    /* VDV Set */
    lcd_write_cmd(0xC4);
    lcd_write_data(0x20);
    /* Frame Rate Control in Normal Mode */
    lcd_write_cmd(0xC6);
    lcd_write_data(0x0F);
    /* Power Control 1 */
    lcd_write_cmd(0xD0);
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);
    /* Positive Voltage Gamma Control */
    lcd_write_cmd(0xE0);
    lcd_write_data(0xD0);
    lcd_write_data(0x04);
    lcd_write_data(0x0D);
    lcd_write_data(0x11);
    lcd_write_data(0x13);
    lcd_write_data(0x2B);
    lcd_write_data(0x3F);
    lcd_write_data(0x54);
    lcd_write_data(0x4C);
    lcd_write_data(0x18);
    lcd_write_data(0x0D);
    lcd_write_data(0x0B);
    lcd_write_data(0x1F);
    lcd_write_data(0x23);
    /* Negative Voltage Gamma Control */
    lcd_write_cmd(0xE1);
    lcd_write_data(0xD0);
    lcd_write_data(0x04);
    lcd_write_data(0x0C);
    lcd_write_data(0x11);
    lcd_write_data(0x13);
    lcd_write_data(0x2C);
    lcd_write_data(0x3F);
    lcd_write_data(0x44);
    lcd_write_data(0x51);
    lcd_write_data(0x2F);
    lcd_write_data(0x1F);
    lcd_write_data(0x1F);
    lcd_write_data(0x20);
    lcd_write_data(0x23);
    /* Display Inversion On */
    lcd_write_cmd(0x21);
    /* Sleep Out */
    lcd_write_cmd(0x11);
    rt_thread_mdelay(100);
    lcd_blk_set(1);
    lcd_write_cmd(0x29);

    return 0;
}

static void lcd_blk_set(uint8_t en)
{
    // if (en)
    // {
    //     LCD_BLK_HIGH();
    // }
    // else
    // {
    //     LCD_BLK_LOW();

    // }
}

/**
 * Set drawing area
 *
 * @param   x1      start of x position
 * @param   y1      start of y position
 * @param   x2      end of x position
 * @param   y2      end of y position
 *
 * @return  void
 */
void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_write_cmd(0x2a);
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1);
    lcd_write_data(x2 >> 8);
    lcd_write_data(x2);

    lcd_write_cmd(0x2b);
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1);
    lcd_write_data(y2 >> 8);
    lcd_write_data(y2);

    lcd_write_cmd(0x2C);
}

/**
 * full color array on the lcd.
 *
 * @param   x_start     start of x position
 * @param   y_start     start of y position
 * @param   x_end       end of x position
 * @param   y_end       end of y position
 * @param   color       Fill color array's pointer
 *
 * @return  void
 */
void lcd_fill_array(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, void *pcolor)
{
    uint32_t size = 0;

    size = (x_end - x_start + 1) * (y_end - y_start + 1);
    lcd_address_set(x_start, y_start, x_end, y_end);
    LCD_DC_HIGH();
    LCD_CS_LOW();
    _spi_transmit_dma(&hspi1, (uint8_t *)pcolor, size);
}