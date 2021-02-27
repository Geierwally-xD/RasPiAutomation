/*==============================================================================

 Name:           PositionControl.c
 Description:    functionality of camera position control driver
 Copyright:      Geierwally, 2021(c)

==============================================================================*/
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <math.h>
#include "PositionControl.h"
#include "timer.h"
#include "PositionMagnetic.h"
#include "PositionGyro.h"
#include "AudioControl.h"
#include "IRcontrol.h"

PosAngle_Data teachedPositions[22];
PosAngle_Data angleData = {0,0,0,0,0,0}; // calculated nick gier and roll angles
KalmanFilter KalmanRoll;
KalmanFilter KalmanNick;
Vector3d GyroAngle;
systemtimer gyroCycleTimer;

// Kalman Filter function
double kalmanFilter(KalmanFilter kalman, double Angle, double gyroRate, double DT)
{
	double  y, S;
	double K_0, K_1;

	kalman.KFangle += DT * (gyroRate - kalman.bias);

	kalman.P_00 +=  - DT * (kalman.P_10 + kalman.P_01) + kalman.Q_angle * DT;
	kalman.P_01 +=  - DT * kalman.P_11;
	kalman.P_10 +=  - DT * kalman.P_11;
	kalman.P_11 +=  + kalman.Q_gyro * DT;

	y = Angle - kalman.KFangle;
	S = kalman.P_00 + kalman.R_angle;
	K_0 = kalman.P_00 / S;
	K_1 = kalman.P_10 / S;

	kalman.KFangle +=  K_0 * y;
	kalman.bias  +=  K_1 * y;
	kalman.P_00 -= K_0 * kalman.P_00;
	kalman.P_01 -= K_0 * kalman.P_01;
	kalman.P_10 -= K_1 * kalman.P_00;
	kalman.P_11 -= K_1 * kalman.P_01;

	return kalman.KFangle;
}


// initialize gyroscope, accelerometer and magnetometer
unsigned char PC_Init(void)
{
	unsigned char retval = PC_SUCCESS;
	retval |= PC_Gyro_Init();
	retval |= PC_Mag_Init();
	memset(teachedPositions,0,sizeof(teachedPositions));
	retval |= PC_readDatFile();
	angleData.gier = 0;
	angleData.nick = 0;
	angleData.roll = 0;
	angleData.gierOffset = teachedPositions[21].gier; // take null position as zero degrees position
	angleData.nickOffset = 0;
	angleData.rollOffset = 0;
	KalmanRoll.CFangle = 0.0;
	KalmanRoll.KFangle = 0.0;
	KalmanRoll.P_00 = 0.0;
	KalmanRoll.P_01 = 0.0;
	KalmanRoll.P_10 = 0.0;
	KalmanRoll.P_11 = 0.0;
	KalmanRoll.Q_angle = 0.01;
	KalmanRoll.Q_gyro = 0.0003;
	KalmanRoll.R_angle = 0.01;
	KalmanRoll.bias = 0.0;
	KalmanNick.CFangle = 0.0;
	KalmanNick.KFangle = 0.0;
	KalmanNick.P_00 = 0.0;
	KalmanNick.P_01 = 0.0;
	KalmanNick.P_10 = 0.0;
	KalmanNick.P_11 = 0.0;
	KalmanNick.Q_angle = 0.01;
	KalmanNick.Q_gyro = 0.0003;
	KalmanNick.R_angle = 0.01;
	KalmanNick.bias = 0.0;
	return (retval);
}

