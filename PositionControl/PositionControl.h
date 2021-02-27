/*==============================================================================

 Name:           PositionControl.h
 Description:    declaration of camera position control driver
 Copyright:      Geierwally, 2021(c)

==============================================================================*/

#ifndef POSITIONCONTROL_POSITIONCONTROL_H_
#define POSITIONCONTROL_POSITIONCONTROL_H_

	#define PC_OUT_MOVE_GIER_LEFT   21 // relais output signal move gier to left
	#define PC_OUT_MOVE_GIER_RIGHT  22 // relais output signal move gier to right
	#define PC_OUT_MOVE_NICK_UP     23 // relais output signal move nick up
	#define PC_OUT_MOVE_NICK_DOWN   24 // relais output signal move nick down
	#define PC_OUT_LOCK_LEFT        25 // relais output signal lock remote control left
	#define PC_OUT_LOCK_RIGHT       27 // relais output signal lock remote control right
	#define PC_OUT_LOCK_UP          28 // relais output signal lock remote control up
	#define PC_OUT_LOCK_DOWN        29 // relais output signal lock remote control down
    #define PC_BUTTON_UP             1 // move up button pressed
    #define PC_BUTTON_DOWN           2 // move down button pressed
    #define PC_BUTTON_LEFT           3 // move left button pressed
    #define PC_BUTTON_RIGHT          4 // move right button pressed
    #define PC_BUTTON_RELEASED       5 // move button released
	#define PC_IN_1   28 // relais output signal 7
	#define PC_IN_2   29 // relais output signal 8
	#define PC_SUCCESS 0 // position control successful finished
	#define PC_FAILLED 1 // device failure sensor not detected
	#define PC_WRITE_FAILED 14 // PC write data file failed
	#define PC_READ_FAILED 15 // PC read data file failed
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
		double x;
		double y;
		double z;
	}Vector3d;

	typedef struct
	{
		double Q_angle;
		double Q_gyro;
		double R_angle;
		double bias;
		double P_00;
		double P_01;
		double P_10;
		double P_11;
		double KFangle;
		double CFangle;
	}KalmanFilter;

	extern PosAngle_Data angleData;                 // calculated nick and gier angles
	extern unsigned char PC_Init(void);
	extern void PC_Test(void);
	extern unsigned char PC_Calibrate(void);        // calibrate magnetometer
    extern unsigned char PC_Move(unsigned char ID); // move to position
    extern unsigned char PC_Teach(unsigned char ID);// teach position
	extern unsigned char PC_Sequencer(int ID);      // position control sequence
	extern unsigned char PC_readDatFile(void);
	extern unsigned char PC_writeDatFile(void);
    extern unsigned char PC_MoveButton (unsigned char ID); // move button pressed \ released function

#endif /* POSITIONCONTROL_POSITIONCONTROL_H_ */
