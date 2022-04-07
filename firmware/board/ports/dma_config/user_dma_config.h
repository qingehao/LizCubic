#ifndef __USER_DMA_CONFIG_H__
#define __USER_DMA_CONFIG_H__

#include <rtthread.h>

#if defined(BSP_USING_USER_DMA_CONFIG)

    #define ADC_DMA_IRQHandler              DMA2_Stream4_IRQHandler
    #define ADC_DMA_RCC                     RCC_AHB1ENR_DMA2EN
    #define ADC_DMA_INSTANCE                DMA2_Stream4
    #define ADC_DMA_CHANNEL                 DMA_CHANNEL_0
    #define ADC_DMA_IRQ                     DMA2_Stream4_IRQn

    #define TIM3_CH2_DMA_IRQHandler         DMA1_Stream5_IRQHandler
    #define TIM3_CH2_DMA_RCC                RCC_AHB1ENR_DMA1EN
    #define TIM3_CH2_DMA_INSTANCE           DMA1_Stream5
    #define TIM3_CH2_DMA_CHANNEL            DMA_CHANNEL_5
    #define TIM3_CH2_DMA_IRQ                DMA1_Stream5_IRQn 

    #define SPI1_DMA_TX_IRQHandler          DMA2_Stream5_IRQHandler
    #define SPI1_TX_DMA_RCC                 RCC_AHB1ENR_DMA2EN
    #define SPI1_TX_DMA_INSTANCE            DMA2_Stream5
    #define SPI1_TX_DMA_CHANNEL             DMA_CHANNEL_3
    #define SPI1_TX_DMA_IRQ                 DMA2_Stream5_IRQn
    
#if defined(BSP_UART1_RX_USING_DMA) /* DMA2_Stream2 */
    #define UART1_DMA_RX_IRQHandler         DMA2_Stream2_IRQHandler
    #define UART1_RX_DMA_RCC                RCC_AHB1ENR_DMA2EN
    #define UART1_RX_DMA_INSTANCE           DMA2_Stream2
    #define UART1_RX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART1_RX_DMA_IRQ                DMA2_Stream2_IRQn
#endif

#if defined(BSP_UART1_TX_USING_DMA) /* DMA2_Stream7 */
    #define UART1_DMA_TX_IRQHandler         DMA2_Stream7_IRQHandler
    #define UART1_TX_DMA_RCC                RCC_AHB1ENR_DMA2EN
    #define UART1_TX_DMA_INSTANCE           DMA2_Stream7
    #define UART1_TX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART1_TX_DMA_IRQ                DMA2_Stream7_IRQn
#endif

#if defined(BSP_UART2_RX_USING_DMA) /* DMA1_Stream5 */
    #define UART2_DMA_RX_IRQHandler          DMA1_Stream7_IRQHandler
    #define UART2_RX_DMA_RCC                 RCC_AHB1ENR_DMA1EN
    #define UART2_RX_DMA_INSTANCE            DMA1_Stream7
    #define UART2_RX_DMA_CHANNEL             DMA_CHANNEL_6
    #define UART2_RX_DMA_IRQ                 DMA1_Stream7_IRQn
#endif

#if defined(BSP_UART2_TX_USING_DMA) /* DMA1_Stream6 */
    #define UART2_DMA_TX_IRQHandler          DMA1_Stream6_IRQHandler
    #define UART2_TX_DMA_RCC                 RCC_AHB1ENR_DMA1EN
    #define UART2_TX_DMA_INSTANCE            DMA1_Stream6
    #define UART2_TX_DMA_CHANNEL             DMA_CHANNEL_4
    #define UART2_TX_DMA_IRQ                 DMA1_Stream6_IRQn
#endif

#if defined(BSP_SPI1_RX_USING_DMA) /* DMA2_Stream0 */
    #define SPI1_DMA_RX_IRQHandler           DMA2_Stream0_IRQHandler
    #define SPI1_RX_DMA_RCC                  RCC_AHB1ENR_DMA2EN
    #define SPI1_RX_DMA_INSTANCE             DMA2_Stream0
    #define SPI1_RX_DMA_CHANNEL              DMA_CHANNEL_3
    #define SPI1_RX_DMA_IRQ                  DMA2_Stream0_IRQn
#endif

#if defined(BSP_SPI1_TX_USING_DMA) /* DMA2_Stream5 */
    #define SPI1_DMA_TX_IRQHandler           DMA2_Stream5_IRQHandler
    #define SPI1_TX_DMA_RCC                  RCC_AHB1ENR_DMA2EN
    #define SPI1_TX_DMA_INSTANCE             DMA2_Stream5
    #define SPI1_TX_DMA_CHANNEL              DMA_CHANNEL_3
    #define SPI1_TX_DMA_IRQ                  DMA2_Stream5_IRQn
#endif

#if defined(BSP_SPI4_RX_USING_DMA) /* DMA2_Stream3 */
    #define SPI4_DMA_RX_IRQHandler           DMA2_Stream3_IRQHandler
    #define SPI4_RX_DMA_RCC                  RCC_AHB1ENR_DMA2EN
    #define SPI4_RX_DMA_INSTANCE             DMA2_Stream3
    #define SPI4_RX_DMA_CHANNEL              DMA_CHANNEL_5
    #define SPI4_RX_DMA_IRQ                  DMA2_Stream3_IRQn
#endif

#if defined(BSP_SPI4_TX_USING_DMA) /* DMA2_Stream1 */
    #define SPI4_DMA_TX_IRQHandler           DMA2_Stream1_IRQHandler
    #define SPI4_TX_DMA_RCC                  RCC_AHB1ENR_DMA2EN
    #define SPI4_TX_DMA_INSTANCE             DMA2_Stream1
    #define SPI4_TX_DMA_CHANNEL              DMA_CHANNEL_4
    #define SPI4_TX_DMA_IRQ                  DMA2_Stream1_IRQn
#endif

#endif /* BSP_USING_USER_DMA_CONFIG */

#endif /* __USER_DMA_CONFIG_H__ */
