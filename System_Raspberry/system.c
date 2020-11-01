/*==============================================================================

 Name:           system.c
 Description:    system HAL for Raspberry Pi
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include <sched.h>
#include <string.h>
#include "system.h"

  struct sched_param sp;

  void SYSTEM_Initialize (void)
  {
	int prio = 0;
	wiringPiSetup();
	// set highest priority (application gets one kernel of the RasPi and runs nearly in real time)
	prio = piHiPri(99);
	memset( &sp, 0, sizeof(sp) );
	sp.sched_priority = 99;
	sched_setscheduler( 0, SCHED_FIFO, &sp );

	pinMode(LED_0, OUTPUT); // LED channel 0
	pinMode(LED_1, OUTPUT); // LED channel 1
	pinMode(LED_2, OUTPUT); // LED channel 2
	pinMode(LED_3, OUTPUT); // LED channel 3
	pinMode(LED_4, OUTPUT); // LED channel 4
	pinMode(LED_5, OUTPUT); // LED channel 5
	pinMode(LED_6, OUTPUT); // LED channel 6
	pinMode(LED_7, OUTPUT); // LED channel 7

	pinMode(DEB_0, OUTPUT); // debug output 0

	//switch off all LEDs
	digitalWrite(LED_0, 0);
	digitalWrite(LED_1, 0);
	digitalWrite(LED_2, 0);
	digitalWrite(LED_3, 0);
	digitalWrite(LED_4, 0);
	digitalWrite(LED_5, 0);
	digitalWrite(LED_6, 0);
	digitalWrite(LED_7, 0);

	//switch off debug output
	digitalWrite(DEB_0, 0);
  }

  
