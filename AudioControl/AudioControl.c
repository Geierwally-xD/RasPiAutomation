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
#include "timer.h"

int uart0_filestream = -1;
int streamlength = 0;
char versionString[]="                    ";
char testString[]="                    ";


AC_Data aC_Data;

// write active audio channel into datfile
unsigned char AC_writeDatFile(void)
{
	unsigned char result = AC_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("CI_Dat.bin","wb");  // w for write, b for binary
	if(write_ptr != NULL)
	{
		fwrite(&aC_Data,sizeof(aC_Data),1,write_ptr);
		fclose(write_ptr);
	}
	else
	{
		result = AC_WRITE_FAILED;
	}
	return(result);
}

// read and set active audio channel from datfile
unsigned char AC_readDatFile(void)
{
	unsigned char result = AC_SUCCESS;
	FILE *read_ptr;

	read_ptr = fopen("CI_Dat.bin","rb");  // r for read, b for binary
	if(read_ptr != NULL)
	{
		fread(&aC_Data,sizeof(aC_Data),1,read_ptr); // read 10 bytes to our buffer
		fclose(read_ptr);
	}
	else
	{
		result = AC_READ_FAILED;
	}
	return(result);
}

unsigned char ACinit(void)
{
	unsigned char result = AC_SUCCESS;
	aC_Data.activeAudioChannel = 0x02; // activate audio channel 2 (sumary signal) default
	// not necessary result |= AC_Execute((unsigned char)ACTIVE + 0x0f); // activate audio channels 1, 2, 3 and 4
	result |= AC_readDatFile(); // read in last active audio channel from file 
    return(result);
}


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
	unsigned char sequence = AC_SINGLE_SEQUENCE;
	char temp = '-';
	char temp2 = 'x';
	char * commandString = 0;
	char * commandString2 = 0;
	int commandlength = 0;
	int commandlength2 = 0;

	switch (command)
	{
		case ACTIVE:
			commandString = &SET_ACTIVE[0];    // init sequence reset, set active, set volume
			commandlength = sizeof(SET_ACTIVE);
			commandString2 = &SET_VOLUME[0];
			commandlength2 = sizeof(SET_VOLUME);
			sequence = AC_INIT_SEQUENCE;
		break;
		case VOLUME:
			commandString = &SET_VOLUME[0];
			commandlength = sizeof(SET_VOLUME);
		break;
		case FADEUP:
			commandString = &FADE_UP[0];       // fader sequence fade up set channel and fade down rest of channels
			commandlength = sizeof(FADE_UP);
			commandString2 = &FADE_DOWN[0];
			commandlength2 = sizeof(FADE_DOWN);
			sequence = AC_FADER_SEQUENCE;
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
	if(sequence > AC_SINGLE_SEQUENCE)
	{
		temp2 = commandString2[7];
	}

// channel 1 settings
	if(channel & 0x1)
	{
		commandString[7] = '1'; // audiochannel 1 is set
		if(sequence == AC_INIT_SEQUENCE)
		{
			commandString2[7] = '1'; // for set volume and fade up commands depending sequence
		}
	}
	else
	{
		if(sequence == AC_FADER_SEQUENCE)
		{
			commandString2[7] = '1'; // for fade down command
		}
	}

// channel 2 settings
	if(channel & 0x2)
	{
		commandString[8] = '2'; // audiochannel 2 is set
		if(sequence == AC_INIT_SEQUENCE)
		{
			commandString2[8] = '2'; // for set volume and fade up commands depending sequence
		}
	}
	else
	{
		if(sequence == AC_FADER_SEQUENCE)
		{
			commandString2[8] = '2'; // for fade down command
		}
	}

// channel 3 settings
	if(channel & 0x4)
	{
		commandString[9] = '3'; // audiochannel 3 is set
		if(sequence == AC_INIT_SEQUENCE)
		{
			commandString2[9] = '3'; // for set volume and fade up commands depending sequence
		}
	}
	else
	{
		if(sequence == AC_FADER_SEQUENCE)
		{
			commandString2[9] = '3'; // for fade down command
		}
	}

// channel 4 settings
	if(channel & 0x8)
	{
		commandString[10] = '4'; // audiochannel 4 is set
		if(sequence == AC_INIT_SEQUENCE)
		{
			commandString2[10] = '4'; // for set volume and fade up commands depending sequence
		}
	}
	else
	{
		if(sequence == AC_FADER_SEQUENCE)
		{
			commandString2[10] = '4'; // for fade down command
		}
	}

	result = AC_open(); // open COM Port
	if(result == AC_SUCCESS)
	{   // this part is only necessary only for maintenance to get firmware version of audiomix
		/*if(sequence == AC_INIT_SEQUENCE)
		   {  reset Audiomix on set active command
			  result |=  AC_write(RESET,sizeof(RESET));
			  wait(100000); // wait 100 ms before sending next command sequence to COM port
			  result |=  AC_GetVersion();
			  wait 20000); // wait 20 ms before sending next command sequence to COM port
		   }*/

		result |=  AC_write(&commandString[0],commandlength);  // send first command sequence to Audiomix
		if(sequence > AC_SINGLE_SEQUENCE)
		{
			wait(100000); // wait 100 ms before sending next command sequence to COM port
			result |=  AC_write(&commandString2[0],commandlength2); // send second commandd sequence to Audiomix
		}
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
	if(sequence > AC_SINGLE_SEQUENCE)
	{
		commandString2[7] = temp2;
		commandString2[8] = temp2;
		commandString2[9] = temp2;
		commandString2[10] = temp2;
	}
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
