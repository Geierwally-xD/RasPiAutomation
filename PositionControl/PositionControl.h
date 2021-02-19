/*==============================================================================

 Name:           PositionControl.h
 Description:    declaration of camera position control driver
 Copyright:      Geierwally, 2021(c)

==============================================================================*/

#ifndef POSITIONCONTROL_POSITIONCONTROL_H_
#define POSITIONCONTROL_POSITIONCONTROL_H_

	#define PC_OUT_1  21 // relais output signal 1
	#define PC_OUT_2  22 // relais output signal 2
	#define PC_OUT_3  23 // relais output signal 3
	#define PC_OUT_4  24 // relais output signal 4
	#define PC_OUT_5  25 // relais output signal 5
	#define PC_OUT_6  27 // relais output signal 6
	//#define PC_OUT_7  28 // relais output signal 7
	//#define PC_OUT_8  29 // relais output signal 8
	#define PC_IN_1   28 // relais output signal 7
	#define PC_IN_2   29 // relais output signal 8
	#define PC_SUCCESS 0 // position control successful finished
	#define PC_FAILLED 1 // device failure sensor not detected
    #define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
    #define Phi 3.14159265358979323846
	#define AA 0.97         // complementary filter constant
	#define RAD_TO_DEG 57.29578


	typedef struct
	{
		short nick;	 // calculated nick angle
		short gier;	 // calculated gier angle
	    short roll;  // calculated roll angle
		short nickOffset;
		short gierOffset;
		short rollOffset;
	}PosAngle_Data;

	typedef struct
	{
		double scalar;
		double x;
		double y;
		double z;
	}Quaternion;

	extern PosAngle_Data angleData;     // calculated nick and gier angles
	extern unsigned char PC_Init(void);
	extern void PC_Test(void);
	extern unsigned char PC_Calibrate(void);

#endif /* POSITIONCONTROL_POSITIONCONTROL_H_ */
