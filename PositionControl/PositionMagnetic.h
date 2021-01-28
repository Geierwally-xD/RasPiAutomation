/*==============================================================================

 Name:           PositionMagnetic.h
 Description:    declaration of magnetic position control driver for
                 LIS3MDL 3-axis magnetometer
 Copyright:      Geierwally, 2021(c)

==============================================================================*/

#ifndef POSITIONCONTROL_POSITIONMAGNETIC_H_
#define POSITIONCONTROL_POSITIONMAGNETIC_H_

#define MAG_Device 0x3D            // device type LIS3MDL
#define MAG_LOW_ADDR   0x1C
#define MAG_HIGH_ADDR  0x1E
// register addresses of LIS3MDL
#define WHO_AM_I  0x0F

#define CTRL_REG1   0x20
#define CTRL_REG2   0x21
#define CTRL_REG3   0x22
#define CTRL_REG4   0x23
#define CTRL_REG5   0x24

#define MAG_STATUS_REG  0x27
#define OUT_X_L     0x28
#define OUT_X_H     0x29
#define OUT_Y_L     0x2A
#define OUT_Y_H     0x2B
#define OUT_Z_L     0x2C
#define OUT_Z_H     0x2D
#define TEMP_OUT_L  0x2E
#define TEMP_OUT_H  0x2F
#define INT_CFG     0x30
#define INT_SRC     0x31
#define INT_THS_L   0x32
#define INT_THS_H   0x33

#define MAG_SUCCESS 0 // read magnetometer data successful finished
#define MAG_FAILLED 1 // device failure sensor not detected

typedef struct
{
	double posX;	 // magnetometer x position
	double posY;	 // magnetometer y position
	double posZ;	 // magnetometer z position
	double Xmax;
	double Xmin;
	double Ymax;
	double Ymin;
	double DeclinationAngle;
}PosMag_Data;


extern int magDevice;  // device of magnetometer
extern PosMag_Data magData; // raw data X,Y,Z magnetometer
extern unsigned char PC_Mag_Init(void); // initialize magnetometer
extern unsigned char PC_Mag_Read(void); // read data from magnetometer

#endif /* POSITIONCONTROL_POSITIONMAGNETIC_H_ */
