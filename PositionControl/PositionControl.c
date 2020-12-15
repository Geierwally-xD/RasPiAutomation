/*==============================================================================

 Name:           PositionControl.c
 Description:    functionality of camera position control driver
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include "PositionControl.h"
#include "timer.h"
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <math.h>


void PC_Init(void)
{
	pinMode(PC_OUT_5, OUTPUT); // PC relais output signal
	pinMode(PC_IN_1,  INPUT);  // PC input signal
	pinMode(PC_IN_2,  INPUT);  // PC input signal
}


void  readSensorData (void)
{
}


void PC_Test (void)
{
	systemtimer offsetTimer;
	unsigned long long maxOffset = 0;
	unsigned long long offset = 0;
	int signalState_1 = 0; // previous signal state for detection signal change input 1
	int signalState_2 = 0; // previous signal state for detection signal change input 2
	int state = 0;
	FILE *write_ptr;
	while(1)
	{
		signalState_1 = digitalRead(PC_IN_1);
		signalState_2 = digitalRead(PC_IN_2);
		switch(state)
		{
			case 0:
				if((signalState_1 == 1)&&(signalState_2 == 1))
				{
					state = 1;
				}
			break;
			case 1:
				if(signalState_1 != signalState_2)
				{
					startMeasurement(&offsetTimer);
					digitalWrite(PC_OUT_5, 1);
					state = 2;
				}
			break;
			case 2:
				if((signalState_1 == 0)&&(signalState_2 == 0))
				{
					digitalWrite(PC_OUT_5, 0);
					offset = getRelativeTickCount(&offsetTimer); // set measured signal time
					if(maxOffset < offset)
					{
						maxOffset = offset;
						printf("\n max Offset = %d \n",maxOffset);
						write_ptr = fopen("PC_Offset_Dat.bin","wb");  // w for write, b for binary
						if(write_ptr != NULL)
						{
							fwrite(&maxOffset,sizeof(maxOffset),1,write_ptr);
							fclose(write_ptr);
						}
					}
					state = 0;
				}
			break;
		}




	}

}
