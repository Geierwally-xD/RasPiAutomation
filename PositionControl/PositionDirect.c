/*==============================================================================

 Name:           PositionDirect.c
 Description:    functionality direct measurement position control driver with
                 AS5048B Magnetic Rorary Encoder
 Copyright:      Geierwally, 2021(c)

==============================================================================*/
#include <math.h>
#include <wiringPiI2C.h>
#include "PositionDirect.h"
#include "timer.h"

   PositionSens PosDir_Gier;
   PositionSens PosDir_Nick;
   int posDirDeviceGier = 0;
   int posDirDeviceNick = 0;

   //init values and overall behaviors for AS5948B use
	unsigned char PosDirect_init(void)
	{
		unsigned char retVal = POSDI_SUCCESS;

		PosDir_Gier._sensorAddress = AS5048_ADDRESS + 1;                     // device I²C addresses
		PosDir_Nick._sensorAddress = AS5048_ADDRESS;

		posDirDeviceGier = wiringPiI2CSetup(PosDir_Gier._sensorAddress); //Initializes I2C with device Address
	    wait(10000);
        posDirDeviceNick = wiringPiI2CSetup(PosDir_Nick._sensorAddress);
        wait(10000);

		if((posDirDeviceGier >0)&&(posDirDeviceNick >0))
		{
			PosDir_Gier._clockWise = 0;
			PosDir_Gier._lastAngleRaw = 0.0;
			PosDir_Gier._zeroRegVal =  PosDirect_zeroRegR(&PosDir_Gier);
			PosDir_Gier._addressRegVal =  PosDirect_addressRegR(&PosDir_Gier);
			PosDirect_resetMovingAvgExp(&PosDir_Gier);
			PosDir_Nick._clockWise = 0;
			PosDir_Nick._lastAngleRaw = 0.0;
			PosDir_Nick._zeroRegVal = PosDirect_zeroRegR(&PosDir_Nick);
			PosDir_Nick._addressRegVal = PosDirect_addressRegR(&PosDir_Nick);
			PosDirect_resetMovingAvgExp(&PosDir_Nick);
		}
		else
		{
			retVal = POSDI_FAILLED; // sensor not detected
		}
		return (retVal);
	}


	// Set / unset clock wise counting - sensor counts CCW natively
    // param int8_t cw - 1: CW, 0
	unsigned char PosDirect_setClockWise(PositionSens* psensor, uint8_t cw)
	{
		unsigned char retVal = POSDI_SUCCESS;
		psensor->_clockWise = cw;
		psensor->_lastAngleRaw = 0.0;
		PosDirect_resetMovingAvgExp(psensor);
		return (retVal);
	}

	// writes OTP control register
	// param unit8_t register value
	unsigned char PosDirect_progRegister(PositionSens* psensor, uint8_t regVal)
	{
		unsigned char retVal = POSDI_SUCCESS;
		retVal = PosDirect_writeReg(psensor, AS5048B_PROG_REG, regVal);
		return(retVal);
	}

	// Burn values to the slave address OTP register
	unsigned char PosDirect_doProg(PositionSens* psensor)
	{
		unsigned char retVal = POSDI_SUCCESS;
		//enable special programming mode
		retVal = PosDirect_progRegister(psensor, 0xFD);
		wait(10000);
		//set the burn bit: enables automatic programming procedure
		retVal |= PosDirect_progRegister(psensor,0x08);
		wait(10000);
		//disable special programming mode
		retVal |= PosDirect_progRegister(psensor,0x00);
		wait(10000);

		return(retVal);
	}

	// Burn values to the zero position OTP register
	unsigned char PosDirect_doProgZero(PositionSens* psensor)
	{
		unsigned char retVal = POSDI_SUCCESS;
		//this will burn the zero position OTP register like described in the datasheet
		//enable programming mode
		retVal = PosDirect_progRegister(psensor,0x01);
		wait(10000);
		//set the burn bit: enables automatic programming procedure
		retVal |= PosDirect_progRegister(psensor,0x08);
		wait(10000);
		//read angle information (equals to 0)
		retVal |= PosDirect_readReg16(psensor,AS5048B_ANGLMSB_REG);
		wait(10000);
		//enable verification
		retVal |= PosDirect_progRegister(psensor,0x40);
		wait(10000);
		//read angle information (equals to 0)
		retVal |= PosDirect_readReg16(psensor,AS5048B_ANGLMSB_REG);
		wait(10000);

		return(retVal);
	}

	// write I2C address value (5 bits) into the address register
	unsigned char PosDirect_addressRegW(PositionSens* psensor, uint8_t regVal)
	{
		unsigned char retVal = POSDI_SUCCESS;
		// write the new chip address to the register
		retVal = PosDirect_writeReg(psensor,AS5048B_ADDR_REG, regVal);
		// update our chip address with our 5 programmable bits
		// the MSB is internally inverted, so we flip the leftmost bit
		psensor->_chipAddress = ((regVal << 2) | (psensor->_chipAddress & 0b11)) ^ (1 << 6);
		return(retVal);
	}

	// reads I2C address register value
	uint8_t PosDirect_addressRegR(PositionSens* psensor)
	{
		return PosDirect_readReg8(psensor, AS5048B_ADDR_REG);
	}


	// sets current angle as the zero position
	unsigned char PosDirect_setZeroReg(PositionSens* psensor)
	{
		unsigned char retVal = POSDI_SUCCESS;
		retVal = PosDirect_zeroRegW(psensor, (uint16_t) 0x00); //Issue closed by @MechatronicsWorkman and @oilXander. The last sequence avoids any offset for the new Zero position
		uint16_t newZero =  PosDirect_readReg16(psensor, AS5048B_ANGLMSB_REG);
		retVal |= PosDirect_zeroRegW(psensor, newZero);
		return(retVal);
	}

	//writes the 2 bytes Zero position register value
	unsigned char PosDirect_zeroRegW(PositionSens* psensor, uint16_t regVal)
	{
		unsigned char retVal = POSDI_SUCCESS;
		retVal =  PosDirect_writeReg(psensor, AS5048B_ZEROMSB_REG, (uint8_t) (regVal >> 6));
		retVal |= PosDirect_writeReg(psensor, AS5048B_ZEROLSB_REG, (uint8_t) (regVal & 0x3F));
		return(retVal);
	}

	//reads the 2 bytes Zero position register value trimmed on 14 bits
	uint16_t  PosDirect_zeroRegR(PositionSens* psensor)
	{
		return (PosDirect_readReg16(psensor, AS5048B_ZEROMSB_REG));
	}

    //reads the 2 bytes magnitude register value trimmed on 14 bits
	uint16_t  PosDirect_magnitudeR(PositionSens* psensor)
	{
		return (PosDirect_readReg16(psensor, AS5048B_MAGNMSB_REG));
	}

	//reads the 2 bytes angle register value trimmed on 14 bits
	uint16_t  PosDirect_angleRegR(PositionSens* psensor)
	{
		return (PosDirect_readReg16(psensor, AS5048B_ANGLMSB_REG));
	}

	//reads the 1 bytes auto gain register value
	uint8_t  PosDirect_getAutoGain(PositionSens* psensor)
	{

		return (PosDirect_readReg8(psensor, AS5048B_GAIN_REG));
	}

	//reads the 1 bytes diagnostic register value
	uint8_t  PosDirect_getDiagReg(PositionSens* psensor)
	{
		return (PosDirect_readReg8(psensor, AS5048B_DIAG_REG));
	}

	//reads current angle value and converts it into the desired unit
	//param 2 String unit : string expressing the unit of the angle. Sensor raw value as default
	//param 3 uint8_t newVal : have a new measurement (1) or use the last read one (0).
	//returns Double angle value converted into the desired unit
	double PosDirect_angleR(PositionSens* psensor, int unit, uint8_t newVal)
	{
		double angleRaw;
		if (newVal == 1)
		{
			if(psensor->_clockWise == 1)
			{
				angleRaw = (double) (0b11111111111111 - PosDirect_readReg16(psensor, AS5048B_ANGLMSB_REG));
			}
			else
			{
				angleRaw = (double) PosDirect_readReg16(psensor, AS5048B_ANGLMSB_REG);
			}
			psensor->_lastAngleRaw = angleRaw;
		}
		else
		{
			angleRaw = psensor->_lastAngleRaw;
		}
		return (PosDirect_convertAngle(unit, angleRaw));
	}

	//Performs an exponential moving average on the angle.
	//Works on Sine and Cosine of the angle to avoid issues 0°/360° discontinuity
	void PosDirect_updateMovingAvgExp(PositionSens* psensor)
	{
		//sine and cosine calculation on angles in radian
		double angle = PosDirect_angleR(psensor, U_RAD, 1);
		if (psensor->_movingAvgCountLoop < EXP_MOVAVG_LOOP)
		{
			psensor->_movingAvgExpSin += sin(angle);
			psensor->_movingAvgExpCos += cos(angle);
			if (psensor->_movingAvgCountLoop == (EXP_MOVAVG_LOOP - 1))
			{
				psensor->_movingAvgExpSin = psensor->_movingAvgExpSin / EXP_MOVAVG_LOOP;
				psensor->_movingAvgExpCos = psensor->_movingAvgExpCos / EXP_MOVAVG_LOOP;
			}
			psensor->_movingAvgCountLoop ++;
		}
		else
		{
			double movavgexpsin = psensor->_movingAvgExpSin + psensor->_movingAvgExpAlpha * (sin(angle) - psensor->_movingAvgExpSin);
			double movavgexpcos = psensor->_movingAvgExpCos + psensor->_movingAvgExpAlpha * (cos(angle) - psensor->_movingAvgExpCos);
			psensor->_movingAvgExpSin = movavgexpsin;
			psensor->_movingAvgExpCos = movavgexpcos;
			psensor->_movingAvgExpAngle =  PosDirect_getExpAvgRawAngle(psensor);
		}
	}

	//sent back the exponential moving averaged angle in the desired unit
	//param 2 String unit : string expressing the unit of the angle. Sensor raw value as default
	//returns Double exponential moving averaged angle value
	double PosDirect_getMovingAvgExp(PositionSens* psensor, int unit)
	{
		return (PosDirect_convertAngle(unit, psensor->_movingAvgExpAngle));
	}

	void PosDirect_resetMovingAvgExp(PositionSens* psensor)
	{
		psensor->_movingAvgExpAngle = 0.0;
		psensor->_movingAvgCountLoop = 0;
		psensor->_movingAvgExpAlpha = 2.0 / (EXP_MOVAVG_N + 1.0);
	}

	uint8_t PosDirect_readReg8(PositionSens* psensor, uint8_t address)
	{
		uint8_t readValue = 0;
		if((psensor == &PosDir_Gier)&&(posDirDeviceGier >0))
		{
			readValue = wiringPiI2CReadReg8(posDirDeviceGier, address);
		}
		else if((psensor == &PosDir_Nick)&&(posDirDeviceNick >0))
		{
			readValue = wiringPiI2CReadReg8(posDirDeviceNick, address);
		}
		return readValue;
	}

	uint16_t  PosDirect_readReg16(PositionSens* psensor, uint8_t address)
	{
		//16 bit value got from 2 8bits registers (7..0 MSB + 5..0 LSB) => 14 bits value

		uint8_t readArray[2];
		uint16_t readValue = 0;

		if((psensor == &PosDir_Gier)&&(posDirDeviceGier >0))
		{
			readArray[0] = wiringPiI2CReadReg8(posDirDeviceGier, address);
			wait(10000);
			readArray[1] = wiringPiI2CReadReg8(posDirDeviceGier, address + 1);
		}
		else if((psensor == &PosDir_Nick)&&(posDirDeviceNick >0))
		{
			readArray[0] = wiringPiI2CReadReg8(posDirDeviceNick, address);
			wait(10000);
			readArray[1] = wiringPiI2CReadReg8(posDirDeviceNick, address + 1);
		}

		readValue = (((uint16_t) readArray[0]) << 6);
		readValue += (readArray[1] & 0x3F);
		/*
		Serial.println(readArray[0], BIN);
		Serial.println(readArray[1], BIN);
		Serial.println(readValue, BIN);
		*/
		return (readValue);
	}

	unsigned char PosDirect_writeReg(PositionSens* psensor, uint8_t address, uint8_t value)
	{
		unsigned char retVal = POSDI_SUCCESS;
		if((psensor == &PosDir_Gier)&&(posDirDeviceGier >0))
		{
			wiringPiI2CWriteReg8 (posDirDeviceGier, address, value);
		}
		else if((psensor == &PosDir_Nick)&&(posDirDeviceNick >0))
		{
			wiringPiI2CWriteReg8 (posDirDeviceNick, address, value);
		}
		else
		{
			retVal = POSDI_FAILLED;
		}
		return(retVal);
	}

	double PosDirect_convertAngle(int unit, double angle)
	{
		// convert raw sensor reading into angle unit
		double angleConv;
		switch (unit)
		{
			case U_RAW:
				//Sensor raw measurement
				angleConv = angle;
				break;
			case U_TRN:
				//full turn ratio
				angleConv = (angle / AS5048B_RESOLUTION);
				break;
			case U_DEG:
				//degree
				angleConv = (angle / AS5048B_RESOLUTION) * 360.0;
				break;
			case U_RAD:
				//Radian
				angleConv = (angle / AS5048B_RESOLUTION) * 2 * M_PI;
				break;
			case U_MOA:
				//minute of arc
				angleConv = (angle / AS5048B_RESOLUTION) * 60.0 * 360.0;
				break;
			case U_SOA:
				//second of arc
				angleConv = (angle / AS5048B_RESOLUTION) * 60.0 * 60.0 * 360.0;
				break;
			case U_GRAD:
				//grade
				angleConv = (angle / AS5048B_RESOLUTION) * 400.0;
				break;
			case U_MILNATO:
				//NATO MIL
				angleConv = (angle / AS5048B_RESOLUTION) * 6400.0;
				break;
			case U_MILSE:
				//Swedish MIL
				angleConv = (angle / AS5048B_RESOLUTION) * 6300.0;
				break;
			case U_MILRU:
				//Russian MIL
				angleConv = (angle / AS5048B_RESOLUTION) * 6000.0;
				break;
			default:
				//no conversion => raw angle
				angleConv = angle;
				break;
		}
		return (angleConv);
	}

	double PosDirect_getExpAvgRawAngle(PositionSens* psensor)
	{

		double angle;
		double twopi = 2 * M_PI;

		if (psensor->_movingAvgExpSin < 0.0)
		{
			angle = twopi - acos(psensor->_movingAvgExpCos);
		}
		else
		{
			angle = acos(psensor->_movingAvgExpCos);
		}
		angle = (angle / twopi) * AS5048B_RESOLUTION;
		return (angle);
	}

	void PosDirect_printDebug(PositionSens* psensor)
	{
	}
