/*=============================================================================

 Name:           system.h 
 Description:    declaration of Raspberry Pi HAL
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#ifndef SYSTEM_H
#define SYSTEM_H
#include "hw.h"
#include "pigpio_adapter.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stddef.h>



#define SERVO_PWM_ZOOM 26 // output channel 0 PWM for Servo

// debug outputs
#define DEB_0 10


int SYSTEM_Initialize (void);
int SYSTEM_GetConfiguredIPAddress(char *ipAddress, size_t ipAddressSize);
void initTimer1(void);
void alarmWakeup(int sig_num);
extern long SYSTEM_map(long x, long in_min, long in_max, long out_min, long out_max);
extern void SYSTEM_PWM(int output, int pulseTime, int breakTime);

#endif
