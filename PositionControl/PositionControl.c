/*==============================================================================

 Name:           PositionControl.c
 Description:    functionality of camera position control driver
 Copyright:      Geierwally, 2021(c)

==============================================================================*/
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <math.h>
#include "PositionControl.h"
#include "timer.h"
#include "PositionMagnetic.h"
#include "PositionGyro.h"

PosAngle_Data angleData = {0,0,0,0,0,0}; // calculated nick gier and roll angles

//Used by Kalman Filters
double Q_angle  =  0.01;
double Q_gyro   =  0.0003;
double R_angle  =  0.01;
double x_bias = 0;
double y_bias = 0;
double XP_00 = 0, XP_01 = 0, XP_10 = 0, XP_11 = 0;
double YP_00 = 0, YP_01 = 0, YP_10 = 0, YP_11 = 0;
double KFangleX = 0.0;
double KFangleY = 0.0;
double gyroXangle = 0.0;
double gyroYangle = 0.0;
double gyroZangle = 0.0;
double CFangleX = 0.0;
double CFangleY = 0.0;
systemtimer gyroCycleTimer;


long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


double kalmanFilterX(double accAngle, double gyroRate, double DT)
{
	double  y, S;
	double K_0, K_1;


	KFangleX += DT * (gyroRate - x_bias);

	XP_00 +=  - DT * (XP_10 + XP_01) + Q_angle * DT;
	XP_01 +=  - DT * XP_11;
	XP_10 +=  - DT * XP_11;
	XP_11 +=  + Q_gyro * DT;

	y = accAngle - KFangleX;
	S = XP_00 + R_angle;
	K_0 = XP_00 / S;
	K_1 = XP_10 / S;

	KFangleX +=  K_0 * y;
	x_bias  +=  K_1 * y;
	XP_00 -= K_0 * XP_00;
	XP_01 -= K_0 * XP_01;
	XP_10 -= K_1 * XP_00;
	XP_11 -= K_1 * XP_01;

	return KFangleX;
}


double kalmanFilterY(double accAngle, double gyroRate, double DT)
{
	double  y, S;
	double K_0, K_1;


	KFangleY += DT * (gyroRate - y_bias);

	YP_00 +=  - DT * (YP_10 + YP_01) + Q_angle * DT;
	YP_01 +=  - DT * YP_11;
	YP_10 +=  - DT * YP_11;
	YP_11 +=  + Q_gyro * DT;

	y = accAngle - KFangleY;
	S = YP_00 + R_angle;
	K_0 = YP_00 / S;
	K_1 = YP_10 / S;

	KFangleY +=  K_0 * y;
	y_bias  +=  K_1 * y;
	YP_00 -= K_0 * YP_00;
	YP_01 -= K_0 * YP_01;
	YP_10 -= K_1 * YP_00;
	YP_11 -= K_1 * YP_01;

	return KFangleY;
}



// initialize gyroscope, accelerometer and magnetometer
unsigned char PC_Init(void)
{
	unsigned char retval = PC_SUCCESS;
	retval |= PC_Gyro_Init();
	retval |= PC_Mag_Init();
	angleData.gier = 0;
	angleData.nick = 0;
	angleData.roll = 0;
	angleData.gierOffset = 0;
	angleData.nickOffset = 0;
	angleData.rollOffset = 0;
	KFangleX = 0.0;
	KFangleY = 0.0;
	gyroXangle = 0.0;
	gyroYangle = 0.0;
	gyroZangle = 0.0;
	CFangleX = 0.0;
    CFangleY = 0.0;
	return (retval);
}

Quaternion PC_mulQuat(Quaternion qa, Quaternion qb )
{
	Quaternion qResult;
	qResult.scalar = qa.scalar * qb.scalar - qa.x * qb.x - qa.y * qb.y - qa.z * qb.z;
	qResult.x = qa.scalar * qb.x + qa.x * qb.scalar + qa.y * qb.z - qa.z * qb.y;
	qResult.y = qa.scalar * qb.y - qa.x * qb.z + qa.y * qb.scalar + qa.z * qb.x;
	qResult.z = qa.scalar * qb.z + qa.x * qb.y - qa.y * qb.x + qa.z * qb.scalar;
	return  qResult;
}

