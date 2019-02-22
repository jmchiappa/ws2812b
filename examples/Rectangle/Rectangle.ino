#include <ws2812.h>

WS2812B myStripLed;

uint8_t color=0;
void setup()
{
	Serial.begin(9600);
	myStripLed.begin();
}

void loop()
{
	Serial.println(color);
	color++;
	myStripLed.Rectangle(random(7),random(7),2,2,color,color,color);
}