/*==============================================================================

 Name:           PositionControl.c
 Description:    functionality of camera position mover control driver
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
PosAngle_Data angleData = {0.0,0.0,0.0,0.0,0.0,0.0}; // calculated nick gier and roll angles
KalmanFilter KalmanRoll;
KalmanFilter KalmanNick;
KalmanFilter KalmanGier;
Vector3d GyroAngle;
Vector3d ComplementaryFilter;
systemtimer gyroCycleTimer;
unsigned long long gyroCycleTime =0.0;

// Kalman Filter function based on http://blog.tkjelectronics.dk/2012/09/a-practical-approach-to-kalman-filter-and-how-to-implement-it
void kalmanFilter(KalmanFilter * kalman, double Angle, double gyroRate, double DT)
{
    /* Step 1 */
    kalman->rate = gyroRate - kalman->bias;
    kalman->KFangle += DT * kalman->rate;
    // Update estimation error covariance - Project the error covariance ahead
    /* Step 2 */
    kalman->P[0][0] += DT * (DT*kalman->P[1][1] - kalman->P[0][1] - kalman->P[1][0] + kalman->Q_angle);
    kalman->P[0][1] -= DT * kalman->P[1][1];
    kalman->P[1][0] -= DT * kalman->P[1][1];
    kalman->P[1][1] += kalman->Q_bias * DT;

    // Discrete Kalman filter measurement update equations - Measurement Update ("Correct")
    // Calculate Kalman gain - Compute the Kalman gain
    /* Step 4 */
    double S = kalman->P[0][0] + kalman->R_measure; // Estimate error
    /* Step 5 */
    double K[2]; // Kalman gain - This is a 2x1 vector
    K[0] = kalman->P[0][0] / S;
    K[1] = kalman->P[1][0] / S;
    // Calculate angle and bias - Update estimate with measurement zk (newAngle)
    /* Step 3 */
    float y = Angle - kalman->KFangle; // Angle difference
    /* Step 6 */
    kalman->KFangle += K[0] * y;
    kalman->bias += K[1] * y;
    // Calculate estimation error covariance - Update the error covariance
    /* Step 7 */
    double P00_temp = kalman->P[0][0];
    double P01_temp = kalman->P[0][1];
    kalman->P[0][0] -= K[0] * P00_temp;
    kalman->P[0][1] -= K[0] * P01_temp;
    kalman->P[1][0] -= K[1] * P00_temp;
    kalman->P[1][1] -= K[1] * P01_temp;
}


// initialize gyroscope, accelerometer and magnetometer
unsigned char PC_Init(void)
{
	unsigned char retval = PC_SUCCESS;
	retval |= PC_Gyro_Init();
	retval |= PC_Mag_Init();
	memset(teachedPositions,0,sizeof(teachedPositions));
	retval |= PC_readDatFile();
	teachedPositions[21].roll = 0.0; // zero position for synchronization with camera mover
	teachedPositions[21].nick = 0.0;
	teachedPositions[21].gier = 90.0;

	angleData.gier = 0.0;
	angleData.nick = 0.0;
	angleData.roll = 0.0;

	angleData.nickOffset = 0.0;
	angleData.rollOffset = 0.0;
	KalmanRoll.KFangle = 0.0;
	KalmanRoll.Q_angle = 0.001f;
	KalmanRoll.Q_bias = 0.003f;
	KalmanRoll.R_measure = 0.03f;
	KalmanRoll.bias = 0.0f;    // Reset bias
	KalmanRoll.P[0][0] = 0.0f; // Since we assume that the bias is 0 and we know the starting angle, the error covariance matrix is set like so - see: http://en.wikipedia.org/wiki/Kalman_filter#Example_application.2C_technical
	KalmanRoll.P[0][1] = 0.0f;
	KalmanRoll.P[1][0] = 0.0f;
	KalmanRoll.P[1][1] = 0.0f;
	KalmanRoll.bias = 0.0;

	KalmanNick.KFangle = 0.0;
	KalmanNick.Q_angle = 0.001f;
	KalmanNick.Q_bias = 0.003f;
	KalmanNick.R_measure = 0.03f;
	KalmanNick.bias = 0.0f;    // Reset bias
	KalmanNick.P[0][0] = 0.0f; // Since we assume that the bias is 0 and we know the starting angle, the error covariance matrix is set like so - see: http://en.wikipedia.org/wiki/Kalman_filter#Example_application.2C_technical
	KalmanNick.P[0][1] = 0.0f;
	KalmanNick.P[1][0] = 0.0f;
	KalmanNick.P[1][1] = 0.0f;
	KalmanNick.bias = 0.0;

	KalmanGier.KFangle = 0.0;
	KalmanGier.Q_angle = 0.001f;
	KalmanGier.Q_bias = 0.003f;
	KalmanGier.R_measure = 0.03f;
	KalmanGier.bias = 0.0f;    // Reset bias
	KalmanGier.P[0][0] = 0.0f; // Since we assume that the bias is 0 and we know the starting angle, the error covariance matrix is set like so - see: http://en.wikipedia.org/wiki/Kalman_filter#Example_application.2C_technical
	KalmanGier.P[0][1] = 0.0f;
	KalmanGier.P[1][0] = 0.0f;
	KalmanGier.P[1][1] = 0.0f;
	KalmanGier.bias = 0.0;
	PC_calc_Angles(1);
	return (retval);
}

