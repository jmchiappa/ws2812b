/*
  Copyright (c) 2015 Arduino LLC. All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

//#if defined(ARDUINO_ARCH_SAMD)

#include <Arduino.h>
#include "Stepper.h"

#include <stm32_def.h>
#include <hw_config.h>
#include <timer.h>
#include <digital_io.h>
#include <clock.h>
#include <analog.h>
#include <variant.h>

#define usToTicks(_us)    (_us)     // converts microseconds to tick
#define ticksToUs(_ticks) (_ticks)  // converts from ticks back to microseconds

#define TRIM_DURATION  2                                   // compensation ticks to trim adjust for digitalWrite delays
#define MIN_PULSE 300
static stepper_t steppers[MAX_STEPPERS];                         // static array of stepper structures
static volatile int8_t timerChannel[_Nbr_16timers ]; // counter for the stepper being pulsed for each timer (or -1 if refresh interval)

uint8_t StepperCount = 0;                                    // the total number of attached steppers

// convenience macros
#define STEPPER_INDEX_TO_TIMER(_stepper_nbr) ((timer16_Sequence_t)(_stepper_nbr / STEPPERS_PER_TIMER))   // returns the timer controlling this stepper
#define STEPPER_INDEX_TO_CHANNEL(_stepper_nbr) (_stepper_nbr % STEPPERS_PER_TIMER)                       // returns the index of the stepper on this timer
#define STEPPER_INDEX(_timer,_channel)  ((_timer*STEPPERS_PER_TIMER) + _channel)                     // macro to access stepper index by timer and channel
#define STEPPER(_timer,_channel)  (steppers[STEPPER_INDEX(_timer,_channel)])                           // macro to access stepper class by timer and channel
#define TIMER_ID(_timer) ((timer_id_e)(_timer))
#define STEPPER_TIMER(_timer_id)  ((timer16_Sequence_t)(_timer_id))

#define TOGGLE(x)   (uint8_t)((~x) & 0x01)

/************ static functions common to all instances ***********************/

// Initialise le timer sans la partie OC
static void TimerStepperInit(stimer_t *obj, uint16_t period)
{
  TIM_HandleTypeDef *handle = &(obj->handle);

  obj->timer = TIMER_STEPPER;

  handle->Instance               = obj->timer;
  handle->Init.Period            = period;
  handle->Init.Prescaler         = (uint32_t)(getTimerClkFreq(obj->timer) / (1000000)) - 1;
  handle->Init.ClockDivision     = 0;
  handle->Init.CounterMode       = TIM_COUNTERMODE_UP;
#if !defined(STM32L0xx) && !defined(STM32L1xx)
  handle->Init.RepetitionCounter = 0;
#endif

  if(HAL_TIM_OC_Init(handle) != HAL_OK) return;

}

// initialise un canal OutputCompare sur le timer SERVO pour l'instant

static void TimerOCStart(stimer_t *obj, uint16_t pulse, uint8_t channel, void (*irqHandle)(stimer_t*, uint32_t))
{
  TIM_OC_InitTypeDef sConfig = {};
  TIM_HandleTypeDef *handle = &(obj->handle);

  obj->timer = TIMER_STEPPER;

  sConfig.OCMode        = TIM_OCMODE_TIMING;
  sConfig.Pulse         = getTimerCounter(obj) + pulse;
  sConfig.OCPolarity    = TIM_OCPOLARITY_HIGH;
  sConfig.OCFastMode    = TIM_OCFAST_DISABLE;
#if !defined(STM32L0xx) && !defined(STM32L1xx)
  sConfig.OCNPolarity   = TIM_OCNPOLARITY_HIGH;
  sConfig.OCIdleState   = TIM_OCIDLESTATE_RESET;
  sConfig.OCNIdleState  = TIM_OCNIDLESTATE_RESET;
#endif

  obj->irqHandleOC = irqHandle;
  HAL_NVIC_SetPriority((IRQn_Type)getTimerIrq(obj->timer), 14, 0);
  HAL_NVIC_EnableIRQ((IRQn_Type)getTimerIrq(obj->timer));

  channel <<=2; 
  if(HAL_TIM_OC_ConfigChannel(handle, &sConfig, channel) != HAL_OK) return;
  if(HAL_TIM_OC_Start_IT(handle, channel) != HAL_OK) return;
}


