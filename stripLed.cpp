
#include "ws2812.h"
#include "analog.h"
#include "timer.h"
/* Variables -----------------------------------------------*/


// static uint8_t LEDbuffer[LED_BUFFER_SIZE];

static uint8_t *MyTempbuffer;
static uint32_t MyTempbufsize;
static uint16_t NbLed;
static uint8_t Brightness;
static uint32_t last_date_call=0U;

//#define WS_RGB
//#define WS_BGR
//#define WS_GRB
#define COLOR(r,g,b)		(r<<16 | g<<8 | b)

stripLed::stripLed(rgb_order_e ws_order) : _order(ws_order) {
	MyTempbuffer = (uint8_t *)malloc(LED_BUFFER_SIZE);
	MyTempbufsize = LED_BUFFER_SIZE;
	NbLed = LED_NUMBER;
};

stripLed::stripLed(uint8_t *buf , uint32_t size) {
	stripLed(buf,size,WS_GRB);
};

stripLed::stripLed(uint8_t *buf , uint32_t size , rgb_order_e ws_order) {
	if(buf!=NULL) {
		MyTempbuffer  = buf;
		MyTempbufsize = size;
		NbLed = CALC_NBLED(size);
	}else{
		// fallback constructor
		stripLed(ws_order);
	}
	_order = ws_order;
};

void stripLed::begin(void)
{
	fillBufferBlack();
	
	this->_init();

	Brightness=FULL_BRIGHTNESS_RANGE;
}

void stripLed::Clear(void)
{
	fillBufferBlack();
}

void stripLed::Fill(uint8_t red, uint8_t green, uint8_t blue)
{
	setWHOLEcolor(red, green, blue);
}


void stripLed::setWHOLEcolor(uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
	uint32_t index;

	for (index = 0; index < NbLed; index++)
		setLEDcolor(index, RED, GREEN, BLUE);
}


void stripLed::fillBufferBlack(void) {
	/*Fill LED buffer - ALL OFF*/
	uint32_t index, buffIndex;
	buffIndex = 0;

	for (index = 0; index < RESET_SLOTS_BEGIN; index++) {
		*(MyTempbuffer+buffIndex) = WS2812_RESET;
		buffIndex++;
	}
	for (index = 0; index < LED_DATA_SIZE(NbLed); index++) {
		*(MyTempbuffer+buffIndex) = WS2812_0;
		buffIndex++;
	}
	*(MyTempbuffer+buffIndex) = WS2812_0;
	buffIndex++;
	for (index = 0; index < RESET_SLOTS_END; index++) {
		*(MyTempbuffer+buffIndex) = 0;
		buffIndex++;
	}
}

void stripLed::fillBufferWhite(void) {
	/*Fill LED buffer - ALL ON*/
	uint32_t index, buffIndex;
	buffIndex = 0;

	for (index = 0; index < RESET_SLOTS_BEGIN; index++) {
		*(MyTempbuffer+buffIndex) = WS2812_RESET;
		buffIndex++;
	}
	for (index = 0; index < LED_DATA_SIZE(NbLed); index++) {
		*(MyTempbuffer+buffIndex) = WS2812_1;
		buffIndex++;
	}
	*(MyTempbuffer+buffIndex) = WS2812_0;
	buffIndex++;
	for (index = 0; index < RESET_SLOTS_END; index++) {
		*(MyTempbuffer+buffIndex) = 0;
		buffIndex++;
	}
}

void stripLed::setLEDcolor(uint32_t LEDnumber, uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
	uint8_t tempBuffer[24];
	uint32_t i;
	uint32_t LEDindex;
	uint8_t color1;
	uint8_t color2;
	uint8_t color3;

	if(LEDnumber<NbLed)
	{
		LEDindex = LEDnumber % NbLed;
		switch(_order)
		{
			case WS_GRB:
				color1 = GREEN;
				color2 = RED;
				color3 = BLUE;
				break;
			case WS_RGB:
				color1 = RED;
				color2 = GREEN;
				color3 = BLUE;
				break;
			default:
				color1 = BLUE;
				color2 = GREEN;
				color3 = RED;
		}
		for (i = 0; i < 8; i++) // GREEN data
			tempBuffer[i] = ((color1 << i) & 0x80) ? WS2812_1 : WS2812_0;
		for (i = 0; i < 8; i++) // RED
			tempBuffer[8 + i] = ((color2 << i) & 0x80) ? WS2812_1 : WS2812_0;
		for (i = 0; i < 8; i++) // BLUE
			tempBuffer[16 + i] = ((color3 << i) & 0x80) ? WS2812_1 : WS2812_0;

		for (i = 0; i < 24; i++)
			*(MyTempbuffer+RESET_SLOTS_BEGIN + LEDindex * 24 + i) = tempBuffer[i];
	}
}
#if defined(STM32L476xx)
# include "l476.init"
#elif defined(STM32WB55xx)
# include "wb55.init"
#elif defined(STM32L432xx)
# include "l432.init"
#else
# error "from library ws2812 : MCU not supported"
#endif
