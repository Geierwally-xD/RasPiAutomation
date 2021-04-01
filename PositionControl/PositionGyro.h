/*==============================================================================

 Name:           PositionGyro.h
 Description:    declaration acceleration and gyroscope position control driver for
                 LSM6DS33 3-axis accelerometer and gyroscope
 Copyright:      Geierwally, 2021(c)

==============================================================================*/

#ifndef POSITIONCONTROL_POSITIONGYRO_H_
#define POSITIONCONTROL_POSITIONGYRO_H_



#define GYRO_Device  0x69                 // device type LSM6DS33
#define GYRO_LOW_ADDR   0x6A
#define GYRO_HIGH_ADDR  0x6B

// register addresses of LIS3MDL
#define FUNC_CFG_ACCESS     0x01

#define FIFO_CTRL1          0x06
#define FIFO_CTRL2          0x07
#define FIFO_CTRL3          0x08
#define FIFO_CTRL4          0x09
#define FIFO_CTRL5          0x0A
#define ORIENT_CFG_G        0x0B
#define INT1_CTRL           0x0D
#define INT2_CTRL           0x0E
#define WHO_AM_I            0x0F
#define CTRL1_XL            0x10
#define CTRL2_G             0x11
#define CTRL3_C             0x12
#define CTRL4_C             0x13
#define CTRL5_C             0x14
#define CTRL6_C             0x15
#define CTRL7_G             0x16
#define CTRL8_XL            0x17
#define CTRL9_XL            0x18
#define CTRL10_C            0x19

#define WAKE_UP_SRC         0x1B
#define TAP_SRC             0x1C
#define D6D_SRC             0x1D
#define GY_STATUS_REG       0x1E

#define OUT_TEMP_L          0x20
#define OUT_TEMP_H          0x21
#define OUTX_L_G            0x22
#define OUTX_H_G            0x23
#define OUTY_L_G            0x24
#define OUTY_H_G            0x25
#define OUTZ_L_G            0x26
#define OUTZ_H_G            0x27
#define OUTX_L_XL           0x28
#define OUTX_H_XL           0x29
#define OUTY_L_XL           0x2A
#define OUTY_H_XL           0x2B
#define OUTZ_L_XL           0x2C
#define OUTZ_H_XL           0x2D

#define FIFO_STATUS1        0x3A
#define FIFO_STATUS2        0x3B
#define FIFO_STATUS3        0x3C
#define FIFO_STATUS4        0x3D
#define FIFO_DATA_OUT_L     0x3E
#define FIFO_DATA_OUT_H     0x3F
#define TIMESTAMP0_REG      0x40
#define TIMESTAMP1_REG      0x41
#define TIMESTAMP2_REG      0x42

#define STEP_TIMESTAMP_L    0x49
#define STEP_TIMESTAMP_H    0x4A
#define STEP_COUNTER_L      0x4B
#define STEP_COUNTER_H      0x4C

#define FUNC_SRC            0x53

#define TAP_CFG             0x58
#define TAP_THS_6D          0x59
#define INT_DUR2            0x5A
#define WAKE_UP_THS         0x5B
#define WAKE_UP_DUR         0x5C
#define FREE_FALL           0x5D
#define MD1_CFG             0x5E
#define MD2_CFG             0x5F

// range of accelerometer
#define LSM6DSL_ACC_FS_2G   0x00
#define LSM6DSL_ACC_FS_4G   0x01
#define LSM6DSL_ACC_FS_8G   0x02
#define LSM6DSL_ACC_FS_16G  0x03

// range of gyroscope
#define LSM6DSL_GYRO_FS_115DPS  115
#define LSM6DSL_GYRO_FS_245DPS  245
#define LSM6DSL_GYRO_FS_338DPS  338
#define LSM6DSL_GYRO_FS_500DPS  500
#define LSM6DSL_GYRO_FS_1000DPS 1000
#define LSM6DSL_GYRO_FS_2000DPS 2000

#define GY_SUCCESS 0 // read accelerometer or/and gyroscope data successful finished
#define GY_FAILLED 1 // device failure sensor not detected

typedef struct
{
	double posX;	 // accelerometer x position
	double posY;	 // accelerometer y position
	double posZ;	 // accelerometer z position
	double offsetX;
	double offsetY;
	double offsetZ;
}PosAcc_Data;


typedef struct
{
	double posX;	 // gyroscope x position
	double posY;	 // gyroscope y position
	double posZ;	 // gyroscope z position
	double offsetX;
	double offsetY;
	double offsetZ;
}PosGyro_Data;

typedef struct
{
	double calX;	 // calibration gyro X
	double calY;	 // calibration gyro Y
	double calZ;	 // calibration gyro Z
}CalGyro_Data;

extern int gyroDevice;  // device of accelerometer and gyroscope
extern PosAcc_Data accData;   // raw data X,Y,Z accelerometer
extern PosGyro_Data gyroData; // raw data X,Y,Z gyroscope
extern unsigned char PC_Gyro_Init(void); // initialize accelerometer and gyroscope sensors
extern unsigned char PC_Acc_Read(void);  // read data from accelerometer
extern unsigned char PC_Gyro_Read(void); // read data from gyroscope
extern unsigned char PC_Gyr_Calibrate(void);


#endif /* POSITIONCONTROL_POSITIONGYRO_H_ */
