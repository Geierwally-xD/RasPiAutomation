#ifndef PIGPIO_ADAPTER_H
#define PIGPIO_ADAPTER_H

#define RPI_MODE_INPUT 0
#define RPI_MODE_OUTPUT 1

const char *RPI_LastError(void);
int RPI_Initialize(void);
void RPI_Terminate(void);
int RPI_SetMode(int pin, int mode);
int RPI_Write(int pin, int value);
int RPI_Read(int pin);
int RPI_I2COpen(int address);
int RPI_I2CClose(int handle);
int RPI_I2CReadReg8(int handle, int reg);
int RPI_I2CWriteReg8(int handle, int reg, int value);
int RPI_ServoPulse(int pin, unsigned pulseWidthMicros);
int RPI_ServoOff(int pin);

#endif