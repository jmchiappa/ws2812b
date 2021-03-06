#ifndef __ws2812_def
#define __ws2812_def

/*#include <stdio.h>
#include <string.h>
#include <inttypes.h>
*/
#include "Arduino.h"

//WS2812
#define MATRIX_NB_COLUMN				(8)
#define MATRIX_NB_ROW						(8)
#define WS2812_FREQ							(800000) 			// it is fixed: WS2812 require 800kHz
#define TIMER_CLOCK_FREQ				(8000000)   	// can be modified - multiples of 0.8MHz are suggested
#define TIMER_PERIOD						(TIMER_CLOCK_FREQ / WS2812_FREQ)
#define LED_NUMBER							(MATRIX_NB_ROW*MATRIX_NB_COLUMN)					// how many LEDs the MCU should control?
#define LED_DATA_SIZE(ledNumber)						(ledNumber * 24)
#define RESET_SLOTS_BEGIN				(200)
#define RESET_SLOTS_END					(200)
#define WS2812_LAST_SLOT				(1)
#define BUF_SIZE(x)							(RESET_SLOTS_BEGIN + LED_DATA_SIZE(x) + WS2812_LAST_SLOT + RESET_SLOTS_END)
#define CALC_NBLED(x)						(uint16_t)((x - RESET_SLOTS_BEGIN - WS2812_LAST_SLOT - RESET_SLOTS_END)/24)
#define LED_BUFFER_SIZE					BUF_SIZE(LED_NUMBER)
#define WS2812_0								(TIMER_PERIOD / 3)				// WS2812's zero high time is long about one third of the period
#define WS2812_1								(TIMER_PERIOD * 2 / 3)		// WS2812's one high time is long about two thirds of the period
#define WS2812_RESET						(0)

#define LAYER_MAX								(5)
#define LAYER_BACKGROUND				(0)
#define LAYER_TOP								(LAYER_MAX-1)
#define RGB_FULL_RANGE					(255)			// Full range of each rgba value
#define FULL_BRIGHTNESS_RANGE		(255.0f)
#endif