// calculates roll, nick and gier euler angles from sensor raw data of accelerometer, gyroscope and magnetometer
void PC_calcAngleDegrees(unsigned char init)
{
	Vector3d magRawData;
	Vector3d Angle;
	Vector3d AngleDeg;
	double elapsedTime;
	if(init == 0)
	{
		//gyroCycleTime = getRelativeTickCount(&gyroCycleTimer);
		elapsedTime = (double)gyroCycleTime / 1000000.0;
	}
	else
	{
		elapsedTime = 0.0;
	}
    // normalize vector data of acceleration sensor
	double length = sqrt(accData.posX * accData.posX + accData.posY * accData.posY + accData.posZ * accData.posZ);
	accData.posX /= length;
	accData.posY /= length;
	accData.posZ /= length;
	// invert acceleration data for tilt compensation of magnetometer and further calculations
	accData.posX = -accData.posX;
	accData.posY = -accData.posY;

	// calculate roll and nick angles from acceleration sensor raw data
	Angle.y = asin(accData.posX);
	Angle.x = -asin(accData.posY/cos(Angle.y));
	AngleDeg.y = Angle.y * RAD_TO_DEG;
	AngleDeg.x = Angle.x * RAD_TO_DEG;


	if(init == 1)
	{// init values of gyroscope and kalman filter for nick and roll
		//ComplementaryFilter.x = AngleDeg.x;
		KalmanRoll.KFangle = AngleDeg.x;
		GyroAngle.x = AngleDeg.x;
		//ComplementaryFilter.y = AngleDeg.y;
		KalmanNick.KFangle = AngleDeg.y;
		GyroAngle.y = AngleDeg.y;

	}
	else
	{
		// calculate gyroscope angles for nick and roll
		GyroAngle.x +=gyroData.posX * elapsedTime;
		GyroAngle.y +=gyroData.posY * elapsedTime;
		// Kalman Filter for nick and roll angles
		kalmanFilter(&KalmanRoll,AngleDeg.x,gyroData.posX, elapsedTime);
		kalmanFilter(&KalmanNick,AngleDeg.y,gyroData.posY, elapsedTime);
	}

	// kalman filter used instead complementary filter
	//Complementary filter to combine the accelerometer and gyro values for nick and roll
	//ComplementaryFilter.x=AA*(ComplementaryFilter.x+gyroData.posX*elapsedTime) +(1 - AA) * AngleDeg.x;
	//ComplementaryFilter.y=AA*(ComplementaryFilter.y+gyroData.posY*elapsedTime) +(1 - AA) * AngleDeg.y;
	//printf ("Kalman Nick %f  Complementaryfilter Nick %f \n",KalmanNick.KFangle,ComplementaryFilter.y);

	// take roll and nick angles in radians from kalman filter for further calculation of tilt compensation for the gier vector
	Angle.y = KalmanNick.KFangle / RAD_TO_DEG;
	Angle.x = KalmanRoll.KFangle / RAD_TO_DEG;

	// normalize sensor raw data of magnetometer with calibration values
	magRawData.x = (magData.posX - calMagDat.calXMin) / (calMagDat.calXMax - calMagDat.calXMin) * 2 - 1;
	magRawData.y = (magData.posY - calMagDat.calYMin) / (calMagDat.calYMax - calMagDat.calYMin) * 2 - 1;
	magRawData.z = (magData.posZ - calMagDat.calZMin) / (calMagDat.calZMax - calMagDat.calZMin) * 2 - 1;

	// calculate tilt compensation of magnetometer axis with roll and nick angles
	double magXcomp = magRawData.x * cos(Angle.y) + magRawData.z * sin(Angle.y);
	double magYcomp = magRawData.x * sin(Angle.x) * sin(Angle.y) + magRawData.y* cos(Angle.x) - magRawData.z * sin(Angle.x) * cos(Angle.y);


	// calculate gier angle in radians
	Angle.z = atan2(magYcomp,magXcomp);

    // Convert radians to degrees for readability. (180/pi)
    angleData.nick = Angle.y * RAD_TO_DEG; /*********  Euler angle nick final result in degrees ********/
    angleData.roll = Angle.x * RAD_TO_DEG; /*********  Euler angle roll final result in degrees ********/

    // Correct for when signs are reversed.
    if(Angle.z < 0)
    {
    	Angle.z += 2*M_PI;
    }

    // Check for wrap due to addition of declination.
    if(Angle.z > 2*M_PI)
    {
    	Angle.z -= 2*M_PI;
    }

    // Convert radians to degrees for readability.
    AngleDeg.z = Angle.z *  RAD_TO_DEG;

	if(init == 1)
	{   // init values of gyroscope and kalman filter for gier angle
		// ComplementaryFilter.z = AngleDeg.z;
		GyroAngle.z = AngleDeg.z;
		elapsedTime = 0;
		KalmanGier.KFangle = AngleDeg.z;
	}
	else
	{
		// calculate gyroscope angle gier
		GyroAngle.z -=gyroData.posZ * elapsedTime;
		// Kalman Filter for gier angle
		kalmanFilter(&KalmanGier,AngleDeg.z,-gyroData.posZ, elapsedTime);
	}

	// kalman filter used instead complementary filter
	// Complementary filter to combine the magnetometer and gyro values for gier.
	// ComplementaryFilter.z=AA*(ComplementaryFilter.z-gyroData.posZ*elapsedTime) +(1 - AA) * AngleDeg.z;
 printf ("Gyr Gier  %f Mag Gier  %f kalman Gier %f\n",GyroAngle.z, AngleDeg.z, KalmanGier.KFangle);
	// Euler angle gier
	AngleDeg.z = KalmanGier.KFangle;
    angleData.gier = AngleDeg.z; /********* Euler angle gier final result in degrees ********/
}

