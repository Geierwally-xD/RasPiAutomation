/*==============================================================================

 Name:           ZoomControl.c
 Description:    functionality of time controlled camcorder zoom driver with
                 PWM servo
 Copyright:      Geierwally, 2022(c)

==============================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ZoomControl.h"
#include "timer.h"
#include "system.h"

uint8_t _AZ_ZoomValue[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
AZ_CONFIG _AZ_Config;
systemtimer _AZ_Move_Timer;
uint64_t  _AZ_Move_Timeout;
uint8_t   _AZ_Next_Position;  // next zoom value

//init values of zoom control
uint8_t ZoomControl_init(void)
{
	uint8_t retVal = AZ_SUCCESS;
	memset(_AZ_ZoomValue,0, sizeof(_AZ_ZoomValue));
	retVal |= ZoomControl_readConfFile(&_AZ_Config);
	retVal |= ZoomControl_readDatFile(&_AZ_ZoomValue[0]);
	_AZ_Move_Timeout = 0ul;
	_AZ_Next_Position = 0;
	retVal |= ZoomControl_MoveServo(AZ_MIDDLE);
	return(retVal);
}


// read zoom configuration from configuration file
uint8_t ZoomControl_readConfFile(AZ_CONFIG * config)
{
	uint8_t result = AZ_SUCCESS;
	FILE *read_ptr;

	read_ptr = fopen("zoomconfig.bin","rb");  // r for read, b for binary
	if(read_ptr != 0)
	{
		fread(config,sizeof(_AZ_Config),1,read_ptr); // read dat file
		fclose(read_ptr);
	}
	else
	{
		result = AZ_READ_FAILED;
	}
	return(result);
}


// read zoom positions from data file
uint8_t ZoomControl_readDatFile(uint8_t * data)
{
	uint8_t result = AZ_SUCCESS;
	FILE *read_ptr;

	read_ptr = fopen("zoomValues.bin","rb");  // r for read, b for binary
	if(read_ptr != 0)
	{
		fread(data,sizeof(_AZ_ZoomValue),1,read_ptr); // read dat file
		fclose(read_ptr);
	}
	else
	{
		result = AZ_READ_FAILED;
	}
	return(result);
}


// write teached zoom data into binary file
uint8_t ZoomControlwriteDatFile(void)
{
	uint8_t result = AZ_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("zoomValues.bin","wb");  // w for write, b for binary
	if(write_ptr != NULL)
	{
		fwrite(&_AZ_ZoomValue[0],sizeof(_AZ_ZoomValue),1,write_ptr); // write 21 bytes from our buffer
		fclose(write_ptr);
	}
	else
	{
		result = AZ_WRITE_FAILED;
	}
	return(result);
}

// move auto zoom to position
uint8_t ZoomControl_MoveToPos(uint8_t Position)
{
	uint8_t retVal = AZ_SUCCESS;
	unsigned char zoomState = AZ_TASK_INITIALIZE;
	_AZ_Next_Position = Position;               // set next zoom position
	while(zoomState > AZ_TASK_INITIALIZE)
	{
		zoomState = ZoomControl_Task(zoomState);// move zoom
	}
	retVal = ZoomControlwriteDatFile(); // write actual zoom position
	return(retVal);
}

// calibrate auto zoom
uint8_t ZoomControl_Calibrate(void)
{
	uint8_t retVal = AZ_SUCCESS;
	retVal |= ZoomControl_MoveToPos(0); // move to reference position
	wait(500000);
	startMeasurement(&_AZ_Move_Timer);  // start timer
	ZoomControl_MoveServo(AZ_CON_RIGHT);// start servo move
	if(isExpired(_AZ_Config.CalibrationTime,&_AZ_Move_Timer)==1)
	{
		ZoomControl_MoveServo(AZ_MIDDLE);     // move servo to middle position
	}
	return(retVal);
}

// move to first five positions in a loop
uint8_t ZoomControl_TestPositions(void)
{
	uint8_t retVal = AZ_SUCCESS;
	retVal |= ZoomControl_MoveToPos(100); // move to reference position
	wait(500000);
	for(uint8_t i= 0; i< 5; i++)
	{
		for(uint8_t j= 0; j< 5; j++)
		{
			retVal |= ZoomControl_MoveToPos(_AZ_ZoomValue[j]);
			wait(500000);
		}
	}
	return(retVal);
}

// move servo to position
uint8_t ZoomControl_MoveServo(uint8_t index)
{
	uint8_t retVal = AZ_SUCCESS;
	int Pulse = _AZ_Config.ServoMiddle;
	int Break = AZ_SERVO_PERIOD;
	switch(index)
	{
	    case AZ_REF_RIGHT:
	    	Pulse += _AZ_Config.ServoReference;
	    break;
	    case AZ_REF_LEFT:
	    	Pulse -= _AZ_Config.ServoReference;
		break;
		case AZ_CON_RIGHT:
	    	Pulse += _AZ_Config.ServoControl;
		break;
		case AZ_CON_LEFT:
	    	Pulse -= _AZ_Config.ServoControl;
		break;
		default:
		break;
	}
	Break -= Pulse;
	SYSTEM_PWM(SERVO_PWM_ZOOM,Pulse, Break);
	return(retVal);
}

// zoom control move task move to position
uint8_t ZoomControl_Task(uint8_t State)
{
	uint8_t servoMove = AZ_CON_LEFT;
	switch(State)
	{
		case AZ_TASK_IDLE: // nothing to do wait for start
		break;
		case AZ_TASK_INITIALIZE: // calculate position, start timer and start movement
			if((_AZ_Next_Position)==(_AZ_ZoomValue[20]))
			{  // next position is equal to previous position, nothing to do
				State = AZ_TASK_IDLE;
				break;
			}
			_AZ_Move_Timeout = 2000000; // move timeout reference points 2 sec
			switch(_AZ_Next_Position)
			{
				case 100:
					servoMove = AZ_REF_RIGHT; // reference 100
				break;
				case 0:
					servoMove = AZ_REF_LEFT;  // reference 0
				break;
				default:
					if((_AZ_Next_Position)>(_AZ_ZoomValue[20]))
					{
						servoMove = AZ_CON_RIGHT; // next value > last value move to right
					}
					// calculate move timeout
					_AZ_Move_Timeout = abs(_AZ_Next_Position - _AZ_ZoomValue[20] ) * _AZ_Config.CalibrationTime / 80;
				break;
			}
			ZoomControl_MoveServo(servoMove);
			startMeasurement(&_AZ_Move_Timer);
			State = AZ_TASK_MOVE;
		break;
		case AZ_TASK_MOVE:
			if(isExpired(_AZ_Move_Timeout,&_AZ_Move_Timer)==1)
			{
				ZoomControl_MoveServo(AZ_MIDDLE);     // move servo to middle position
				State = AZ_TASK_IDLE;
				_AZ_ZoomValue[20] = _AZ_Next_Position;// set last position value to next position value
			}
		break;
		default: // undefined state
			ZoomControl_MoveServo(AZ_MIDDLE); // move servo to middle position
			State = AZ_TASK_IDLE;
		break;

	}
	return(State);
}

// switch off PWM output servo pulse
void ZoomControl_shutdown(void)
{
	pinMode(SERVO_PWM_ZOOM, OUTPUT);
	digitalWrite(SERVO_PWM_ZOOM, 0);
}
