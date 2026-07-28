/*==============================================================================

 Name:           TestProg.c
 Description:    functionality of test application
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include "TestProg.h"

#include "timer.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "pigpio_adapter.h"


void TP_Init(void)
{
	RPI_SetMode(TP_OUT_5, RPI_MODE_OUTPUT); // test output signal
	RPI_SetMode(TP_IN_1,  RPI_MODE_INPUT);  // test input signal
	RPI_SetMode(TP_IN_2,  RPI_MODE_INPUT);  // test input signal
}




void TP_Test (void)
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
		signalState_1 = RPI_Read(TP_IN_1);
		signalState_2 = RPI_Read(TP_IN_2);
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
					RPI_Write(TP_OUT_5, 1);
					state = 2;
				}
			break;
			case 2:
				if((signalState_1 == 0)&&(signalState_2 == 0))
				{
					RPI_Write(TP_OUT_5, 0);
					offset = getRelativeTickCount(&offsetTimer); // set measured signal time
					if(maxOffset < offset)
					{
						maxOffset = offset;
						printf("\n max Offset = %d \n",(int)(maxOffset));
						write_ptr = fopen("/home/pi/JokiAutomation/config/PC_Offset_Dat.bin","wb");  // w for write, b for binary
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
