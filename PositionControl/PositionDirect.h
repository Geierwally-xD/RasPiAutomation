/*==============================================================================

 Name:           PositionDirect.h
 Description:    declarations for direct measurement position control driver with
                 AS5048B Magnetic Rorary Encoder
 Copyright:      Geierwally, 2021(c)

==============================================================================*/

#include <stdint.h>
#ifndef POSITIONCONTROL_POSITIONDIRECT_H_
#define POSITIONCONTROL_POSITIONDIRECT_H_

// OPERATIONS
#define SERIAL_DEBUG_ENABLED
#define USE_WIREBEGIN_ENABLED // to comment if Wire.begin() function is called in Setup() for instance. Usefull to manage one or several I2C devices in the same sketch

// Default addresses for AS5048B
#define AS5048_ADDRESS 0x40 // 0b10000 + ( A1 & A2 to GND)
#define AS5048B_PROG_REG 0x03
#define AS5048B_ADDR_REG 0x15
#define AS5048B_ZEROMSB_REG 0x16 //bits 0..7
#define AS5048B_ZEROLSB_REG 0x17 //bits 0..5
#define AS5048B_GAIN_REG 0xFA
#define AS5048B_DIAG_REG 0xFB
#define AS5048B_MAGNMSB_REG 0xFC //bits 0..7
#define AS5048B_MAGNLSB_REG 0xFD //bits 0..5
#define AS5048B_ANGLMSB_REG 0xFE //bits 0..7
#define AS5048B_ANGLLSB_REG 0xFF //bits 0..5
#define AS5048B_RESOLUTION 16384.0 //14 bits


// Moving Exponential Average on angle - beware heavy calculation for some Arduino boards
// This is a 1st order low pass filter
// Moving average is calculated on Sine et Cosine values of the angle to provide an extrapolated accurate angle value.
#define EXP_MOVAVG_N 5	//history length impact on moving average impact - keep in mind the moving average will be impacted by the measurement frequency too
#define EXP_MOVAVG_LOOP 1 //number of measurements before starting mobile Average - starting with a simple average - 1 allows a quick start. Value must be 1 minimum

//unit consts - just to make the units more readable
#define U_RAW 1
#define U_TRN 2
#define U_DEG 3
#define U_RAD 4
#define U_GRAD 5
#define U_MOA 6
#define U_SOA 7
#define U_MILNATO 8
#define U_MILSE 9
#define U_MILRU 10
#define M_PI 3.14159265358979323846


#define POSDI_SUCCESS 0 // read magnetsensor data successful finished
#define POSDI_FAILLED 1 // device failure sensor not detected


typedef struct
{
	uint8_t		_clockWise;
	uint8_t		_chipAddress;
	uint8_t		_addressRegVal;
	uint16_t	_zeroRegVal;
	double		_lastAngleRaw;
	double		_movingAvgExpAngle;
	double		_movingAvgExpSin;
	double		_movingAvgExpCos;
	double		_movingAvgExpAlpha;
	int		    _movingAvgCountLoop;
	int         _sensorAddress;
}PositionSens;

	//AMS_AS5048B(uint8_t chipAddress);

	extern unsigned char PosDirect_init(void);                                         // to init the object, must be called in the setup loop
	extern unsigned char PosDirect_setClockWise(PositionSens* psensor,uint8_t cw );    //set clockwise counting, default is false (native sensor)
	extern unsigned char PosDirect_progRegister(PositionSens* psensor,uint8_t regVal); //nothing so far - manipulate the OTP register
	extern unsigned char PosDirect_doProg(PositionSens* psensor); 				       //progress programming slave address OTP
	extern unsigned char PosDirect_doProgZero(PositionSens* psensor); 			       //progress programming zero position OTP
	extern unsigned char PosDirect_addressRegW(PositionSens* psensor,uint8_t regVal);  //change the chip address
	extern uint8_t       PosDirect_addressRegR(PositionSens* psensor); 			       //read chip address
	extern unsigned char PosDirect_setZeroReg(PositionSens* psensor); 			       //set Zero to current angle position
	extern unsigned char PosDirect_zeroRegW(PositionSens* psensor, uint16_t regVal);   //write Zero register value
	extern uint16_t      PosDirect_zeroRegR(PositionSens* psensor);  			       //read Zero register value
	extern uint16_t      PosDirect_angleRegR(PositionSens* psensor); 		   	       //read raw value of the angle register
	extern uint8_t       PosDirect_getDiagReg(PositionSens* psensor); 		           //read diagnostic register
	extern uint16_t      PosDirect_magnitudeR(PositionSens* psensor); 			       //read current magnitude
	extern double        PosDirect_angleR(PositionSens* psensor, int unit, uint8_t newVal); //Read current angle or get last measure with unit conversion : RAW, TRN, DEG, RAD, GRAD, MOA, SOA, MILNATO, MILSE, MILRU
	extern uint8_t       PosDirect_getAutoGain(PositionSens* psensor);
	extern void          PosDirect_updateMovingAvgExp(PositionSens* psensor);          //measure the current angle and feed the Exponential Moving Average calculation
	extern double        PosDirect_getMovingAvgExp(PositionSens* psensor, int unit);   //get Exponential Moving Average calculation
	extern void          PosDirect_resetMovingAvgExp(PositionSens* psensor);           //reset Exponential Moving Average calculation values
	extern uint8_t       PosDirect_readReg8(PositionSens* psensor, uint8_t address);
	extern uint16_t      PosDirect_readReg16(PositionSens* psensor, uint8_t address);  //16 bit value got from 2x8bits registers (7..0 MSB + 5..0 LSB) => 14 bits value
	extern unsigned char PosDirect_writeReg(PositionSens* psensor, uint8_t address, uint8_t value);
	extern double        PosDirect_convertAngle(int unit, double angle);               //RAW, TRN, DEG, RAD, GRAD, MOA, SOA, MILNATO, MILSE, MILRU
	extern double        PosDirect_getExpAvgRawAngle(PositionSens* psensor) ;
	extern void          PosDirect_printDebug(PositionSens* psensor);



#endif /* POSITIONCONTROL_POSITIONDIRECT_H_ */
