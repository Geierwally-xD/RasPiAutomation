/*==============================================================================

 Name:           system.c
 Description:    system HAL for Raspberry Pi
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include <sched.h>
#include <string.h>
#include <fcntl.h>
#include "system.h"

  struct sched_param sp;

  long SYSTEM_map(long x, long in_min, long in_max, long out_min, long out_max)
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }

  int SYSTEM_Initialize (void)
  {
	if (RPI_Initialize() < 0)
	{
		fprintf(stderr, "pigpio initialization failed: %s\n", RPI_LastError());
		return -1;
	}
	atexit(RPI_Terminate);

	// set highest priority (application gets one kernel of the RasPi and runs nearly in real time)
	memset( &sp, 0, sizeof(sp) );
	sp.sched_priority = 99;
	if (sched_setscheduler(0, SCHED_FIFO, &sp) < 0)
	{
		fprintf(stderr, "Warning: failed to elevate scheduler priority: %s\n", strerror(errno));
	}
	RPI_SetMode(DEB_0, RPI_MODE_OUTPUT); // debug output 0
	//switch off debug output
	RPI_Write(DEB_0, 0);

	RPI_SetMode(SERVO_PWM_ZOOM, RPI_MODE_OUTPUT);
	return 0;
  }

  void SYSTEM_PWM(int output, int pulseTime, int breakTime)
   {
	  (void)breakTime;
	  RPI_ServoPulse(output, (unsigned)pulseTime);
   }