static void StepperIrqHandle(stimer_t *obj, uint32_t channel)
{
  uint8_t timer_id = obj->idx;
  stepper_t *stp = &STEPPER(STEPPER_TIMER(timer_id),channel);
  uint8_t stp_nb = STEPPER_INDEX(STEPPER_TIMER(timer_id),channel);
    // if(channel>0)
    // {
  // Serial.print(channel);
  // Serial.print(" ");
  // Serial.print(timer_id);
  // Serial.print(" ");
  // Serial.print(stp_nb);
  // Serial.print(" ");
  // Serial.print(stp->Pin.isActive);
  // Serial.print(" ");
  // Serial.print(stp->ticks);
  // Serial.println();
    // }
  if((stp_nb) < StepperCount && stp->Pin.isActive == true)
  {
    uint32_t  time =getTimerCounter(obj);
    // Serial.print(time);
    // Serial.print(" ");
    // Serial.print(stp->Pin.etat);
    // Serial.print(" ");
    digitalWrite(stp->Pin.nbr, stp->Pin.etat);  // écrit l'état courant
    stp->Pin.etat = TOGGLE(stp->Pin.etat);      // inverse le prochain état
    if(stp->ticks>0)
    {
      stp->ticks--;
      if(stp->ticks==0)
      {
        // Serial.println("Fin de rotation");
        stp->Pin.isActive == false; // fin du déplacement
        // TODO : notifier fin de rotation
        HAL_TIM_OC_Stop_IT(&(obj->handle), channel<<2); // Stop IT for this OC channel
        stp->completed=true;
        return;
      }
      uint32_t newtime = (time + stp->period) % DEFAULT_PERIOD;
      setCCRRegister(obj, channel, newtime);
      // Serial.print(newtime);
      // Serial.print(" ");
      // Serial.println(stp->Pin.etat);
      // Serial.print("period " );
      // Serial.println(stp->period);

    }
  }
}

/*
*
* Start SERVO timer at 1kHz
*
*/
static void initISR(stimer_t *obj)
{
  TimerStepperInit(obj, DEFAULT_PERIOD);
}

static void finISR(stimer_t *obj)
{
  TimerPulseDeinit(obj);
}

static boolean isTimerActive(timer16_Sequence_t timer)
{
  // returns true if any stepper is active on this timer
  for(uint8_t channel=0; channel < STEPPERS_PER_TIMER; channel++) {
    if(STEPPER(timer,channel).Pin.isActive == true)
      return true;
  }
  return false;
}

/****************** end of static functions ******************************/

Stepper::Stepper(void)
{
  if (StepperCount < MAX_STEPPERS) {
    this->stepperIndex = StepperCount++;                    // assign a stepper index to this instance
    steppers[this->stepperIndex].ticks = 0;   // store default values
    diviseur = 1; // M1M0=0x00
    steppers[this->stepperIndex].completed = false;
  } else {
    this->stepperIndex = INVALID_STEPPER;  // too many steppers
  }
}

uint8_t Stepper::attach(uint16_t nb_step,uint8_t enaPin, uint8_t stepPin, uint8_t dirPin,uint8_t M0Pin, uint8_t M1Pin)
{
  timer16_Sequence_t timer;

  this->nstep=  nb_step;  // store the number of step per rotation
  this->M0Pin=  M0Pin;
  this->M1Pin=  M1Pin;
  this->DirPin= dirPin;
  this->enaPin= enaPin;

  if (this->stepperIndex < MAX_STEPPERS) {
    pinMode(M0Pin,OUTPUT);
    pinMode(M1Pin,OUTPUT);
    pinMode(dirPin,OUTPUT);
    pinMode(stepPin, OUTPUT);                                   // set stepper pin to output
    pinMode(enaPin,OUTPUT);
    
    digitalWrite(this->enaPin,HIGH); // désactive le driver par défaut
    digitalWrite(stepPin,LOW);
    Setmstep(diviseur);

    steppers[this->stepperIndex].Pin.nbr = stepPin;
    steppers[this->stepperIndex].Pin.isActive = false; // will be activate once move() is called
    steppers[this->stepperIndex].Pin.etat = LOW;

    // initialize the timer if it has not already been initialized
    timer = STEPPER_INDEX_TO_TIMER(stepperIndex);

    if (isTimerActive(timer) == false) {
      _timer.idx = timer;
      initISR(&_timer);   // horloge fixe
    }
    this->ready=true;
  }
  return this->stepperIndex;
}

void Stepper::detach()
{
  timer16_Sequence_t timer;

  steppers[this->stepperIndex].Pin.isActive = false;
  timer = STEPPER_INDEX_TO_TIMER(stepperIndex);
  if(isTimerActive(timer) == false) {
    finISR(&_timer);
  }
}

void Stepper::SetDirPolarity(bool Avance)
{
  this->Avance = Avance;
  this->Recule = !Avance;
}

// indique le nombre de pas à réaliser
// -1 : tourne en permanence
// <0 : inverse le moteur (TODO)
// =0 : arrête le moteur

/*
* Restart command
* restart moves from the last tick
*
*/
void Stepper::Move(void)
{
  this->Move(steppers[this->stepperIndex].ticks>>((this->diviseur)+1));
}

