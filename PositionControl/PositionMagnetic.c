/*==============================================================================

 Name:           PositionMagnetic.c
 Description:    functionality magnetic position control driver for
                 LIS3MDL 3-axis magnetometer
 Copyright:      Geierwally, 2021(c)

==============================================================================*/
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <math.h>
#include "PositionMagnetic.h"
#include "timer.h"

int magDevice = 0;
PosMag_Data magData = {0,0,0,0,0,0,0,0};

// initializes magnetometer
unsigned char PC_Mag_Init(void)
{
	unsigned char retVal = MAG_SUCCESS;

	magDevice = wiringPiI2CSetup(MAG_HIGH_ADDR);    /*Initializes I2C with device Address*/

	if(magDevice >0)
	{

		//Temperature disable, X Y axis ultra high performance, 40 Hz, fast ODR dis, self test disabled
		wiringPiI2CWriteReg8 (magDevice, CTRL_REG1, 0b01111000);
		wait(10);
		//Full scale +-16gauss, no reboot normal mode, no SOFT-RESET normal operation
		wiringPiI2CWriteReg8 (magDevice, CTRL_REG2, 0b01100000);
		wait(10);
		//no LP, wake up, continuous conversion mode
		wiringPiI2CWriteReg8 (magDevice, CTRL_REG3, 0b00000000);
		wait(10);
		//Z axis medium performance, little endian data selection, continuous update
		wiringPiI2CWriteReg8 (magDevice, CTRL_REG4, 0b00000100);
		wait(10);
		//not fast read (you must read H and L), no block data update
		wiringPiI2CWriteReg8 (magDevice, CTRL_REG5, 0b00000000);
		wait(100000);
		magData.posX = 0;
		magData.posY = 0;
		magData.posZ = 0;
		magData.Xmax = 508;
		magData.Xmin = -992;
		magData.Ymax = 550;
		magData.Ymin = -1476;
		magData.DeclinationAngle = 0.0645772; // magnetic declination angle of Hersbruck +3.7° in rad
	}
	else
	{
		retVal = MAG_FAILLED; // sensor not detected
	}
	return (retVal);
}

// read data from magnetometer
unsigned char PC_Mag_Read(void)
{
	unsigned char retVal = MAG_SUCCESS;

	if(magDevice >0)
	{
		short temp = 0;
		temp  = wiringPiI2CReadReg8(magDevice, OUT_X_L);
		temp |= ((wiringPiI2CReadReg8(magDevice, OUT_X_H)<<8)& 0xff00);
		magData.posX = (temp*1 + magData.posX*8)/9; // calculate average X
		if(magData.posX > magData.Xmax)
		{
			magData.Xmax = magData.posX;
		}
		else if(magData.posX < magData.Xmin)
		{
			magData.Xmin = magData.posX;
		}

		temp   = 0;
		temp   = wiringPiI2CReadReg8(magDevice, OUT_Y_L);
		temp |= ((wiringPiI2CReadReg8(magDevice, OUT_Y_H)<<8)& 0xff00);
		magData.posY = (temp*1 + magData.posY*8)/9; // calculate average Y
		if(magData.posY > magData.Ymax)
		{
			magData.Ymax = magData.posY;
		}
		else if(magData.posY < magData.Ymin)
		{
			magData.Ymin = magData.posY;
		}

		temp  = 0;
		temp  = wiringPiI2CReadReg8(magDevice, OUT_Z_L);
		temp |= ((wiringPiI2CReadReg8(magDevice, OUT_Z_H)<<8)& 0xff00);
		magData.posZ = temp;
	}
	else
	{
		retVal = MAG_FAILLED; // sensor not detected
	}
    return(retVal);
}

