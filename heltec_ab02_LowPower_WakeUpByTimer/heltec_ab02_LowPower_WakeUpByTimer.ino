/*
L. Bosio
4/12/2024 
aggiunto istruzioni per spegnere servizi non richiesti durante il deep sleep
*/

#include "Arduino.h"
#include "LoRa_APP.h"

#define timetillsleep 5000
#define timetillwakeup 5000
static TimerEvent_t sleep;
static TimerEvent_t wakeUp;
uint8_t lowpower=1;

void onSleep()
{
  Serial.printf("Going into lowpower mode, %d ms later wake up.\r\n",timetillwakeup);
  lowpower=1;
  //timetillwakeup ms later wake up;
//INIZIO: LB, 4/12/24
  digitalWrite(Vext, HIGH);    // Turn off the external power before sleeping
  Radio.Sleep( );
  //display.stop();
//detachInterrupt(RADIO_DIO_1);
  turnOffRGB();
  pinMode(Vext,ANALOG);
  pinMode(ADC,ANALOG);
//FINE

  TimerSetValue( &wakeUp, timetillwakeup );
  TimerStart( &wakeUp );
}
void onWakeUp()
{
  Serial.printf("Woke up, %d ms later into lowpower mode.\r\n",timetillsleep);
  lowpower=0;
  //timetillsleep ms later into lowpower mode;
  TimerSetValue( &sleep, timetillsleep );
  TimerStart( &sleep );
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Radio.Sleep( );
  TimerInit( &sleep, onSleep );
  TimerInit( &wakeUp, onWakeUp );
  onSleep();
}

void loop() {
  if(lowpower){
    lowPowerHandler();
  }
  // put your main code here, to run repeatedly:
}
