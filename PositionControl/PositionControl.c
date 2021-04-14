/*==============================================================================

 Name:           PositionControl.c
 Description:    functionality of camera position mover control driver
 Copyright:      Geierwally, 2021(c)

==============================================================================*/
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <math.h>
#include "PositionControl.h"
#include "PositionDirect.h"
#include "timer.h"
#include "AudioControl.h"
#include "IRcontrol.h"

extern PositionSens PosDir_Gier;
extern PositionSens PosDir_Nick;

PosAngle_Data teachedPositions[22];
PosAngle_Data angleData = {0.0,0.0,0.0,0.0,0.0,0.0}; // calculated nick gier and roll angles

// initialize gyroscope, accelerometer and magnetometer
unsigned char PC_Init(void)
{
	unsigned char retval = PC_SUCCESS;
    // initialize position sensors
	retval = PosDirect_init();
	retval|=PosDirect_setClockWise(&PosDir_Gier,1);
	retval|=PosDirect_setClockWise(&PosDir_Nick,1);

	memset(teachedPositions,0,sizeof(teachedPositions));
	retval |= PC_readDatFile();

	angleData.gier = 0.0;
	angleData.nick = 0.0;
	angleData.roll = 0.0;

	angleData.nickOffset = 0.0;
	angleData.rollOffset = 0.0;
	angleData.gierOffset = teachedPositions[21].gier; // take null position as zero degrees position

	return (retval);
}


// calculate nick and gier angles
unsigned char PC_calc_Angles(void)
{
	unsigned char retval = PC_SUCCESS;
	angleData.gier = 0.0;
	angleData.nick = 0.0;
	angleData.gier = PosDirect_angleR(&PosDir_Gier, U_DEG, 1) - angleData.gierOffset;
	angleData.nick = PosDirect_angleR(&PosDir_Nick, U_DEG, 1);
    if(angleData.gier < 0)
    {
    	angleData.gier += 360;
    }
	//printf("Gier Deg=%f Nick DEG=%f\n",angleData.gier,angleData.nick);
	return(retval );
}


// move to position
unsigned char PC_Move(unsigned char ID)
{
	unsigned char retVal = PC_SUCCESS;
	unsigned char moveDone = 0;
	unsigned char moveUp = 0;
	unsigned char moveRight = 0;
	systemtimer timeoutTimer;
	systemtimer loopTimer;
	startMeasurement(&timeoutTimer);

	// lock remote control switches during auto move
	digitalWrite(PC_OUT_LOCK_REMOTE, 1);
	moveDone = 0;
	moveUp = 0;
	moveRight = 0;

    PC_calc_Angles();
	if(fabs(teachedPositions[ID].nick - angleData.nick)>0.5) // difference upper 0 degrees, start nick movement
	{
		moveDone |= 0x01;
		if(teachedPositions[ID].nick > angleData.nick)
		{
			digitalWrite(PC_OUT_MOVE_NICK_UP, 0);
			digitalWrite(PC_OUT_MOVE_NICK_DOWN, 1);
			moveUp = 1;
			//printf("move up\n");
		}
		else
		{
			digitalWrite(PC_OUT_MOVE_NICK_DOWN, 0);
			digitalWrite(PC_OUT_MOVE_NICK_UP, 1);
			//printf("move down\n");
		}
	}

	if((fabs(teachedPositions[ID].gier - angleData.gier)>0.5)&&(teachedPositions[ID].gier>10)&&(teachedPositions[ID].gier<350)) // difference upper 0 degrees, start gier movement
	{
		moveDone |= 0x02;

		if(teachedPositions[ID].gier > angleData.gier)
		{
			digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 1);
			digitalWrite(PC_OUT_MOVE_GIER_LEFT, 0);
			moveRight = 1;
			//printf("move right\n");
		}
		else
		{
			digitalWrite(PC_OUT_MOVE_GIER_LEFT, 1);
			digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 0);
			//printf("move left\n");
		}
	}

	while((isExpired(60000000,&timeoutTimer)==0)&&(moveDone > 0)) // start move with timeout after 60 seconds
	{
		startMeasurement(&loopTimer);
		PC_calc_Angles();
		if(moveUp)  // check nick movement and switch off if position reached
		{
			if(teachedPositions[ID].nick <= angleData.nick)
			{
				digitalWrite(PC_OUT_MOVE_NICK_DOWN,0);
				moveDone &= 0x02;
			}
		}
		else
		{
			if(teachedPositions[ID].nick >= angleData.nick)
			{
				digitalWrite(PC_OUT_MOVE_NICK_UP,0);
				moveDone &= 0x02;
			}
		}

		if(moveRight) // check gier position and switch off if position reached
		{

			if(teachedPositions[ID].gier <= angleData.gier)
			{
				digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 0);
				moveDone &= 0x01;
			}
		}
		else
		{
			if(teachedPositions[ID].gier >= angleData.gier)
			{
				digitalWrite(PC_OUT_MOVE_GIER_LEFT, 0);
				moveDone &= 0x01;
			}
		}
		//printf("Gier ist =%f grad Nick=%f Gier Soll=%f\n",angleData.gier,angleData.nick,teachedPositions[ID].gier);
		while(isExpired(35000,&loopTimer)==0)
		{}
	}
	// unlock remote control switches after auto move
	digitalWrite(PC_OUT_LOCK_REMOTE, 0);
	return(retVal);
}

