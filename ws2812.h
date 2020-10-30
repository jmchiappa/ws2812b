#ifndef __ws2812
#define __ws2812

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "ws2812_def.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t alpha;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} pixel_t;

typedef enum {
	WS_GRB=0,
	WS_BGR,
	WS_RGB
}rgb_order_e;

void TIMx_DMA_IRQHandler(void);
void ws2812_update(void);


class WS2812B
{
	public:
		//WS2812B(rgb_order_e ws_order=WS_GRB);      // constructor
		WS2812B(rgb_order_e ws_order=WS_GRB);      // constructor
		WS2812B(uint8_t *buf , uint32_t size);      // constructor
		WS2812B(uint8_t *buf , uint32_t size, rgb_order_e ws_order);      // constructor
		void begin(void);
		void Clear(void);        // fill the screen with black color
		void Clear(uint8_t nb_layer);
		void Rectangle(uint8_t nb_layer,int8_t x, int8_t y, uint8_t width, uint8_t height, uint8_t alpha,uint8_t red, uint8_t green, uint8_t blue);
		void Fill(uint8_t nb_layer,uint8_t alpha,uint8_t red, uint8_t green, uint8_t blue);
		void Bitmap(uint8_t nb_layer,int8_t x, int8_t y, uint8_t width, uint8_t height, uint8_t *bitmap);
		void SetPixelAt(uint8_t nb_layer,int8_t x, int8_t y, uint8_t alpha,uint8_t red, uint8_t green, uint8_t blue);
		void SetPixelAt(uint8_t nb_layer,uint8_t nb_led, uint8_t alpha,uint8_t red, uint8_t green, uint8_t blue);
		void fillBackground(uint8_t red, uint8_t green, uint8_t blue);
		void SetBrightness(uint8_t nb_layer,uint8_t brightness);
		void setLEDcolor(uint32_t LEDnumber, uint8_t RED, uint8_t GREEN, uint8_t BLUE);
		// static void updateCallback(void);
		void fillBufferBlack(void);
	private:

		/* 	-1 : non sauvegardé					*/
		/*	>0 : couleur RGB 1 byte/composante 	*/
		// static int32_t BackUpBuffer[LED_NUMBER];
		// static int32_t ScreenBuffer[LED_NUMBER];

		// declare the graphiclayer
		/* Layer : 5 Top
		 * Layer : 0 Background
		 */

		void setWHOLEcolor(uint8_t RED, uint8_t GREEN, uint8_t BLUE);
		void fillBufferWhite(void);
		bool Check_Zone(int8_t x,int8_t y);
		void Make_Layer_composite(void);
		void Make_Pixel_Composite(int8_t x, int8_t y);
		rgb_order_e _order;
};

#ifdef __cplusplus
}
#endif

#endif
