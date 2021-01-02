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
#include "system.h"

int uart0_filestream = -1;
int streamlength = 0;
char versionString[]="                    ";
char testString[]="                    ";
int volumeRequest[4] = {0,0,0,0};  // requested profile volume
int volumeState[4] = {0,0,0,0};    // volume state of all audio mix channels
int volumeSteps[4] = {0,0,0,0};    // volume steps to fade up/down


AC_Data aC_Data;

// write active audio channel into datfile
unsigned char AC_writeDatFile(void)
{
	unsigned char result = AC_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("AC_Dat.bin","wb");  // w for write, b for binary
	if(write_ptr != NULL)
	{
		fwrite(&aC_Data,sizeof(aC_Data),1,write_ptr); // write dat file
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

	read_ptr = fopen("AC_Dat.bin","rb");  // r for read, b for binary
	if(read_ptr != NULL)
	{
		fread(&aC_Data,sizeof(aC_Data),1,read_ptr); // read dat file
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
	aC_Data.activeAudioProfile = 0x01; // activate audio profile 1 (sumary signal)
	for(int i =0; i<10; i++)
	{
		for(int j =0; j<4; j++)
		{
			aC_Data.audioProfiles[i][j] = 0;
		}
	}
	// not necessary result |= AC_Execute((unsigned char)ACTIVE + 0x0f); // activate audio channels 1, 2, 3 and 4
	result |= AC_readDatFile(); // read in last active audio channel from file 

    return(result);
}


// open  audio control interface
unsigned char AC_open(void)
{
	unsigned char result = AC_SUCCESS;

	if(uart0_filestream <=0)
	{
		uart0_filestream = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY /*| O_NDELAY*/);
		if (uart0_filestream == -1)
		{
			result = AC_COM_FAILED;  // AC COM connection failure
		}
		else
		{
			struct termios options;
			tcgetattr(uart0_filestream, &options);
				options.c_cflag = B9600 | CS8 |CSTOPB| CLOCAL | CREAD;
				options.c_iflag = IGNPAR;
				options.c_oflag = 0;
				options.c_lflag = 0;
				options.c_cc[VTIME]=10;
				tcflush(uart0_filestream, TCIFLUSH);
				tcsetattr(uart0_filestream, TCSANOW, &options);

		}
	}
    return(result);
}

// write bytestream to COM Port
unsigned char AC_write(char* buffer, int length)
{
	unsigned char result = AC_SUCCESS;
    streamlength = write(uart0_filestream,buffer, length); //write buffer to COM Port
    printf("write  %s \n",buffer); //rvtest
    if(streamlength < 0)
	{
       result = AC_TX_FAILED;
    }
	return(result);
}

// read bytestream from COM Port
unsigned char AC_read(char* buffer, int length)
{
	unsigned char result = AC_SUCCESS;
	int bytes_read = 0;
	systemtimer timeoutTimer;
	streamlength = 0;
	memset(buffer,0,10);
	startMeasurement(&timeoutTimer); // start on signal timer
	while(streamlength <9)
	{
		bytes_read = read(uart0_filestream,&buffer[streamlength], length); //read buffer from COM Port
		if(bytes_read >0)
		{
			streamlength += bytes_read;
			length -= bytes_read;
		}
		if(isExpired(1000000,&timeoutTimer)== 1)
		{
			result |= AC_RX_TIMEOUT;
			break;
		}
	}
    printf("read   %s \n",buffer); //rvtest
	if(streamlength < 0)
	{
		result |= AC_RX_FAILED;
	}
	return(result);
}

unsigned char AC_Teach(int index)
{
	unsigned char result = AC_SUCCESS;
	unsigned char tempDat = 0;
	aC_Data.activeAudioProfile = (unsigned char)((index >> 16) & 0x0f); // index of audio profile
	tempDat = (unsigned char)((index >> 12) & 0x0f);                    // audio channel 4
	aC_Data.audioProfiles[aC_Data.activeAudioProfile][3] = (unsigned char)(SYSTEM_map(tempDat,0,10, 0, 63));
	tempDat = (unsigned char)((index >> 8) & 0x0f);                     // audio channel 3
	aC_Data.audioProfiles[aC_Data.activeAudioProfile][2] = (unsigned char)(SYSTEM_map(tempDat,0,10, 0, 63));
	tempDat = (unsigned char)((index >> 4) & 0x0f);                     // audio channel 2
	aC_Data.audioProfiles[aC_Data.activeAudioProfile][1] = (unsigned char)(SYSTEM_map(tempDat,0,10, 0, 63));
	tempDat = (unsigned char)(index & 0x0f);                            // audio channel 1
	aC_Data.audioProfiles[aC_Data.activeAudioProfile][0] = (unsigned char)(SYSTEM_map(tempDat,0,10, 0, 63));
	result = AC_writeDatFile(); // write teach data into binary file
	printf("Teachdata = 1= %d 2= %d 3= %d 4= %d\n",aC_Data.audioProfiles[aC_Data.activeAudioProfile][0],aC_Data.audioProfiles[aC_Data.activeAudioProfile][1],aC_Data.audioProfiles[aC_Data.activeAudioProfile][2],aC_Data.audioProfiles[aC_Data.activeAudioProfile][3]);//rvtest
	result |= AC_open(); // open COM Port
	result |= AC_Profile(aC_Data.activeAudioProfile); // execute the teached profile
	return(result);
}

// read requested volume for profile
unsigned char AC_Request(unsigned char index)
{
	unsigned char result = AC_SUCCESS;
	if(index < 10)
	{
		volumeRequest[0] = aC_Data.audioProfiles[index][0]; // audio channel 1
		volumeRequest[1] = aC_Data.audioProfiles[index][1]; // audio channel 1
		volumeRequest[2] = aC_Data.audioProfiles[index][2]; // audio channel 1
		volumeRequest[3] = aC_Data.audioProfiles[index][3]; // audio channel 1
		printf("Requested Profile data from index %d : 1= %d 2= %d 3= %d 4= %d\n",index,aC_Data.audioProfiles[index][0],aC_Data.audioProfiles[index][1],aC_Data.audioProfiles[index][2],aC_Data.audioProfiles[index][3]);//rvtest
	}
	else
	{
		result = AC_OUT_OF_RANGE;
	}
	return(result);
}

// get volume state from audio mix
unsigned char AC_GetVolume(void)
{
	char * commandString = 0;
	int commandlength = 0;
	char volString[]="           ";
	unsigned char result = AC_SUCCESS;

	// read audiomix channel 1 volume
	commandString = &VOL1[0];    // init seqence set active all channels
	commandlength = sizeof(VOL1);
	result |=  AC_write(&commandString[0],commandlength);  // send first command sequence to Audiomix
	wait(30000);
    result |=  AC_read(&volString[0],9);
    volString[8] = 0;
    volumeState[0] = atoi(&volString[6]);
    if((volumeState[0] > 63)||(volumeState[0] < 0)) // range check
    {
      volumeState[0] = 0;
    }
	// read audiomix channel 2 volume
	commandString = &VOL2[0];    // init seqence set active all channels
	commandlength = sizeof(VOL2);
	result |=  AC_write(&commandString[0],commandlength);  // send first command sequence to Audiomix
	wait(30000);
    result |=  AC_read(&volString[0],9);
    volString[8] = 0;
    volumeState[1] = atoi(&volString[6]);
    if((volumeState[1] > 63)||(volumeState[1] < 0)) // range check
    {
      volumeState[1] = 0;
    }
	// read audiomix channel 3 volume
	commandString = &VOL3[0];    // init seqence set active all channels
	commandlength = sizeof(VOL3);
	result |=  AC_write(&commandString[0],commandlength);  // send first command sequence to Audiomix
	wait(30000);
    result |=  AC_read(&volString[0],9);
    volString[8] = 0;
    volumeState[2] = atoi(&volString[6]);
    if((volumeState[2] > 63)||(volumeState[2] < 0)) // range check
    {
      volumeState[2] = 0;
    }
	// read audiomix channel 4 volume
	commandString = &VOL4[0];    // init seqence set active all channels
	commandlength = sizeof(VOL4);
	result |=  AC_write(&commandString[0],commandlength);  // send first command sequence to Audiomix
	wait(30000);
    result |=  AC_read(&volString[0],9);
    volString[8] = 0;
    volumeState[3] = atoi(&volString[6]);
    if((volumeState[3] > 63)||(volumeState[3] < 0)) // range check
    {
      volumeState[3] = 0;
    }
	printf("GetVolume = 1= %d 2= %d 3= %d 4= %d\n",volumeState[0],volumeState[1],volumeState[2],volumeState[3]);//rvtest
	return(result);
}

// step audio mix to new profile volumes
unsigned char AC_Control(void)
{
	unsigned char result = AC_SUCCESS;
	char * commandString = &SET_VOLUME[0];
	int commandlength = sizeof(SET_VOLUME);
	unsigned char profileFade = 0;
	systemtimer timeoutTimer;

	startMeasurement(&timeoutTimer); // start timeout timer
	do
	{
		profileFade = 0;
		if(volumeSteps[0] != volumeRequest[0])
		{
			if(volumeSteps[0] > volumeRequest[0])
			{
				volumeSteps[0]--;
			}
			else if(volumeSteps[0] < volumeRequest[0])
			{
				volumeSteps[0]++;
			}
			commandString[7] = '1';
			sprintf(&commandString[12],"%02d)",volumeSteps[0]);
			result |=  AC_write(&commandString[0],commandlength);  // send step fade up to audiomix
			profileFade |=1;
			wait(30000); //wait 30 ms
			commandString[7] = 'x';
		}

		if(volumeSteps[1] != volumeRequest[1])
		{
			if(volumeSteps[1] > volumeRequest[1])
			{
				volumeSteps[1]--;
			}
			else if(volumeSteps[1] < volumeRequest[1])
			{
				volumeSteps[1]++;
			}
			commandString[8] = '2';
			sprintf(&commandString[12],"%02d)",volumeSteps[1]);
			result |=  AC_write(&commandString[0],commandlength);  // send step fade up to audiomix
			profileFade |=1;
			wait(30000); //wait 30 ms
			commandString[8] = 'x';
		}

		if(volumeSteps[2] != volumeRequest[2])
		{
			if(volumeSteps[2] > volumeRequest[2])
			{
				volumeSteps[2]--;
			}
			else if(volumeSteps[2] < volumeRequest[2])
			{
				volumeSteps[2]++;
			}
			commandString[9] = '3';
			sprintf(&commandString[12],"%02d)",volumeSteps[2]);
			result |=  AC_write(&commandString[0],commandlength);  // send step fade up to audiomix
			profileFade |=1;
			wait(30000); //wait 30 ms
			commandString[9] = 'x';
		}

		if(volumeSteps[3] != volumeRequest[3])
		{
			if(volumeSteps[3] > volumeRequest[3])
			{
				volumeSteps[3]--;
			}
			else if(volumeSteps[3] < volumeRequest[3])
			{
				volumeSteps[3]++;
			}
			commandString[10] = '4';
			sprintf(&commandString[12],"%02d)",volumeSteps[3]);
			result |=  AC_write(&commandString[0],commandlength);  // send step fade up to audiomix
			profileFade |=1;
			wait(30000); //wait 30 ms
			commandString[10] = 'x';
		}
		if(isExpired(20000000,&timeoutTimer)== 1)
		{
			result |= AC_RX_TIMEOUT;
			break;
		}
	}while(profileFade >0);
	return(result);
}

// execute audiomix profile
unsigned char AC_Profile(unsigned char index)
{
	unsigned char result = AC_SUCCESS;
	if(index == 0)
	{
		result |=  AC_write(&RESET[0],sizeof(RESET));  // send first command sequence to Audiomix
	}

	if(result == AC_SUCCESS)
	{
		result|= AC_Request(index);    // read requested volume for profile
	}
	if(result == AC_SUCCESS)
	{
		result|= AC_GetVolume();  // get volume state from audio mix
	}
	if(result == AC_SUCCESS)
	{
	    volumeSteps[0] = volumeState[0];   // set steps to fade up / down for channel 1
	    volumeSteps[1] = volumeState[1];   // set steps to fade up / down for channel 2
	    volumeSteps[2] = volumeState[2];   // set steps to fade up / down for channel 3
	    volumeSteps[3] = volumeState[3];   // set steps to fade up / down for channel 4
		result|= AC_Control();    // step audio mix to new profile volumes
	}

	if(uart0_filestream >0)
	{
	    close(uart0_filestream); // close COM Port
	}

	return(result);
}

// execute audiomix command
unsigned char AC_Execute(unsigned char index)
{
	unsigned char result = AC_SUCCESS;
	unsigned char command = index & 0xf0;
	unsigned char channel = index & 0x0f;
	char temp = '*';
	char * commandString = 0;
	int commandlength = 0;

	result = AC_open(); // open COM Port
	switch (command)
	{
		case ACTIVE:
			commandString = &SET_ACTIVE[0];    // init seqence set active all channels
			commandlength = sizeof(SET_ACTIVE);
		break;
		case VOLUME:
			commandString = &SET_VOLUME[0];
			commandlength = sizeof(SET_VOLUME);
		break;
		case FADEUP:
			commandString = &FADE_UP[0];       // fader sequence fade up set channel and fade down rest of channels
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
		case AUDIO_PROFILE:
			return(AC_Profile(channel));      // execute audio profile
		break;
		default:
			commandString = &FADE_STOP[0];
			commandlength = sizeof(FADE_STOP);
			result = AC_EXEC_FAILED;  //undefined command
		break;
	}
	temp = commandString[7];

// channel 1 settings
	if(channel & 0x1)
	{
		commandString[7] = '1'; // audiochannel 1 is set
	}

// channel 2 settings
	if(channel & 0x2)
	{
		commandString[8] = '2'; // audiochannel 2 is set
	}

// channel 3 settings
	if(channel & 0x4)
	{
		commandString[9] = '3'; // audiochannel 3 is set
	}

// channel 4 settings
	if(channel & 0x8)
	{
		commandString[10] = '4'; // audiochannel 4 is set
	}

	if(result == AC_SUCCESS)
	{
		result |=  AC_write(&commandString[0],commandlength);  // send first command sequence to Audiomix
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