// calculate nick and gier angles
unsigned char PC_calc_Angles(unsigned char init)
{
	unsigned char retval = PC_SUCCESS;
	angleData.gier = 0.0;
	angleData.nick = 0.0;
	angleData.roll = 0.0;
	retval = PC_Acc_Read();  // get accelerometer raw data from sensor
    gyroCycleTime = getRelativeTickCount(&gyroCycleTimer);
	retval = PC_Gyro_Read(); // get gyroscope raw data from sensor
	startMeasurement(&gyroCycleTimer);
	retval |= PC_Mag_Read(); // get magnetometer  raw data from sensor
	if(retval == PC_SUCCESS)
	{
		PC_calcAngleDegrees(init);
	}
    return(retval );
}

// calibrate position control magnetometer or gyroscope depending parameter comp
unsigned char PC_Calibrate(unsigned char comp)
{
	unsigned char retVal = PC_SUCCESS;
	if(comp == Magnetometer)
	{
		// lock remote control switches on calibration
		digitalWrite(PC_OUT_LOCK_REMOTE, 1);
		retVal = PC_Mag_Calibrate();
		// unlock remote control switches
		digitalWrite(PC_OUT_LOCK_REMOTE, 0);
	}
	else if(comp == Gyroscope)
	{
		retVal = PC_Gyr_Calibrate();
	}
	else
	{
		retVal = PC_CALIB_FAILED;
	}
	return(retVal);
}