/*
* Move command
* start moving the motor
* n_incr : number of steps to do
*/
void Stepper::Move(int32_t nb_incr)
{
  steppers[this->stepperIndex].completed=false;
  bool direction=this->Avance;
  if(nb_incr==0)
  {
    steppers[this->stepperIndex].completed=true;
    steppers[this->stepperIndex].Pin.isActive = false;
    steppers[this->stepperIndex].ticks = 0;
    return;
  }
  else
  {
    digitalWrite(this->enaPin, LOW);    // Active le moteur
    steppers[this->stepperIndex].Pin.isActive = true;
    if(nb_incr<0)
    {
      nb_incr=abs(nb_incr);
      direction=this->Recule;
    }
    digitalWrite(this->DirPin,(direction&0x01));
    // 1 tick = 1 transition, donc 1 periode ou 1 pas = 2 ticks
    nb_incr = nb_incr*(1<<(this->diviseur)+1); // multiplie le nombre d'incrément par le diviseur et par 2 pour le nombre de transition par pas
    steppers[this->stepperIndex].ticks = nb_incr; // soit >0, soit -1
    TimerOCStart(&_timer, steppers[this->stepperIndex].period, this->stepperIndex, StepperIrqHandle); // active le canal TIM_OC_CHANNEL_x
  }
}
// Arrête immédiatement l'IT de l'OC
void Stepper::Stop(void)
{
  HAL_TIM_OC_Stop_IT(&_timer.handle, this->stepperIndex<<2); // Stop IT for this OC channel
}

void Stepper::Deactivate(void)
{
  digitalWrite(this->enaPin, HIGH);    // désactive le driver du moteur
}

bool Stepper::Completed(void)
{ 
  // bool res= steppers[this->stepperIndex].completed; // store and send the completed status
  // steppers[this->stepperIndex].completed==false;    // reset completed status
  return steppers[this->stepperIndex].completed;
}

bool Stepper::Ready(void)
{ 
  return this->ready;
}


uint8_t Stepper::GetDivider(void)
{
  return diviseur;
}

void Stepper::SetSpeed(uint16_t value)  // tour par minute
{
  uint32_t speed = value * nstep/60; // nombre de pas par seconde
  speed = (2*1e6) / speed;  // demi periode de chaque tick demandé

  // Calcul du nombre de pas indépendamment du diviseur (1, 2, 4 ou 8)
  uint32_t ticks = (steppers[this->stepperIndex].ticks>>this->diviseur+1);
  // maintenant on choisit le meilleur diviseur
  speed =SetDiv(speed);   // calcule le meilleur diviseur et retourne la vitesse à appliquer
  //Recalcule le nombre de pas en fonction du nouveau diviseur
  steppers[this->stepperIndex].ticks = ticks * (1 << this->diviseur+1);
  // inscrit la période dans la structure qui servira à la callback OC
  writeMicroseconds(speed);

}

uint32_t Stepper::SetDiv(uint32_t speed)
{
  byte diviseur=0;
  while((speed>900)&&(diviseur<0x03))
  {
    diviseur++;  // incrémente le diviseur
    speed>>=1; // multiplie par 2 la vitesse
  }
  speed=max(speed,MIN_PULSE); // le pulse minimum est 300µs
  
  this->Setmstep(diviseur);
  return speed;
}

void Stepper::Setmstep(byte ustep)
{
  if((ustep & 0x01)==0x01)
    digitalWrite(this->M0Pin,HIGH);
  else
    digitalWrite(this->M0Pin,LOW);
  if((ustep & 0x02)==0x02 )
    digitalWrite(this->M1Pin,HIGH);
  else
    digitalWrite(this->M1Pin,LOW);

  this->diviseur = ustep;

}
void Stepper::writeMicroseconds(int value)
{
  // calculate and store the values for the given channel
  byte channel = this->stepperIndex;
  if( (channel < MAX_STEPPERS) )   // ensure channel is valid
  {
    value = value - TRIM_DURATION;
    value = usToTicks(value);  // convert to ticks after compensating for interrupt overhead
    steppers[channel].period = value;
  }
}

uint16_t Stepper::read()
{
  uint16_t pulsewidth;
  if (this->stepperIndex != INVALID_STEPPER)
    pulsewidth = ticksToUs(steppers[this->stepperIndex].ticks>>this->diviseur+1);
  else
    pulsewidth  = 0;

  return pulsewidth;
}

int Stepper::readMicroseconds()
{
  unsigned int pulsewidth;
  if (this->stepperIndex != INVALID_STEPPER)
    pulsewidth = ticksToUs(steppers[this->stepperIndex].ticks)  + TRIM_DURATION;
  else
    pulsewidth  = 0;

  return pulsewidth;
}

bool Stepper::attached()
{
  return steppers[this->stepperIndex].Pin.isActive;
}
