/*==============================================================================

 Name:           timer.h 
 Description:    declaration of system timer functionality with 1 us clock
                 for 64 bit Raspberry Pi
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#ifndef TIMER_H
#define TIMER_H
typedef struct
{
	unsigned long long startTime_;
}systemtimer;

extern void initSystemTimer(void);
extern unsigned char isExpired(unsigned long long timeout,systemtimer * pTimer);
extern unsigned long long getAbsoluteTickCount(systemtimer * pTimer);
extern unsigned long long getRelativeTickCount(systemtimer * pTimer);
extern void startMeasurement(systemtimer * pTimer);
extern void wait(unsigned long long timeout);


#endif
