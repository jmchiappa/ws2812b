#ifndef __ws2812_def
#define __ws2812_def

/*#include <stdio.h>
#include <string.h>
#include <inttypes.h>
*/
#include "Arduino.h"

/* Definition of TIM instance */
#define TIMx                             	TIM16

/* Definition for TIMx clock resources */
#define TIMx_CLK_ENABLE                  	__HAL_RCC_TIM16_CLK_ENABLE
#define DMAx_CLK_ENABLE                  	__HAL_RCC_DMA1_CLK_ENABLE

/* Definition for TIMx Pins - refer to p. 93/94 DM00108832 */
#define TIMx_CHANNEL1_GPIO_CLK_ENABLE    	__HAL_RCC_GPIOA_CLK_ENABLE
#define TIMx_GPIO_CHANNEL1_PORT          	GPIOA
#define GPIO_PIN_CHANNEL1                	GPIO_PIN_6
#define GPIO_AF_TIMx                     	GPIO_AF14_TIM16

/* Definition for TIMx's DMA - refer to p343 RM0351 / DM00083560 */
#define TIMx_CC1_DMA_REQUEST             	DMA_REQUEST_4
#define TIMx_CC1_DMA_INST                	DMA1_Channel6

/* Definition for DMAx's NVIC */
#define TIMx_DMA_IRQn                    	DMA1_Channel6_IRQn
#define TIMx_DMA_IRQHandler              	DMA1_Channel6_IRQHandler

//WS2812
#define MATRIX_NB_COLUMN					(8)
#define MATRIX_NB_ROW						(8)
#define WS2812_FREQ							(800000) 			// it is fixed: WS2812 require 800kHz
#define TIMER_CLOCK_FREQ					(8000000)   	// can be modified - multiples of 0.8MHz are suggested
#define TIMER_PERIOD						(TIMER_CLOCK_FREQ / WS2812_FREQ)
#define LED_NUMBER							(MATRIX_NB_ROW*MATRIX_NB_COLUMN)					// how many LEDs the MCU should control?
#define LED_DATA_SIZE						(LED_NUMBER * 24)
#define RESET_SLOTS_BEGIN					(50)
#define RESET_SLOTS_END						(50)
#define WS2812_LAST_SLOT					(1)
#define LED_BUFFER_SIZE						(RESET_SLOTS_BEGIN + LED_DATA_SIZE + WS2812_LAST_SLOT + RESET_SLOTS_END)
#define WS2812_0							(TIMER_PERIOD / 3)				// WS2812's zero high time is long about one third of the period
#define WS2812_1							(TIMER_PERIOD * 2 / 3)		// WS2812's one high time is long about two thirds of the period
#define WS2812_RESET						(0)

#define LAYER_MAX					(5)
#define LAYER_BACKGROUND			(0)
#define LAYER_TOP					(LAYER_MAX-1)
#define RGB_FULL_RANGE				(255)			// Full range of each rgba value
#define FULL_BRIGHTNESS_RANGE		(255.0f)
#endif