// calculates roll, nick and gier angles from  sensor raw data of accelerometer, gyroscope and magnetometer
void PC_calcAngleDegrees(void)
{
	Vector3d magRawData;
	Vector3d Angle;
	Vector3d AngleDeg;
	Vector3d kalmanFilterValues;
    unsigned long long gyroCycleTime = getRelativeTickCount(&gyroCycleTimer);
    double elapsedTime = (double)gyroCycleTime / 1000000.0;
	startMeasurement(&gyroCycleTimer);

    // normalize vector data from acceleration sensor
	double length = sqrt(accData.posX * accData.posX + accData.posY * accData.posY + accData.posZ * accData.posZ);
	accData.posX /= length;
	accData.posY /= length;
	accData.posZ /= length;
	accData.posX = -accData.posX;
	accData.posY = -accData.posY;

	// calculate roll and nick angles from acceleration sensor raw data
	Angle.y = asin(accData.posX);
	Angle.x = -asin(accData.posY/cos(Angle.y));
	AngleDeg.y = Angle.y * RAD_TO_DEG;
	AngleDeg.x = Angle.x * RAD_TO_DEG;

	// Kalman Filter for roll and nick angles
	kalmanFilterValues.x = kalmanFilter(KalmanRoll,AngleDeg.x,gyroData.posX, elapsedTime);
	kalmanFilterValues.y = kalmanFilter(KalmanNick,AngleDeg.y,gyroData.posY, elapsedTime);
	//printf ("\033[22;31mkalmanX %7.3f  \033[22;36mkalmanY %7.3f\t\e[m",kalmanFilterValues.x,kalmanFilterValues.y);

    // calculate gyroscope angles
	GyroAngle.x +=gyroData.posX * elapsedTime;
	GyroAngle.y +=gyroData.posY * elapsedTime;
	GyroAngle.z +=gyroData.posZ * elapsedTime;

	// Complementary filter used to combine the accelerometer and gyro values for roll and nick .
	KalmanRoll.CFangle=AA*(KalmanRoll.CFangle+gyroData.posX*elapsedTime) +(1 - AA) * AngleDeg.x;
	KalmanNick.CFangle=AA*(KalmanNick.CFangle+gyroData.posY*elapsedTime) +(1 - AA) * AngleDeg.y;

	//printf ("GyroX  %7.3f \t AccXangle \e[m %7.3f \t \033[22;31mCFangleX %7.3f\033[0m\t GyroY  %7.3f \t AccYangle %7.3f \t \033[22;36mCFangleY %7.3f\t\033[0m\n",GyroAngle.x,AngleDeg.x,KalmanRoll.CFangle,GyroAngle.y,AngleDeg.y,KalmanNick.CFangle);

	// drift correction of gyroscope
	if((GyroAngle.x > 90)||(GyroAngle.x < -90))
	{
		KalmanRoll.CFangle = AngleDeg.x;
		GyroAngle.x = AngleDeg.x;
	}

	if((GyroAngle.y > 90)||(GyroAngle.y < -90))
	{
		KalmanNick.CFangle = AngleDeg.y;
		GyroAngle.y = AngleDeg.y;
	}

	// take roll and nick angles from Kalman filter for further calculations
	Angle.y = KalmanNick.CFangle / RAD_TO_DEG;
	Angle.x = KalmanRoll.CFangle / RAD_TO_DEG;

	// normalize raw data from magnetometer witz calibration values
	magRawData.x = (magData.posX - calMagDat.calXMin) / (calMagDat.calXMax - calMagDat.calXMin) * 2 - 1;
	magRawData.y = (magData.posY - calMagDat.calYMin) / (calMagDat.calYMax - calMagDat.calYMin) * 2 - 1;
	magRawData.z = (magData.posZ - calMagDat.calZMin) / (calMagDat.calZMax - calMagDat.calZMin) * 2 - 1;

	// calculate drift compensation of megnetometer axis with roll and nick angles
	double magXcomp = magRawData.x * cos(Angle.y) + magRawData.z * sin(Angle.y);
	double magYcomp = magRawData.x * sin(Angle.x) * sin(Angle.y) + magRawData.y* cos(Angle.x) - magRawData.z * sin(Angle.x) * cos(Angle.y);

	// calculate gier angle in radians
	Angle.z = atan2(magYcomp,magXcomp);

    // Convert radians to degrees for readability. (180/pi)
    angleData.nick = Angle.y * RAD_TO_DEG;
    angleData.roll = Angle.x * RAD_TO_DEG;
    AngleDeg.z = Angle.z *  RAD_TO_DEG;

    if(AngleDeg.z < 0)
    {
    	AngleDeg.z += 360;
    }
    AngleDeg.z -= angleData.gierOffset;
    if(AngleDeg.z < 0)
    {
    	AngleDeg.z += 360;
    }
    angleData.gier = AngleDeg.z;
}

