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
    #define PC_CALIB_FAILED 2 // calibration failed
	#define PC_WRITE_FAILED 14 // PC write data file failed
	#define PC_READ_FAILED 15 // PC read data file failed
    #define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
    #define Phi 3.14159265358979323846
	#define AA 0.97          // complementary filter constant
	#define RAD_TO_DEG 57.29578
	#define Magnetometer 1
	#define Gyroscope 2

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

	typedef struct
	{
	    /* Kalman filter variables */
	    double Q_angle;  // Process noise variance for the accelerometer / magnetometer
	    double Q_bias;   // Process noise variance for the gyro bias
	    double R_measure;// Measurement noise variance - this is actually the variance of the measurement noise
	    double bias;     // The gyro bias calculated by the Kalman filter - part of the 2x1 state vector
	    double rate;     // Unbiased rate calculated from the rate and the calculated bias - you have to call getAngle to update the rate
	    double P[2][2];  // Error covariance matrix - This is a 2x2 matrix
		double KFangle;  // The angle calculated by the Kalman filter - part of the 2x1 state vector
	}KalmanFilter;

	extern PosAngle_Data angleData;                 // calculated nick and gier angles
	extern unsigned char PC_Init(void);
	extern void PC_Test(void);
	extern void PC_Test_Pos(void);
	extern unsigned char PC_Calibrate(unsigned char comp); // calibrate magnetometer or gyroscope depending parameter comp
    extern unsigned char PC_Move(unsigned char ID); // move to position
    extern unsigned char PC_Teach(unsigned char ID);// teach position
	extern unsigned char PC_Sequencer(int ID);      // position control sequence
	extern unsigned char PC_readDatFile(void);
	extern unsigned char PC_writeDatFile(void);
    extern unsigned char PC_MoveButton (unsigned char ID); // move button pressed \ released function
    extern void PC_Cal_switchMove(int direction); // switch direction of gier and nich move during calibration
    extern void PC_shutdown();
    extern unsigned char PC_calc_Angles(unsigned char init);

#endif /* POSITIONCONTROL_POSITIONCONTROL_H_ */