// teach position
unsigned char PC_Teach(unsigned char ID)
{
	unsigned char retVal = PC_SUCCESS;
	if(ID == 21) // teach null position, reset offset
	{
		angleData.gierOffset = 0;
	}
	angleData.gier = PosDirect_angleR(&PosDir_Gier, U_DEG, 1) - angleData.gierOffset;
    if(angleData.gier < 0)
    {
    	angleData.gier += 360;
    }
	angleData.nick = PosDirect_angleR(&PosDir_Nick, U_DEG, 1);
	// write angle values into teached positions data file
    teachedPositions[ID].nick = angleData.nick;
    teachedPositions[ID].roll = angleData.roll;
    teachedPositions[ID].gier = angleData.gier;
    //printf("Gier=%f\n",angleData.gier);
    if(ID == 21)
    {   // take null position as zero degree position
    	angleData.gierOffset = teachedPositions[ID].gier;
    }
    retVal = PC_writeDatFile();
	return(retVal);
}


// position control sequence
unsigned char PC_Sequencer(int ID)
{
	unsigned char result = PC_SUCCESS;
	unsigned char position = (unsigned char)(ID & 0xff);        // extract position information
	unsigned char camView =  (unsigned char )((ID>> 8) & 0x0f); // extract camera view during movement
	unsigned char audio =    (unsigned char )((ID>>12) & 0x0f); // extract audio profile

	result = AC_Execute((unsigned char)AUDIO_PROFILE + audio ); // switch audio profile
    //AC_SLIDERSHOW  0x70 // audio profile 1
    //AC_WORSHIP     0x71 // audio profile 2
    //AC_PREACHING   0x72 // audio profile 3
    //AC_TEXT        0x73 // audio profile 4
    //AC_BAND        0x74 // audio profile 5

	if(result == PC_SUCCESS)
	{
		result = IR_SequenceOut(camView);                       // HDMI switch camera view during movement
	}
	if(result == PC_SUCCESS)
	{
		result = PC_Move(position);								// move to position
	}
	if((result == PC_SUCCESS)&&(camView != 2))
	{
		result = IR_SequenceOut(2);                             // HDMI switch camcorder with position control
	}
	return(result);
}

// button pressed \ released function for movement up down left right
unsigned char PC_MoveButton (unsigned char ID)
{
	unsigned char result = PC_SUCCESS;
	if(ID != PC_BUTTON_RELEASED)
	{
		// lock remote control switches on move
		digitalWrite(PC_OUT_LOCK_REMOTE, 1);
	}
	switch(ID)
	{
		case PC_BUTTON_UP:
    		digitalWrite(PC_OUT_MOVE_NICK_DOWN, 0);
    		digitalWrite(PC_OUT_MOVE_NICK_UP, 1);
		break;
		case PC_BUTTON_DOWN:
    		digitalWrite(PC_OUT_MOVE_NICK_UP, 0);
    		digitalWrite(PC_OUT_MOVE_NICK_DOWN, 1);
		break;
		case PC_BUTTON_RIGHT:
    		digitalWrite(PC_OUT_MOVE_GIER_LEFT, 0);
    		digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 1);
		break;
		case PC_BUTTON_LEFT:
    		digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 0);
    		digitalWrite(PC_OUT_MOVE_GIER_LEFT, 1);
		break;
		default:
		case PC_BUTTON_RELEASED:
    		digitalWrite(PC_OUT_MOVE_NICK_UP, 0);
    		digitalWrite(PC_OUT_MOVE_NICK_DOWN, 0);
    		digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 0);
    		digitalWrite(PC_OUT_MOVE_GIER_LEFT, 0);
			// unlock remote control switches after move
			digitalWrite(PC_OUT_LOCK_REMOTE, 0);
		break;
	}
	return(result);
}