Quaternion PC_conjugateQuat(Quaternion qa)
{
    Quaternion qResult;
    qResult.scalar = qa.scalar;
    qResult.x = -qResult.x;
    qResult.y = -qResult.y;
    qResult.z = -qResult.z;
    return qResult;
}

void PC_calcAngleDegrees(void)
{
	double nickAngle = 0.0;
	double nickAngleDeg = 0.0;
	double rollAngle = 0.0;
	double rollAngleDeg = 0.0;
	double gierAngle = 0.0;
    Quaternion m;

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
	nickAngle = asin(accData.posX);
	rollAngle = -asin(accData.posY/cos(nickAngle));
	nickAngleDeg = nickAngle * RAD_TO_DEG;
	rollAngleDeg = rollAngle * RAD_TO_DEG;

	// Kalman Filter
	double kalmanX = kalmanFilterX(rollAngleDeg,gyroData.posX, elapsedTime);
	double kalmanY = kalmanFilterY(nickAngleDeg,gyroData.posY, elapsedTime);
	//printf ("\033[22;31mkalmanX %7.3f  \033[22;36mkalmanY %7.3f\t\e[m",kalmanX,kalmanY);

	//Complementary filter used to combine the accelerometer and gyro values.
	CFangleX=AA*(CFangleX+gyroData.posX*elapsedTime) +(1 - AA) * rollAngleDeg;
	CFangleY=AA*(CFangleY+gyroData.posY*elapsedTime) +(1 - AA) * nickAngleDeg;

	gyroXangle +=gyroData.posX * elapsedTime;
	gyroYangle +=gyroData.posY * elapsedTime;
	//printf ("GyroX  %7.3f \t AccXangle \e[m %7.3f \t \033[22;31mCFangleX %7.3f\033[0m\t GyroY  %7.3f \t AccYangle %7.3f \t \033[22;36mCFangleY %7.3f\t\033[0m\n",gyroXangle,rollAngleDeg,CFangleX,gyroYangle,nickAngleDeg,CFangleY);

	//drift correction of gyroscope
	if((gyroXangle > 90)||(gyroXangle < -90))
	{
		CFangleX = rollAngleDeg;
		gyroXangle = rollAngleDeg;
	}
	if((gyroYangle > 90)||(gyroYangle < -90))
	{
		CFangleY = nickAngleDeg;
		gyroYangle = nickAngleDeg;
	}

	//take roll and nick angles from kalman filter for further calculations
	nickAngle = CFangleY / RAD_TO_DEG;
	rollAngle = CFangleX / RAD_TO_DEG;

	m.x = (magData.posX - calMagDat.calXMin) / (calMagDat.calXMax - calMagDat.calXMin) * 2 - 1;
    m.y = (magData.posY - calMagDat.calYMin) / (calMagDat.calYMax - calMagDat.calYMin) * 2 - 1;
    m.z = (magData.posZ - calMagDat.calZMin) / (calMagDat.calZMax - calMagDat.calZMin) * 2 - 1;
	double magXcomp = m.x * cos(nickAngle) + m.z * sin(nickAngle);
	double magYcomp = m.x * sin(rollAngle) * sin(nickAngle) + m.y* cos(rollAngle) - m.z * sin(rollAngle) * cos(nickAngle);
    gierAngle = atan2(magYcomp,magXcomp);

    angleData.nick = nickAngle * RAD_TO_DEG;
    angleData.roll = rollAngle * RAD_TO_DEG;
    // Convert radians to degrees for readability. (180/pi)
    gierAngle *=  RAD_TO_DEG;
    if(gierAngle < 0)
    {
    	gierAngle += 360;
    }
    angleData.gier = (gierAngle + angleData.gierOffset);
}


double PC_distance(double a, double b)
{
	return(sqrt(a * a) + (b * b));
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
	unsigned char retVal = PC_Mag_Calibrate();
	return(retVal);
}

void PC_Test(void)
{
	while(1)
	{
		PC_calc_Angles();
		printf("Gier=%d grad Nick=%d  Roll=%d\n",angleData.gier,angleData.nick,angleData.roll);
		wait(20000);
	}
}