// calculate nick and gier angles
unsigned char PC_calc_Angles(void)
{
	unsigned char retval = PC_SUCCESS;
	angleData.gier = 0;
	angleData.nick = 0;
	angleData.roll = 0;
	retval = PC_Acc_Read();  // get accelerometer raw data from sensor
	retval = PC_Gyro_Read(); // get gyroscope raw data from sensor
	retval |= PC_Mag_Read(); // get magnetometer  raw data from sensor
	if(retval == PC_SUCCESS)
	{
		PC_calcAngleDegrees();
	}
    return(retval );
}

// calibrate position control
unsigned char PC_Calibrate(void)
{
	unsigned char retVal = PC_SUCCESS;
	retVal = PC_Mag_Calibrate();
	return(retVal);
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
	digitalWrite(PC_OUT_LOCK_LEFT, 1);
	digitalWrite(PC_OUT_LOCK_RIGHT, 1);
	digitalWrite(PC_OUT_LOCK_UP, 1);
	digitalWrite(PC_OUT_LOCK_DOWN, 1);

	PC_calc_Angles();
    if(abs(teachedPositions[ID].nick - angleData.nick)>3) // difference upper 3 degrees, start nick movement
    {
    	moveDone |= 0x01;
    	if(teachedPositions[ID].nick > angleData.nick)
    	{
    		digitalWrite(PC_OUT_MOVE_NICK_UP, 1);
    		digitalWrite(PC_OUT_MOVE_NICK_DOWN, 0);
    		moveUp = 1;
    		printf("move up\n");
    	}
    	else
    	{
    		digitalWrite(PC_OUT_MOVE_NICK_DOWN, 1);
    		digitalWrite(PC_OUT_MOVE_NICK_UP, 0);
    		printf("move down\n");
    	}
    }
    if(abs(teachedPositions[ID].gier - angleData.gier)>3) // difference upper 3 degrees, start gier movement
    {
    	moveDone |= 0x02;
    	if(teachedPositions[ID].gier > angleData.gier)
    	{
    		digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 1);
    		digitalWrite(PC_OUT_MOVE_GIER_LEFT, 0);
    		moveRight = 1;
    		printf("move right\n");
    	}
    	else
    	{
    		digitalWrite(PC_OUT_MOVE_GIER_LEFT, 1);
    		digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 0);
    		printf("move left\n");
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
				digitalWrite(PC_OUT_MOVE_NICK_UP,0);
				moveDone &= 0x02;
			}
		}
		else
		{
			if(teachedPositions[ID].nick >= angleData.nick)
			{
				digitalWrite(PC_OUT_MOVE_NICK_DOWN,0);
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
		printf("Gier=%d grad Nick=%d  Move=%d\n",angleData.gier,angleData.nick,moveDone);
		while(isExpired(20000,&loopTimer)==0) // loop timer 20 ms
		{}
	}

	// lock remote control switches during auto move
	digitalWrite(PC_OUT_LOCK_LEFT, 0);
	digitalWrite(PC_OUT_LOCK_RIGHT, 0);
	digitalWrite(PC_OUT_LOCK_UP, 0);
	digitalWrite(PC_OUT_LOCK_DOWN, 0);
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
	PC_calc_Angles(); // get actual angle values
	// write angle values into teached positions data file
    teachedPositions[ID].nick = angleData.nick;
    teachedPositions[ID].roll = angleData.roll;
    teachedPositions[ID].gier = angleData.gier;
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
	result |= IR_init();
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
		digitalWrite(PC_OUT_LOCK_LEFT, 1);
		digitalWrite(PC_OUT_LOCK_RIGHT, 2);
		digitalWrite(PC_OUT_LOCK_UP, 3);
		digitalWrite(PC_OUT_LOCK_DOWN, 4);
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
			digitalWrite(PC_OUT_LOCK_LEFT, 0);
			digitalWrite(PC_OUT_LOCK_RIGHT, 0);
			digitalWrite(PC_OUT_LOCK_UP, 0);
			digitalWrite(PC_OUT_LOCK_DOWN, 0);
		break;
	}
	return(result);
}

// write teached IR data into binary file
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

// read IR teach data from binary file
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
	startMeasurement(&testTimer);
	while((isExpired(20000000,&testTimer)==0)/*&&(nextStepHelper == nextStep)*/)
	{
		PC_calc_Angles();
		printf("Gier=%d grad Nick=%d  Roll=%d\n",angleData.gier,angleData.nick,angleData.roll);
		wait(20000);
	}
}



