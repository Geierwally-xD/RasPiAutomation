/*==============================================================================

 Name:           PositionGyro.c
 Description:    functionality acceleration and gyroscope position control driver for
                 LSM6DS33 3-axis accelerometer and gyroscope
 Copyright:      Geierwally, 2021(c)

==============================================================================*/
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <math.h>
#include "PositionGyro.h"
#include "PositionControl.h"
#include "timer.h"

int gyroDevice = 0;
PosAcc_Data accData = {0,0,0,0,0,0};
PosGyro_Data gyroData = {0,0,0,0,0,0};



/**
 * order in array [0, 1, 2, 3] is
 * LSM6DSL_ACC_FS_2G, LSM6DSL_ACC_FS_4G, LSM6DSL_ACC_FS_8G, LSM6DSL_ACC_FS_16G
 */
static const int16_t range_acc[] = { 2000, 4000, 8000 , 16000};

/**
 * order in array [0, 1, 2, 3] is
 * LSM6DSL_GYRO_FS_245DPS, LSM6DSL_GYRO_FS_500DPS,
 * LSM6DSL_GYRO_FS_1000DPS, LSM6DSL_GYRO_FS_2000DPS
 */
static const int16_t range_gyro[] = { 245, 500, 1000, 2000 };
CalGyro_Data calGyDat = {0.0,0.0,0.0};

// write gyroscope calibration data into datfile
unsigned char PC_GY_writeDatFile(void)
{
	unsigned char result = GY_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("PC_GY_Dat.bin","wb");  // w for write, b for binary
	if(write_ptr != NULL)
	{
		fwrite(&calGyDat,sizeof(calGyDat),1,write_ptr); // write calibration data file
		fclose(write_ptr);
	}
	else
	{
		result = GY_FAILLED;
	}
	return(result);
}

// read and set gyroscope calibration data from datfile
unsigned char PC_GY_readDatFile(void)
{
	unsigned char result = GY_SUCCESS;
	FILE *read_ptr;

	read_ptr = fopen("PC_GY_Dat.bin","rb");  // r for read, b for binary
	if(read_ptr != NULL)
	{
		fread(&calGyDat,sizeof(calGyDat),1,read_ptr); // read calibration data file
		fclose(read_ptr);
	}
	else
	{
		result = GY_FAILLED;
	}
	return(result);
}


// initializes gyro accelerometer
unsigned char PC_Gyro_Init(void)
{
	unsigned char retVal = GY_SUCCESS;

	gyroDevice = wiringPiI2CSetup(GYRO_HIGH_ADDR);          /*Initializes I2C with device Address*/
    wait(10);

	if(gyroDevice >0)
	{
		// enable X, Y, Z axis of gyroscope
		wiringPiI2CWriteReg8 (gyroDevice, CTRL10_C, 0b00111000);
        wait(10);
		// LSM6DS33 gyro init
		// ODR = 1000 (1.66 kHz (high performance))
		// FS_G = 00 (245 dps)
		wiringPiI2CWriteReg8 (gyroDevice, CTRL2_G, 0b10000000);
        wait(10);
		// defaults
		wiringPiI2CWriteReg8 (gyroDevice, CTRL7_G, 0b00000000);
        wait(10);

        // enable X, Y, Z axis
		wiringPiI2CWriteReg8 (gyroDevice, CTRL9_XL, 0b00111000);
        wait(10);
		// LSM6DS33 accelerometer
		// ODR = 0011 (52Hz (high performance))
		// FS_XL = 10 (8 g full scale)
		// BW_XL = 00 (400 Hz filter bandwidth)
		wiringPiI2CWriteReg8 (gyroDevice, CTRL1_XL, 0b00111000);
        wait(10);
		// common
		// IF_INC = 0 (automatically increment address register) BDU = 1 BLE = 0 little endian
		wiringPiI2CWriteReg8 (gyroDevice, CTRL3_C, 0b01000000);
		wait(100000);
		gyroData.offsetX = 1.59;
		gyroData.offsetY = -2.908;
		gyroData.offsetZ = -2.959;

		if(PC_GY_readDatFile()== GY_SUCCESS)
		{
			gyroData.offsetX = calGyDat.calX;
			gyroData.offsetY = calGyDat.calY;
			gyroData.offsetZ = calGyDat.calZ;
		}
	}
	else
	{
		retVal = GY_FAILLED; // sensor not detected
	}
	return (retVal);
}


