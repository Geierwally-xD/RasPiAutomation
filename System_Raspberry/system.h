/*=============================================================================

 Name:           system.h 
 Description:    declaration of Raspberry Pi HAL
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#ifndef SYSTEM_H
#define SYSTEM_H
#include "hw.h"
#include <signal.h>
#include <wiringPiI2C.h>
#include <wiringPiSPI.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#define LED_0 21 //LED channel 0
#define LED_1 22 //LED channel 1
#define LED_2 23 //LED channel 2
#define LED_3 24 //LED channel 3
#define LED_4 25 //LED channel 4
#define LED_5 27 //LED channel 5
#define LED_6 28 //LED channel 6
#define LED_7 29 //LED channel 7


// debug outputs
#define DEB_0 10


void SYSTEM_Initialize (void);
void initTimer1(void);
void alarmWakeup(int sig_num);

#endif
