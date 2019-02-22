/*
  Servo.h - Interrupt driven Servo library for Arduino using 16 bit timers- Version 2
  Copyright (c) 2009 Michael Margolis.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
  A servo is activated by creating an instance of the Servo class passing
  the desired pin to the attach() method.
  The servos are pulsed in the background using the value most recently
  written using the write() method.

  Note that analogWrite of PWM on pins associated with the timer are
  disabled when the first servo is attached.
  Timers are seized as needed in groups of 12 servos - 24 servos use two
  timers, 48 servos will use four.
  The sequence used to sieze timers is defined in timers.h
 */

#ifndef Stripws2812b_h
#define Stripws2812b_h

#include <inttypes.h>
#include "ws2812b.h"

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue
} pixel_t;

class WS2812B
{
public:
  WS2812B(void);      // constructor
  Clear(void);        // fill the screen with black color
  Rectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8 red, uint8_t green, uint8_t blue);
  Fill(uint8 red, uint8_t green, uint8_t blue);
  Bitmap(uint8_t x, uint8_t y,uint8_t width, uint8_t height, uint8_t *bitmap);
  SetPixelAt(uint8_t x, uint8_t y, uint8 red, uint8_t green, uint8_t blue);

private:
  
  uint32_t SetDiv(uint32_t speed);
  void Setmstep(byte ustep);
  bool Avance=true;
  bool Recule=false;

  uint8_t stepperIndex;               // index into the channel data for this servo
  stimer_t _timer;
  uint16_t nstep;
  uint8_t M0Pin;
  uint8_t M1Pin;
  uint8_t DirPin;
  uint8_t enaPin;
  uint8_t diviseur=DIV_FULL;     // store the driver divider value [M1M0]
  uint32_t speed=0;
  bool ready=false;
};

#endif
