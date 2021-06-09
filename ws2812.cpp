
#include "ws2812.h"
#include "analog.h"
#include "timer.h"
/* Variables -----------------------------------------------*/


GPIO_InitTypeDef GPIO_InitStruct;
DMA_HandleTypeDef hdma_tim;
TIM_HandleTypeDef _TimHandle;
TIM_OC_InitTypeDef _sConfig;
static uint8_t LEDbuffer[LED_BUFFER_SIZE];

static uint8_t *MyTempbuffer;
static uint32_t MyTempbufsize;
static uint16_t NbLed;
static pixel_t Layer[LAYER_MAX][MATRIX_NB_ROW][MATRIX_NB_COLUMN];
static uint8_t Brightness[LAYER_MAX];
static uint32_t last_date_call=0U;

//#define WS_RGB
//#define WS_BGR
//#define WS_GRB
#define INDEX_LED(x,y)		(x+(y*MATRIX_NB_COLUMN))
#define COLOR(r,g,b)		(r<<16 | g<<8 | b)

#define ALPHA_SOLID			RGB_FULL_RANGE
#define ALPHA_TRANSPARENT	0

// #define SCREEN_REFRESH_PERIOD	25
/* Functions -----------------------------------------------*/

// void WS2812B::updateCallback(void)
// {
// 	// Serial.println(last_date_call);
// 	if((HAL_GetTick()-last_date_call)>SCREEN_REFRESH_PERIOD)
// 	{
// 		WS2812B::Make_Layer_composite();
// 		Serial.print(".");
// 		last_date_call = HAL_GetTick();
// 		ws2812_update();
// 	}
// 	  // ws2812_update();
// }





/* Constructeur Initialise le screen */

WS2812B::WS2812B(rgb_order_e ws_order) : _order(ws_order) {
	MyTempbuffer = LEDbuffer;
	MyTempbufsize = LED_BUFFER_SIZE;
	NbLed = LED_NUMBER;
};

WS2812B::WS2812B(uint8_t *buf , uint32_t size) {
	WS2812B(buf,size,WS_GRB);
};

WS2812B::WS2812B(uint8_t *buf , uint32_t size , rgb_order_e ws_order) {
	if(buf!=NULL) {
		MyTempbuffer  = buf;
		MyTempbufsize = size;
		NbLed = CALC_NBLED(size);
	}else{
		NbLed = LED_NUMBER; 
		MyTempbuffer  = LEDbuffer;
		MyTempbufsize = LED_BUFFER_SIZE;
	}
	_order = ws_order;
};

void WS2812B::begin(void)
{
	fillBufferBlack();
	
	this->_init();

	memset(Layer,0, sizeof(Layer));
	memset(Brightness,FULL_BRIGHTNESS_RANGE,sizeof(Brightness));
	// Serial.println(NbLed);
}

void WS2812B::Clear(void)
{
	Fill(LAYER_BACKGROUND,ALPHA_TRANSPARENT,0,0,0);
	// fillBufferBlack();
}

void WS2812B::Clear(uint8_t nb_layer)
{
	Fill(nb_layer,ALPHA_TRANSPARENT,0,0,0);
}

void WS2812B::Rectangle(uint8_t nb_layer,int8_t x, int8_t y, uint8_t width, uint8_t height, uint8_t alpha,uint8_t red, uint8_t green, uint8_t blue)
{
	int8_t i,j;
	for(j=y;j<(y+height);j++)
		for(i=x;i<(x+width);i++)
		{
			SetPixelAt(nb_layer,i,j,alpha,red,green,blue);
		}
}

void WS2812B::Fill(uint8_t nb_layer,uint8_t alpha,uint8_t red, uint8_t green, uint8_t blue)
{
	// setWHOLEcolor(red, green, blue);
	uint8_t x,y;
	pixel_t p={alpha,red,green,blue};
	for(x=0; x<MATRIX_NB_COLUMN;x++)
		for(y=0; y<MATRIX_NB_ROW;y++)
			memcpy(&Layer[nb_layer][y][x], &p , sizeof(pixel_t));

	Make_Layer_composite();

}

void WS2812B::SetBrightness(uint8_t nb_layer,uint8_t brightness)
{
	Brightness[nb_layer]=brightness;
	Make_Layer_composite();
}

