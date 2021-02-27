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
#include "PositionControl.h"
#include "PositionGyro.h"

int magDevice = 0;
PosMag_Data magData = {0,0,0};
CalMag_Data calMagDat = {0,0,0,0,0,0,0,0,0,0};


// write active audio channel into datfile
unsigned char PC_Mag_writeDatFile(void)
{
	unsigned char result = MAG_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("PC_Mag_Dat.bin","wb");  // w for write, b for binary
	if(write_ptr != NULL)
	{
		fwrite(&calMagDat,sizeof(calMagDat),1,write_ptr); // write calibration data file
		fclose(write_ptr);
	}
	else
	{
		result = MAG_FAILLED;
	}
	return(result);
}

// read and set calibration date from datfile
unsigned char PC_Mag_readDatFile(void)
{
	unsigned char result = MAG_SUCCESS;
	FILE *read_ptr;

	read_ptr = fopen("PC_Mag_Dat.bin","rb");  // r for read, b for binary
	if(read_ptr != NULL)
	{
		fread(&calMagDat,sizeof(calMagDat),1,read_ptr); // read calibration data file
		fclose(read_ptr);
	}
	else
	{
		result = MAG_FAILLED;
	}
	return(result);
}

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
		calMagDat.calDeclinationAngle = 0.0645772; // magnetic declination angle of Hersbruck +3.7° in rad
		retVal = PC_Mag_readDatFile();
	}
	else
	{
		retVal = MAG_FAILLED; // sensor not detected
	}
	return (retVal);
}

// calibrate magnetometer
unsigned char PC_Mag_Calibrate(void)
{
	unsigned char retVal = MAG_SUCCESS;
	systemtimer calTimer;
	calMagDat.calXMax = 0;
	calMagDat.calXMin = 0;
	calMagDat.calYMax = 0;
	calMagDat.calYMin = 0;
	calMagDat.calZMax = 0;
	calMagDat.calZMin = 0;
	calMagDat.calXCenter = 0;
	calMagDat.calYCenter = 0;
	calMagDat.calZCenter = 0;


	startMeasurement(&calTimer);

	while((isExpired(MAG_CAL_TIMEOUT,&calTimer)==0)/*&&(nextStepHelper == nextStep)*/)
	{
		retVal = PC_Acc_Read();  // get accelerometer raw data from sensor
		retVal |= PC_Mag_Read();
		if(retVal == MAG_SUCCESS)
		{
			if(magData.posX > calMagDat.calXMax)
			{
				calMagDat.calXMax = magData.posX;
				printf("calib Mag new Xmax=%f\n",calMagDat.calXMax);
			}
			else if(magData.posX < calMagDat.calXMin)
			{
				calMagDat.calXMin = magData.posX;
				printf("calib Mag new Xmin=%f\n",calMagDat.calXMin);
			}
			if(magData.posY > calMagDat.calYMax)
			{
				calMagDat.calYMax = magData.posY;
				printf("calib Mag new Ymax=%f\n",calMagDat.calYMax);
			}
			else if(magData.posY < calMagDat.calYMin)
			{
				calMagDat.calYMin = magData.posY;
				printf("calib Mag new Ymin=%f\n",calMagDat.calYMin);
			}
			if(magData.posZ > calMagDat.calZMax)
			{
				calMagDat.calZMax = magData.posZ;
				printf("calib Mag new Zmax=%f\n",calMagDat.calZMax);
			}
			else if(magData.posZ < calMagDat.calZMin)
			{
				calMagDat.calZMin = magData.posZ;
				printf("calib Mag new Zmin=%f\n",calMagDat.calZMin);
			}
		}

	}
	calMagDat.calXCenter = (calMagDat.calXMax + calMagDat.calXMin)/2;
	calMagDat.calYCenter = (calMagDat.calYMax + calMagDat.calYMin)/2;
	calMagDat.calZCenter = (calMagDat.calZMax + calMagDat.calZMin)/2;
    printf("calibration magnetometer done X center=%f  Y center=%f Z center=%f\n",calMagDat.calXCenter,calMagDat.calYCenter,calMagDat.calZCenter);
	retVal |= PC_Mag_writeDatFile(); // write new calibration data into binary file
	return(retVal);
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
		//magData.posX = (temp*1 + magData.posX*8)/9; // calculate average X
		magData.posX = temp;


		temp   = 0;
		temp   = wiringPiI2CReadReg8(magDevice, OUT_Y_L);
		temp |= ((wiringPiI2CReadReg8(magDevice, OUT_Y_H)<<8)& 0xff00);
		//magData.posY = (temp*1 + magData.posY*8)/9; // calculate average Y
		magData.posY = temp;


		temp  = 0;
		temp  = wiringPiI2CReadReg8(magDevice, OUT_Z_L);
		temp |= ((wiringPiI2CReadReg8(magDevice, OUT_Z_H)<<8)& 0xff00);
		//magData.posZ = (temp*1 + magData.posZ*8)/9; // calculate average Z
		magData.posZ = temp;
	}
	else
	{
		retVal = MAG_FAILLED; // sensor not detected
	}
    return(retVal);
}



