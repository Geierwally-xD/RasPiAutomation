/*==============================================================================

 Name:           IRcontrol.c
 Description:    functionality of IR remote control driver
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include <wiringPi.h>
#include <stdio.h>

#include <signal.h>
#include <unistd.h>
#include "IRcontrol.h"
#include "system.h"
#include "timer.h"

IR_Data IR_sequences[NUM_SEQ][SEQ_SIZE];


systemtimer timeoutTimer;			// function timeout timer (3 seconds limit)


// initialize IR interface
unsigned char IR_init(void)
{
	unsigned char result = IR_SUCCESS;
	pinMode(IR_OUT, OUTPUT); // IR output signal
	pinMode(IR_IN,  INPUT);  // IR input signal
	digitalWrite(IR_OUT, 0); // reset IR output
	for(int i = 0;i<NUM_SEQ;i++)
	{
		for(int j = 0; j< SEQ_SIZE; j++)
		{
			IR_sequences[i][j].IR_Time = 0;
			IR_sequences[i][j].signal = 0;
		}
	}

	result = IR_readDatFile();
	return(result);

}

// switch IR sequence out
unsigned char IR_SequenceOut(unsigned char seqID)
{
	unsigned char result = IR_SUCCESS;
	systemtimer onSignalTimer;
	systemtimer frequencyTimer;
	int OnSignal = 0;
	unsigned int periodTimeHalf = 500/_FREQUENCY_; //calculate period time
	if(seqID < NUM_SEQ)
	{
		if(IR_sequences[seqID][0].IR_Time > 0)
		{
		  for(int j = 0; j<2; j++)
		  {
			for(int i = 0; i< SEQ_SIZE; i++) // switch IR sequence
			{
				if(IR_sequences[seqID][i].IR_Time > 0)
				{
					if((int) IR_sequences[seqID][i].signal == 0)
					{
						digitalWrite(IR_OUT, (int) IR_sequences[seqID][i].signal);
						wait(IR_sequences[seqID][i].IR_Time);
					}
					else
					{
						startMeasurement(&onSignalTimer); // start on signal timer
						startMeasurement(&frequencyTimer);// start frequency timer

						while(isExpired(IR_sequences[seqID][i].IR_Time,&onSignalTimer)==0)
						{
							if(isExpired(periodTimeHalf,&frequencyTimer)==1)
							{
								OnSignal ^=1;
								digitalWrite(IR_OUT, OnSignal);
								startMeasurement(&frequencyTimer);
							}
						}
					}
				}
				else // end of sequence reached, break loop
				{
					break;
				}
			}
			wait(1000000);
		  }
		}
		else
		{
			result = IR_SEQ_EMPTY;
		}
	}
	else
	{
		result = IR_UNDEFINED;
	}
	digitalWrite(IR_OUT, 0);
	return(result);
}

// teach IR sequence
unsigned char IR_SequenceIn(unsigned char seqID)
{
	unsigned char result = IR_SUCCESS;
	systemtimer teachTimer;
	int prevTeacheData = 1; // previous signal state for detection signal change
	int teachData = 1;
	if(seqID < NUM_SEQ)
	{
		for(int i = 0; i< SEQ_SIZE; i++) // switch IR sequence
		{
			IR_sequences[seqID][i].IR_Time = 0;
			IR_sequences[seqID][i].signal = 0;
		}
		startMeasurement(&timeoutTimer);

		while(teachData == prevTeacheData) // wait for leading rising edge
		{
			teachData = digitalRead(IR_IN);
		    if(isExpired(IR_SCAN_TIMEOUT,&timeoutTimer)==1) // loop timed out ?
		    {
		    	result = IR_TIMEOUT;
		    	break;
		    }
		}
		if(result == IR_SUCCESS) // leading edge detected and no timeout
		{
			startMeasurement(&teachTimer); // restart teach timer
			startMeasurement(&timeoutTimer);// restart timeout timer
			prevTeacheData = teachData;
			for(int i = 0; i< SEQ_SIZE; i++) // teach IR sequence
			{
				while(1)
				{
					teachData = digitalRead(IR_IN);
					if(teachData != prevTeacheData)
					{
						IR_sequences[seqID][i].IR_Time = getRelativeTickCount(&teachTimer); // set measured signal time
						startMeasurement(&teachTimer); // restart teach timer
						startMeasurement(&timeoutTimer);// restart timeout timer
						prevTeacheData = teachData;
						IR_sequences[seqID][i].signal = (unsigned char) teachData;  // set sequence signal level
						break;
					}
					if(isExpired(IR_SEQUENCE_TEACHED,&timeoutTimer)==1) // loop timed out ?
					{
						result = IR_SUCCESS;
						IR_writeDatFile();
						break;
					}
				}
			}
		}
	}
	else
	{
		result = IR_UNDEFINED; // undefined sequence ID
	}
	return(result);
}

// write teached IR data into binary file
unsigned char IR_writeDatFile(void)
{
	unsigned char result = IR_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("IR_Dat.bin","wb");  // w for write, b for binary
	if(write_ptr != NULL)
	{
		fwrite(IR_sequences,sizeof(IR_sequences),1,write_ptr); // write 10 bytes from our buffer
		fclose(write_ptr);
	}
	else
	{
		result = IR_WRITE_FAILED;
	}
	return(result);
}

// read IR teach data from binary file
unsigned char IR_readDatFile(void)
{
	unsigned char result = IR_SUCCESS;
	FILE *read_ptr;

	read_ptr = fopen("IR_Dat.bin","rb");  // r for read, b for binary
	if(read_ptr != NULL)
	{
		fread(IR_sequences,sizeof(IR_sequences),1,read_ptr); // read 10 bytes to our buffer
		fclose(read_ptr);
	}
	else
	{
		result = IR_READ_FAILED;
	}
	return(result);
}


