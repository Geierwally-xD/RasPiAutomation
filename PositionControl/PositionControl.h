/*==============================================================================

 Name:           PositionControl.h
 Description:    declaration of camera position mover control driver
 Copyright:      Geierwally, 2021(c)

==============================================================================*/

#ifndef POSITIONCONTROL_POSITIONCONTROL_H_
#define POSITIONCONTROL_POSITIONCONTROL_H_

	#define PC_OUT_MOVE_GIER_LEFT   21 // relais output signal move gier to left
	#define PC_OUT_MOVE_GIER_RIGHT  22 // relais output signal move gier to right
	#define PC_OUT_MOVE_NICK_UP     23 // relais output signal move nick up
	#define PC_OUT_MOVE_NICK_DOWN   24 // relais output signal move nick down
	#define PC_OUT_LOCK_REMOTE      25 // relais output signal lock remote control
	#define PC_OUT_1                27 // relais output signal 1
	#define PC_OUT_2                28 // relais output signal 2
	#define PC_OUT_3                29 // relais output signal 3
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

	typedef struct
	{
		double nick;	 // calculated nick angle
		double gier;	 // calculated gier angle
	    double roll;     // calculated roll angle
		double nickOffset;
		double gierOffset;
		double rollOffset;
	}PosAngle_Data;

	typedef struct
	{
		double x;
		double y;
		double z;
	}Vector3d;


	extern PosAngle_Data angleData;                 // calculated nick and gier angles
	extern unsigned char PC_Init(void);
	extern void PC_Test(void);
	extern void PC_Test_Pos(void);
    extern unsigned char PC_Move(unsigned char ID); // move to position
    extern unsigned char PC_Teach(unsigned char ID);// teach position
	extern unsigned char PC_Sequencer(int ID);      // position control sequence
	extern unsigned char PC_readDatFile(void);
	extern unsigned char PC_writeDatFile(void);
    extern unsigned char PC_MoveButton (unsigned char ID); // move button pressed \ released function
    extern void PC_shutdown();
    extern unsigned char PC_calc_Angles(void);

#endif /* POSITIONCONTROL_POSITIONCONTROL_H_ */