void WS2812B::Bitmap(uint8_t nb_layer,int8_t x, int8_t y, uint8_t width, uint8_t height, uint8_t *bitmap)
{
	int8_t i,j;
	pixel_t *p = (pixel_t *)bitmap;

	for(j=y;j<(y+height);j++)
		for(i=x;i<(x+width);i++)
		{
			SetPixelAt(nb_layer,i,j,p->alpha, p->red,p->green,p->blue);
			p++;
		}
}

void WS2812B::SetPixelAt(uint8_t nb_layer,int8_t x, int8_t y, uint8_t alpha,uint8_t red, uint8_t green, uint8_t blue)
{
	if(nb_layer<LAYER_MAX)
	{
		if(Check_Zone(x,y))
		{
			if(alpha>=RGB_FULL_RANGE) alpha=RGB_FULL_RANGE;
			if(red  >=RGB_FULL_RANGE) red  =RGB_FULL_RANGE;
			if(green>=RGB_FULL_RANGE) green=RGB_FULL_RANGE;
			if(blue >=RGB_FULL_RANGE) blue =RGB_FULL_RANGE;

			// setLEDcolor(INDEX_LED(x,y),red, green,blue);
			Layer[nb_layer][y][x].alpha = alpha;
			Layer[nb_layer][y][x].red=red;
			Layer[nb_layer][y][x].green=green;
			Layer[nb_layer][y][x].blue=blue;
			Make_Pixel_Composite(x,y);		
		}
	}
}

void WS2812B::SetPixelAt(uint8_t nb_layer,uint8_t nbled, uint8_t alpha,uint8_t red, uint8_t green, uint8_t blue)
{
	setLEDcolor(nbled,red,green,blue);
}

void WS2812B::setLEDcolor(uint32_t LEDnumber, uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
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


void WS2812B::setWHOLEcolor(uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
	uint32_t index;

	for (index = 0; index < NbLed; index++)
		setLEDcolor(index, RED, GREEN, BLUE);
}

void WS2812B::fillBackground(uint8_t red, uint8_t green, uint8_t blue)
{
	Fill(LAYER_BACKGROUND,ALPHA_SOLID,red,green,blue);
}

void WS2812B::fillBufferBlack(void) {
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

void WS2812B::fillBufferWhite(void) {
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
bool WS2812B::Check_Zone(int8_t x,int8_t y)
{
	if(x<0) 					return false;
	if(y<0) 					return false;
	if(x>(MATRIX_NB_COLUMN-1)) 	return false;
	if(y>(MATRIX_NB_ROW-1))		return false;
	return true;
}

void WS2812B::Make_Layer_composite(void)
{
	for(uint8_t x=0; x<MATRIX_NB_COLUMN;x++)
		for(uint8_t y=0; y<MATRIX_NB_ROW;y++)
		{
			Make_Pixel_Composite(x,y);
		}
}

void WS2812B::Make_Pixel_Composite(int8_t x, int8_t y)
{
	pixel_t *s;
	pixel_t d = { ALPHA_SOLID,0,0,0};	// destination
	if(!((x==2)&&(y==6))) // dead pixel
		for(uint8_t l=LAYER_BACKGROUND;l<LAYER_MAX;l++)
		{
			s=&Layer[l][y][x];
			d.red  = (1-s->alpha/(float)(RGB_FULL_RANGE))*d.red   + (s->alpha/(float)(RGB_FULL_RANGE))*s->red  *Brightness[l]/FULL_BRIGHTNESS_RANGE;
			d.green= (1-s->alpha/(float)(RGB_FULL_RANGE))*d.green + (s->alpha/(float)(RGB_FULL_RANGE))*s->green*Brightness[l]/FULL_BRIGHTNESS_RANGE;
			d.blue = (1-s->alpha/(float)(RGB_FULL_RANGE))*d.blue  + (s->alpha/(float)(RGB_FULL_RANGE))*s->blue *Brightness[l]/FULL_BRIGHTNESS_RANGE;
		}
	setLEDcolor(INDEX_LED(x,y),d.red,d.green,d.blue);
}




void ws2812_update(void) {
	//HAL_TIM_PWM_ConfigChannel(&_TimHandle, &_sConfig, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start_DMA(&_TimHandle, TIM_CHANNEL_1, (uint32_t *) MyTempbuffer,
			MyTempbufsize);
}

#if defined(STM32L476xx)
# include "l476.init"
#elif defined(STM32WB55xx)
# include "wb55.init"
#else
# error "from library ws2812 : MCU not supported"
#endif