// move switch with change direction in 2 second cycle for calibration of magnetometer
void PC_Cal_switchMove(int direction)
{
	if(direction == 0)
	{
		digitalWrite(PC_OUT_MOVE_NICK_DOWN, 0);
		digitalWrite(PC_OUT_MOVE_NICK_UP, 1);
		digitalWrite(PC_OUT_MOVE_GIER_LEFT, 0);
		digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 1);
	}
	else
	{
		digitalWrite(PC_OUT_MOVE_NICK_UP, 0);
		digitalWrite(PC_OUT_MOVE_NICK_DOWN, 1);
		digitalWrite(PC_OUT_MOVE_GIER_RIGHT, 0);
		digitalWrite(PC_OUT_MOVE_GIER_LEFT, 1);
	}
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
	PC_calc_Angles(1);
	for(int i = 0; i<2; i++)
	{
		if(i == 0)
		{
			if(fabs(teachedPositions[ID].nick - angleData.nick)>0) // difference upper 0 degrees, start nick movement
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
		}
		if(i == 1)
		{
			wait(500000); // wait 500ms after finished moved nick before moving gier axis
			if((fabs(teachedPositions[ID].gier - angleData.gier)>0)&&(teachedPositions[ID].gier>10)&&(teachedPositions[ID].gier<350)) // difference upper 0 degrees, start gier movement
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
		}

		while((isExpired(60000000,&timeoutTimer)==0)&&(moveDone > 0)) // start move with timeout after 60 seconds
		{
			startMeasurement(&loopTimer);
			PC_calc_Angles(0);
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

			PC_calc_Angles(0);
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
		//	printf("Gier=%f grad Nick=%f  Move=%d\n",angleData.gier,angleData.nick,moveDone);
		}
	}
	// unlock remote control switches after auto move
	digitalWrite(PC_OUT_LOCK_REMOTE, 0);
	return(retVal);
}

// teach position
unsigned char PC_Teach(unsigned char ID)
{
	unsigned char retVal = PC_SUCCESS;
	PC_calc_Angles(1); // get actual angle values
	wait(10000);
	PC_calc_Angles(0); // get actual angle values
	// write angle values into teached positions data file
    teachedPositions[ID].nick = angleData.nick;
    teachedPositions[ID].roll = angleData.roll;
    teachedPositions[ID].gier = angleData.gier;
    printf("Gier=%f\n",angleData.gier);
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
		    result = PC_Sequencer(0x000000); // Altar position, laptop view,  audioprofile diashow
		}
		PC_calc_Angles(0);
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[0].gier,angleData.nick,teachedPositions[0].nick);
		wait(1000000);
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x010101); // Taufstein position, GoPro view,  audioprofile Gottesdienst
		}
		PC_calc_Angles(0);
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[1].gier,angleData.nick,teachedPositions[1].nick);
		wait(1000000);
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x020202); // Kanzel position, Camcorder 1 view,  audioprofile Predigt
		}
		PC_calc_Angles(0);
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[2].gier,angleData.nick,teachedPositions[2].nick);
		wait(1000000);
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x030303); // Orgel position, Camcorder 2 view,  audioprofile Text
		}
		PC_calc_Angles(0);
		//printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[3].gier,angleData.nick,teachedPositions[3].nick);
		wait(1000000);
		if(result == PC_SUCCESS)
		{
		    result = PC_Sequencer(0x040104); // Mittelgang position, Camcorder 1 view,  audioprofile Band
		}
		PC_calc_Angles(0);
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
		PC_calc_Angles(0);
		printf("Gier Ist=%f grad Soll=%f **** Nick Ist=%f  Soll=%f\n",angleData.gier,teachedPositions[positionIndex].gier,angleData.nick,teachedPositions[positionIndex].nick);
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



