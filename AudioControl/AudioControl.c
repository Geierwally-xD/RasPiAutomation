/*==============================================================================

 Name:           AudioControl.c
 Description:    functionality of Audioswitch control driver
 Copyright:      Geierwally, 2020(c)

==============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "AudioControl.h"
#include "AudioCommands.h"

int uart0_filestream = -1;
int streamlength = 0;
char versionString[]="                    ";
char testString[]="                    ";

// open  audio control interface
unsigned char AC_open(void)
{
	unsigned char result = AC_SUCCESS;
	uart0_filestream = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (uart0_filestream == -1)
	{
		result = AC_COM_FAILED;  // AC COM connection failure
	}
	else
	{
		struct termios options;
		tcgetattr(uart0_filestream, &options);
			options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
			options.c_iflag = IGNPAR;
			options.c_oflag = 0;
			options.c_lflag = 0;
		tcflush(uart0_filestream, TCIFLUSH);
		tcsetattr(uart0_filestream, TCSANOW, &options);
	}
    return(result);
}

// write bytestream to COM Port
unsigned char AC_write(char* buffer, int length)
{
	unsigned char result = AC_open(); // open COM Port
	if (result==AC_SUCCESS)
	{
	   streamlength = write(uart0_filestream,buffer, length); //write buffer to COM Port
	   if(streamlength < 0)
	   {
		   result = AC_TX_FAILED;
	   }

	}
	return(result);
}

// read bytestream to COM Port
unsigned char AC_read(char* buffer, int length)
{
	unsigned char result = AC_SUCCESS;
	streamlength = read(uart0_filestream,buffer, length); //write buffer to COM Port
	if(streamlength < 0)
	{
		result = AC_RX_FAILED;
	}
	return(result);
}


unsigned char AC_Execute(unsigned char index)
{
	unsigned char result = AC_SUCCESS;
	unsigned char command = index & 0xf0;
	unsigned char channel = index & 0x0f;
	unsigned char reset = 0;
	char temp = 'x';
	char * commandString = 0;
	int commandlength = 0;

	switch (command)
	{
		case ACTIVE:
			commandString = &SET_ACTIVE[0];
			commandlength = sizeof(SET_ACTIVE);
			reset = 1;
		break;
		case VOLUME:
			commandString = &SET_VOLUME[0];
			commandlength = sizeof(SET_VOLUME);
		break;
		case FADEUP:
			commandString = &FADE_UP[0];
			commandlength = sizeof(FADE_UP);
		break;
		case FADEDOWN:
			commandString = &FADE_DOWN[0];
			commandlength = sizeof(FADE_DOWN);
		break;
		case FADESTOP:
			commandString = &FADE_STOP[0];
			commandlength = sizeof(FADE_STOP);
		break;
		default:
			commandString = &FADE_STOP[0];
			commandlength = sizeof(FADE_STOP);
			result = AC_EXEC_FAILED;  //undefined command
		break;
	}
	temp = commandString[7];
	if(channel & 0x1)
	{
		commandString[7] = '1'; // audiochannel 1 is set
	}
	if(channel & 0x2)
	{
		commandString[8] = '2'; // audiochannel 2 is set
	}
	if(channel & 0x4)
	{
		commandString[9] = '3'; // audiochannel 3 is set
	}
	if(channel & 0x8)
	{
		commandString[10] = '4'; // audiochannel 4 is set
	}

	result = AC_open(); // open COM Port
	if(result == AC_SUCCESS)
	{
		if(reset == 1)
		{   // reset Audiomix on set active command
			result =  AC_write(RESET,sizeof(RESET));
			result =  AC_GetVersion();
		}
		result =  AC_write(&commandString[0],commandlength);

	}
	if(uart0_filestream >0)
	{
	    close(uart0_filestream); // close COM Port
	}
	// reset channels for next command
	commandString[7] = temp;
	commandString[8] = temp;
	commandString[9] = temp;
	commandString[10] = temp;


	return (result);
}


unsigned char AC_GetVersion (void)
{
	unsigned char result = AC_SUCCESS;

	result =  AC_write(GET_VERSION,sizeof(GET_VERSION));

	if(result == AC_SUCCESS)
	{
		result =  AC_read(&versionString[0],sizeof(versionString));
	}


	return(result);
}



unsigned char AC_Test (void)
{
	unsigned char result = AC_SUCCESS;
	result = AC_open(); // open COM Port

	result =  AC_write(MUTESTATE,sizeof(MUTESTATE));
	//result =  AC_write(VOL1,sizeof(VOL1));


	if(result == AC_SUCCESS)
	{
		result =  AC_read(&testString[0],sizeof(testString));
	}
	if(uart0_filestream >0)
	{
	    close(uart0_filestream); // close COM Port
	}
	return(result);
}
