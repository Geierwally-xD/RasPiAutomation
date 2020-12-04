/*==============================================================================

 Name:           PositionControl.h
 Description:    declaration of camera position control driver
 Copyright:      Geierwally, 2020(c)

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

extern void PC_Init(void);

#endif /* POSITIONCONTROL_POSITIONCONTROL_H_ */