// read sensor raw data of accelerometer
unsigned char PC_Acc_Read(void)
{
	unsigned char retVal = GY_SUCCESS;
	unsigned char tmp = 0;
    short posVal = 0;

	if(gyroDevice >0)
	{
		accData.posX = 0;
		posVal = wiringPiI2CReadReg8(gyroDevice, OUTX_L_XL);
		tmp = wiringPiI2CReadReg8(gyroDevice, OUTX_H_XL);
		posVal |= tmp<<8;
		posVal  = posVal *  range_acc[LSM6DSL_ACC_FS_8G] / 0x7fff;
		accData.posX = posVal;

		accData.posY = 0;
		posVal = wiringPiI2CReadReg8(gyroDevice, OUTY_L_XL);
		tmp = wiringPiI2CReadReg8(gyroDevice, OUTY_H_XL);
		posVal  |= tmp<<8;
		posVal  = posVal *  range_acc[LSM6DSL_ACC_FS_8G] / 0x7fff;
		accData.posY = posVal;

		accData.posZ = 0;
		posVal = wiringPiI2CReadReg8(gyroDevice, OUTZ_L_XL);
		tmp = wiringPiI2CReadReg8(gyroDevice, OUTZ_H_XL);
		posVal |= tmp<<8;
		posVal  = posVal *  range_acc[LSM6DSL_ACC_FS_8G] / 0x7fff;
		accData.posZ = posVal;
	}
	else
	{
		retVal = GY_FAILLED; // sensor not detected
	}
	return (retVal);
}


// read sensor raw data of gyroscope
unsigned char PC_Gyro_Read(void)
{
	unsigned char retVal = GY_SUCCESS;
	unsigned char tmp = 0;
    short posVal = 0;

	if(gyroDevice >0)
	{
		gyroData.posX = 0;
		posVal = wiringPiI2CReadReg8(gyroDevice, OUTX_L_G);
		tmp = wiringPiI2CReadReg8(gyroDevice, OUTX_H_G);
		posVal |= tmp<<8;
		gyroData.posX = posVal;

		gyroData.posY = 0;
		posVal = wiringPiI2CReadReg8(gyroDevice, OUTY_L_G);
		tmp = wiringPiI2CReadReg8(gyroDevice, OUTY_H_G);
		posVal |= tmp<<8;
		gyroData.posY = posVal;

		gyroData.posZ = 0;
		posVal = wiringPiI2CReadReg8(gyroDevice, OUTZ_L_G);
		tmp = wiringPiI2CReadReg8(gyroDevice, OUTZ_H_G);
		posVal |= tmp<<8;
		gyroData.posZ = posVal;

        /* calculate gyroscope raw data with range 245DPS  */
        gyroData.posX = (gyroData.posX * range_gyro[LSM6DSL_GYRO_FS_500DPS] / 0x7fff)-gyroData.offsetX;
        gyroData.posY = (gyroData.posY * range_gyro[LSM6DSL_GYRO_FS_500DPS] / 0x7fff)-gyroData.offsetY;
        gyroData.posZ = (gyroData.posZ * range_gyro[LSM6DSL_GYRO_FS_245DPS] / 0x7fff)-gyroData.offsetZ;
	}
	else
	{
		retVal = GY_FAILLED; // sensor not detected
	}
	return (retVal);
}

// calibrate gyroscope
unsigned char PC_Gyr_Calibrate(void)
{
	unsigned char retVal = GY_SUCCESS;
	systemtimer calibrationTimer;
	systemtimer gyroCycleTimer;
    unsigned long long gyroCycleTime = 0;
    double elapsedTime = 0;
    Vector3d GyroAngle;
	gyroData.offsetX = 0.0;
	gyroData.offsetY = 0.0;
	gyroData.offsetZ = 0.0;
	gyroData.posX = 0.0;
	gyroData.posY = 0.0;
	gyroData.posZ = 0.0;

	PC_Gyro_Read();

	startMeasurement(&calibrationTimer);
	startMeasurement(&gyroCycleTimer);
	while(isExpired(60000000,&calibrationTimer)==0)
	{
	    gyroCycleTime = getRelativeTickCount(&gyroCycleTimer);
		startMeasurement(&gyroCycleTimer);
		retVal |= PC_Gyro_Read(); // get gyroscope raw data from sensor
		elapsedTime = (double)gyroCycleTime / 1000000.0;
		GyroAngle.x +=gyroData.posX * elapsedTime;
		GyroAngle.y +=gyroData.posY * elapsedTime;
		GyroAngle.z +=gyroData.posZ * elapsedTime;
		printf ("Gyr Nick %f Gyro Roll %f Gyro Gier %f \n",GyroAngle.y,GyroAngle.x,GyroAngle.z);
		while(isExpired(10000,&gyroCycleTimer)==0)
		{}
	}
	gyroData.offsetX = GyroAngle.x / 60;
	gyroData.offsetY = GyroAngle.y / 60;
	gyroData.offsetZ = GyroAngle.z / 60;
	calGyDat.calX = gyroData.offsetX;
	calGyDat.calY = gyroData.offsetY;
	calGyDat.calZ = gyroData.offsetZ;
	PC_GY_writeDatFile();

	printf ("Gyr NickOffset %f Gyro RollOffset %f Gyro GierOffset %f \n",gyroData.offsetY,gyroData.offsetX,gyroData.offsetZ);
	return(retVal);
}
