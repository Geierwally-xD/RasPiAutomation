/*==============================================================================

 Name:           ZoomControl.h
 Description:    declaration of time controlled camcorder zoom driver with
                 PWM servo
 Copyright:      Geierwally, 2022(c)

==============================================================================*/

#ifndef POSITIONCONTROL_ZOOMCONTROL_H_
#define POSITIONCONTROL_ZOOMCONTROL_H_
#include <stdint.h>
// data types
typedef struct
{
   uint16_t ServoMiddle;      // 16 bit value servo middle position
   uint16_t ServoReference;   // 16 bit value servo reference move position
   uint16_t ServoControl;     // 16 bit value servo control move + position
   uint16_t ServoControlN;    // 16 bit value servo control move - position
   uint32_t CalibrationTime;  // 32 bit value calibration time
}AZ_CONFIG;

// constants
#define AZ_SERVO_PERIOD 20000 // servo period time 20 ms
// return values
#define AZ_SUCCESS 0
#define AZ_READ_FAILED 10
#define AZ_WRITE_FAILED 11
// execute IDs

// servo positions
#define AZ_MIDDLE      0       // middle position
#define AZ_REF_RIGHT   1       // move reference position right
#define AZ_REF_LEFT    2       // move reference position left
#define AZ_CON_RIGHT   3       // move control position right
#define AZ_CON_LEFT    4       // move control position left

// zoom control task states
#define AZ_TASK_IDLE   0       // nothing to do wait for next movement
#define AZ_TASK_INITIALIZE  10 // calculate position, start move timer, start movement
#define AZ_TASK_MOVE   20      // move and wait for move timer elapsed, if elapsed, stop move

// global variables and functions
extern uint8_t ZoomControl_init();
extern uint8_t ZoomControl_readConfFile(AZ_CONFIG * config); // read zoom config
extern uint8_t ZoomControl_readDatFile(uint8_t * data);      // read zoom positions
extern uint8_t ZoomControlwriteDatFile(void);                // write zoom positions
extern uint8_t ZoomControl_MoveToPos(uint8_t Position);      // move zoom to position
extern uint8_t ZoomControl_Calibrate();                      // calibrate auto zoom
extern uint8_t ZoomControl_TestPositions();                  // move to first five positions in a loop
extern uint8_t ZoomControl_MoveServo(uint8_t index);         // move servo to position
extern uint8_t ZoomControl_Task(uint8_t State);              // task move zoom to position
extern void ZoomControl_shutdown(void);                      // shut down PWM servo pulse
extern uint8_t _AZ_ZoomValue[];                              // all zoom positions
extern AZ_CONFIG _AZ_Config;                                 // zoom configuration
extern uint8_t   _AZ_Next_Position;                          // next zoom value

#endif /* POSITIONCONTROL_ZOOMCONTROL_H_ */
