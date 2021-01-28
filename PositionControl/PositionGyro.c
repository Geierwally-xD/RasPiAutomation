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
#include "timer.h"

int gyroDevice = 0;
PosAcc_Data accData = {0,0,0,0};
PosGyro_Data gyroData = {0,0,0};

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
static const int16_t range_gyro[] = { 2450, 5000, 10000, 20000 };

// initializes gyro accelerometer
unsigned char PC_Gyro_Init(void)
{
	unsigned char retVal = GY_SUCCESS;

	gyroDevice = wiringPiI2CSetup(GYRO_HIGH_ADDR);          /*Initializes I2C with device Address*/
    wait(10);

	if(gyroDevice >0)
	{
		// enable X, Y, Z axis of gyroscope
		wiringPiI2CWriteReg8 (gyroDevice, CTRL10_C, 0b00000000);
        wait(10);
		// LSM6DS33 gyro init
		// ODR = 1000 (1.66 kHz (high performance))
		// FS_G = 11 (2000 dps)
		wiringPiI2CWriteReg8 (gyroDevice, CTRL2_G, 0b00000000);
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
	}
	else
	{
		retVal = GY_FAILLED; // sensor not detected
	}
	return (retVal);
}


// read data from accelerometer
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
		posVal  = posVal *  range_acc[LSM6DSL_ACC_FS_8G] / 0xfffe;
		accData.shortposX = posVal;
		accData.posX = posVal;

		accData.posY = 0;
		posVal = wiringPiI2CReadReg8(gyroDevice, OUTY_L_XL);
		tmp = wiringPiI2CReadReg8(gyroDevice, OUTY_H_XL);
		posVal  |= tmp<<8;
		posVal  = posVal *  range_acc[LSM6DSL_ACC_FS_8G] / 0xfffe;
		accData.posY = posVal;

		accData.posZ = 0;
		posVal = wiringPiI2CReadReg8(gyroDevice, OUTZ_L_XL);
		tmp = wiringPiI2CReadReg8(gyroDevice, OUTZ_H_XL);
		posVal |= tmp<<8;
		posVal  = posVal *  range_acc[LSM6DSL_ACC_FS_8G] / 0xfffe;
		accData.posZ = posVal;

		/* calculate acceleration raw data with range 8G  */
		//accData.posX = accData.posX *  range_acc[LSM6DSL_ACC_FS_8G] / 0xfffe;
		//accData.posY = accData.posY *  range_acc[LSM6DSL_ACC_FS_8G] / 0xfffe;
		//accData.posZ = accData.posZ *  range_acc[LSM6DSL_ACC_FS_8G] / 0xfffe;
	}
	else
	{
		retVal = GY_FAILLED; // sensor not detected
	}
	return (retVal);
}


// read data from gyroscope
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
		gyroData.posX = gyroData.posX * range_gyro[LSM6DSL_GYRO_FS_245DPS] / 0xfff;
		gyroData.posY = gyroData.posY * range_gyro[LSM6DSL_GYRO_FS_245DPS] / 0xfff;
		gyroData.posZ = gyroData.posZ * range_gyro[LSM6DSL_GYRO_FS_245DPS] / 0xfff;
	}
	else
	{
		retVal = GY_FAILLED; // sensor not detected
	}
	return (retVal);
}
