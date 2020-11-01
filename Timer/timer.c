/*==============================================================================

 Name:           timer.c 
 Description:    system software timer functionality with 1 us clock for 64 bit Rspberry Pi
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include <sys/time.h>
#include "timer.h"
volatile unsigned long long systemTime = 0;
systemtimer waitTimer;


void initSystemTimer(void)
{
	struct timeval tv;
	gettimeofday(&tv,0);
	systemTime = (1000000*tv.tv_sec) + tv.tv_usec;
}
// presets start time of software timer
void startMeasurement(systemtimer * pTimer)
{
	struct timeval tv;
	gettimeofday(&tv,0);
	systemTime = (1000000*tv.tv_sec) + tv.tv_usec;
	pTimer->startTime_ = systemTime;
};
// checks if software timer is expired (timeout reached)returns 1 if expired otherwise 0
unsigned char isExpired(unsigned long long timeout,systemtimer * pTimer)
{
	struct timeval tv;
    unsigned char retVal = 0;
    unsigned long long curTime = 0;
	gettimeofday(&tv,0);
	systemTime = (1000000*tv.tv_sec) + tv.tv_usec;
	curTime = systemTime;
	if(((curTime - pTimer->startTime_)) >= timeout)
	{
		retVal = 1;
	}
    return(retVal);
};

// checks if software timer is expired (timeout reached)returns time ticks since start
unsigned long long getAbsoluteTickCount(systemtimer * pTimer)
{
	struct timeval tv;
    unsigned long long retVal = 0;
	gettimeofday(&tv,0);
	systemTime = (1000000*tv.tv_sec) + tv.tv_usec;
	retVal = systemTime;
    return(retVal);
};
// returns time ticks since start
unsigned long long getRelativeTickCount(systemtimer * pTimer)
{
	struct timeval tv;
    unsigned long long retVal = 0;
    unsigned long long curTime = 0;
	gettimeofday(&tv,0);
	systemTime = (1000000*tv.tv_sec) + tv.tv_usec;
	curTime = systemTime;
	retVal = (curTime - pTimer->startTime_);
    return(retVal);
}
// loops until timeout
void wait(unsigned long long timeout)
{
	struct timeval tv;
    unsigned long long curTime = 0;
    startMeasurement(&waitTimer);

	do
	{
		gettimeofday(&tv,0);
		systemTime = (1000000*tv.tv_sec) + tv.tv_usec;
		curTime = systemTime;
	}
	while((curTime - waitTimer.startTime_) < timeout);
}
