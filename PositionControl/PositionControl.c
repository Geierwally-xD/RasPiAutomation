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

PosAngle_Data angleData = {0,0}; // calculated nick and gier angles
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
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
	return (retval);
}

double PC_distance(double a, double b)
{
	return(sqrt(a * a) + (b * b));
}

void PC_get_Roll(void)
{
    /****solution 1 map roll angle to x axis accelerometer****/
	short degrees = 0;
	degrees = map((short)accData.posX,-1000, 1000, -90, 90);
	degrees = constrain(degrees, -90, 90);

	/****solution 2 calculate nick angle with accelerometer x y z raw data****/
	//double degrees = atan2(accData.posY,PC_distance(accData.posX,accData.posZ)) * 57.2957795;
	angleData.roll = degrees;
}

void PC_get_Nick(void)
{
    /****solution 1 map nick angle to x axis accelerometer****/
	short degrees = 0;
	degrees = map(accData.shortposX,-1000, 1000, -90, 90);
	degrees = constrain(degrees, -90, 90);

	/****solution 2 calculate nick angle with accelerometer x y z raw data****/
	//double degrees = atan2(accData.posX,PC_distance(accData.posY,accData.posZ)) * 57.2957795;

	angleData.nick = degrees;
}

void PC_get_Gier(void)
{
    float degrees = 0;
    float xcenter = (magData.Xmax + magData.Xmin)/2;
    float ycenter = (magData.Ymax + magData.Ymin)/2;

    // calculate gier angle with magnetometer raw data of x and y axis
    degrees = atan2(magData.posX - xcenter, magData.posY - ycenter) + magData.DeclinationAngle;

    // Correct for when signs are reversed.
    if(degrees < 0)
    {
    	degrees += 6.283185309; // add pi*2
    }

    // Check for wrap due to addition of declination. (angle > pi*2)
    if(degrees > 6.283185309)
    {
    	degrees -= 6.283185309;
    }
    // Convert radians to degrees for readability. (180/pi)
    degrees *=  57.2957795;
   	angleData.gier = degrees + angleData.gierOffset;
}

// calculate nick and gier angles
unsigned char PC_calc_Angles(void)
{
	unsigned char retval = PC_SUCCESS;
	angleData.gier = 0;
	angleData.nick = 0;
	retval = PC_Acc_Read();  // get accelerometer raw data from sensor
	retval |= PC_Mag_Read(); // get magnetometer  raw data from sensor
	if(retval == PC_SUCCESS)
	{
		PC_get_Nick(); // calculate nick angle with acceleration raw data
		PC_get_Gier(); // calculate gier angle with magnetometer raw data
	}
    return(retval );
}

void PC_Test(void)
{
	while(1)
	{
		PC_calc_Angles();
		printf("Gier=%d grad Nick=%d\n",angleData.gier,angleData.nick);
		wait(80000);
	}
}




