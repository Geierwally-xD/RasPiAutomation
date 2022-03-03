/*==============================================================================

 Name:           system.c
 Description:    system HAL for Raspberry Pi
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include <sched.h>
#include <string.h>
#include "system.h"

  struct sched_param sp;

  long SYSTEM_map(long x, long in_min, long in_max, long out_min, long out_max)
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }

  void SYSTEM_Initialize (void)
  {
	int prio = 0;
	wiringPiSetup();
	// set highest priority (application gets one kernel of the RasPi and runs nearly in real time)
	prio = piHiPri(99);
	memset( &sp, 0, sizeof(sp) );
	sp.sched_priority = 99;
	sched_setscheduler( 0, SCHED_FIFO, &sp );
	pinMode(DEB_0, OUTPUT); // debug output 0
	//switch off debug output
	digitalWrite(DEB_0, 0);
  }

  void SYSTEM_PWM(int output, int pulseTime, int breakTime)
   {
 	  if(output == SERVO_PWM_ZOOM)
 	  {
 		  pulseTime /= 10;
 		  breakTime /= 10;
 		  pinMode(output, PWM_OUTPUT);
 		  // set the PWM mode to Mark Space
 		  pwmSetMode(PWM_MODE_MS);
 		  // set the clock divisor to reduce the 19.2 Mhz clock
 		  // to something slower, 5 Khz.
 		  // Range of pwmSetClock() is 2 to 4095.
 		  pwmSetClock (192);  // 19.2 Mhz divided by 192 is 1000 Khz.
 		  // set the PWM range which is the value for the range counter
 		  // which is decremented at the modified clock frequency.
 		  // times per second since the clock at 19.2 Mhz is being
 		  // divided by 192 to give us 100 Khz.
 		  pwmSetRange (pulseTime + breakTime);  // range is  counts to give us half second.
 		  pwmWrite (output, pulseTime);  // set the Duty Cycle for this range.
 	  }
   }