// write teached position data into binary file
unsigned char PC_writeDatFile(void)
{
	unsigned char result = PC_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("PC_Dat.bin","wb");  // w for write, b for binary
	if(write_ptr != NULL)
	{
		fwrite(teachedPositions,sizeof(teachedPositions),1,write_ptr); // write 10 bytes from our buffer
		fclose(write_ptr);
	}
	else
	{
		result = PC_WRITE_FAILED;
	}
	return(result);
}

// read position teach data from binary file
unsigned char PC_readDatFile(void)
{
	unsigned char result = PC_SUCCESS;
	FILE *read_ptr;

	read_ptr = fopen("PC_Dat.bin","rb");  // r for read, b for binary
	if(read_ptr != NULL)
	{
		fread(teachedPositions,sizeof(teachedPositions),1,read_ptr); // read 10 bytes to our buffer
		fclose(read_ptr);
	}
	else
	{
		PC_writeDatFile();
		result = PC_READ_FAILED;
	}
	return(result);
}

void PC_Test(void)
{
	systemtimer testTimer;
	unsigned char result = PC_SUCCESS;
	startMeasurement(&testTimer);
	// lock remote control switches on test program
	digitalWrite(PC_OUT_LOCK_REMOTE, 1);

	while(isExpired(300000000,&testTimer)==0) // five minutes test time
	{
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x0000); // Altar position, laptop view,  audioprofile diashow
		}
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[0].gier,angleData.nick,teachedPositions[0].nick);
		wait(1000000);
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x1101); // Taufstein position, GoPro view,  audioprofile Gottesdienst
		}
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[1].gier,angleData.nick,teachedPositions[1].nick);
		wait(1000000);
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x2202); // Kanzel position, Camcorder 1 view,  audioprofile Predigt
		}
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[2].gier,angleData.nick,teachedPositions[2].nick);
		wait(1000000);
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x3303); // Orgel position, Camcorder 2 view,  audioprofile Text
		}
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[3].gier,angleData.nick,teachedPositions[3].nick);
		wait(1000000);
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x4104); // Mittelgang position, Camcorder 1 view,  audioprofile Band
		}
		PC_calc_Angles();
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[4].gier,angleData.nick,teachedPositions[4].nick);
		wait(1000000);
	}
	// unlock remote control switches on test program
	digitalWrite(PC_OUT_LOCK_REMOTE, 0);
}


// moves in loop to first five teached positions
void PC_Test_Pos(void)
{
	systemtimer testTimer;
	unsigned char result = PC_SUCCESS;
	unsigned char positionIndex = 0;
	startMeasurement(&testTimer);
	// lock remote control switches on test program
	digitalWrite(PC_OUT_LOCK_REMOTE, 1);
	while((isExpired(300000000,&testTimer)==0)&&(result == PC_SUCCESS)) // five minutes test time
	{
		result = PC_Move(positionIndex);
		PC_calc_Angles();
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[positionIndex].gier,angleData.nick,teachedPositions[positionIndex].nick);
		if(positionIndex == 4)
		{
			positionIndex = 0;
		}
		else
		{
			positionIndex ++;
		}
		wait(1000000); // wait one second befor move to next position
	}
	// unlock remote control switches on test program
	digitalWrite(PC_OUT_LOCK_REMOTE, 0);
}

// switch off all used gpios
void PC_shutdown(void)
{
	digitalWrite(PC_OUT_MOVE_GIER_LEFT,0);    // relais output signal move gier to left
    digitalWrite(PC_OUT_MOVE_GIER_RIGHT,0);   // relais output signal move gier to right
    digitalWrite(PC_OUT_MOVE_NICK_UP,0);      // relais output signal move nick up
    digitalWrite(PC_OUT_MOVE_NICK_DOWN,0);    // relais output signal move nick down
    digitalWrite(PC_OUT_LOCK_REMOTE,0);       // relais output signal lock remote control
    digitalWrite(PC_OUT_1,0);                 // relais output signal 1
    digitalWrite(PC_OUT_2,0);                 // relais output signal 2
    digitalWrite(PC_OUT_3,0);                 // relais output signal 3
}



