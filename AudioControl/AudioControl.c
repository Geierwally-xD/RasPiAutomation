/*==============================================================================

 Name:           AudioControl.c
 Description:    functionality of Audioswitch control driver
 Copyright:      Geierwally, 2020(c)

==============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdarg.h>
#include <time.h>
#include "AudioControl.h"
#include "AudioCommands.h"
#include "AudioControlValidation.h"
#include "AudioControlWrite.h"
#include "timer.h"
#include "system.h"

#define AC_LOG_FILE "/home/pi/JokiAutomation/log/AudioControl.log"
#define AC_LOG_DIR "/home/pi/JokiAutomation/log"
#define AC_LOG_MAX_BYTES (5 * 1024 * 1024)
#define AC_VOLUME_RESPONSE_LENGTH 9
#define AC_VOLUME_RESPONSE_BUFFER_LENGTH (AC_VOLUME_RESPONSE_LENGTH + 1)
#define AC_VOLUME_MAX_ATTEMPTS 2
#define AC_PROFILE_SETTLE_US 100000

static void AC_RotateLogIfNeeded(const char *logPath)
{
	struct stat st;
	char log1[256];
	char log2[256];

	if (logPath == NULL)
	{
		return;
	}

	if (stat(logPath, &st) != 0)
	{
		return;
	}

	if (st.st_size < AC_LOG_MAX_BYTES)
	{
		return;
	}

	snprintf(log1, sizeof(log1), "%s.1", logPath);
	snprintf(log2, sizeof(log2), "%s.2", logPath);

	remove(log2);
	rename(log1, log2);
	rename(logPath, log1);
}

int uart0_filestream = -1;
int streamlength = 0;
char versionString[]="                    ";
char testString[]="                    ";
int volumeRequest[4] = {0,0,0,0};  // requested profile volume
int volumeState[4] = {0,0,0,0};    // volume state of all audio mix channels
int volumeSteps[4] = {0,0,0,0};    // volume steps to fade up/down


AC_Data aC_Data;

void AC_Log(const char *fmt, ...)
{
	FILE *fp;
	va_list args;
	time_t now;
	struct tm tm_info;
	char timebuf[32];
	struct stat dirStat;

	if (mkdir(AC_LOG_DIR, 0775) != 0 && errno != EEXIST)
	{
		return;
	}
	if (stat(AC_LOG_DIR, &dirStat) != 0 || !S_ISDIR(dirStat.st_mode) || chmod(AC_LOG_DIR, 0775) != 0)
	{
		return;
	}

	AC_RotateLogIfNeeded(AC_LOG_FILE);

	fp = fopen(AC_LOG_FILE, "a");
	if (fp == NULL)
	{
		return;
	}
	if (fchmod(fileno(fp), 0664) != 0)
	{
		fclose(fp);
		return;
	}

	time(&now);
	if (localtime_r(&now, &tm_info) != NULL)
	{
		strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_info);
		fprintf(fp, "[%s] ", timebuf);
	}

	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

	fprintf(fp, "\n");
	fclose(fp);
}

// write active audio channel into datfile
unsigned char AC_writeDatFile(void)
{
	unsigned char result = AC_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("/home/pi/JokiAutomation/config/AC_Dat.bin","wb");  // w for write, b for binary
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

	read_ptr = fopen("/home/pi/JokiAutomation/config/AC_Dat.bin","rb");  // r for read, b for binary
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

	if(uart0_filestream < 0)
	{
		uart0_filestream = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY /*| O_NDELAY*/);
		if (uart0_filestream < 0)
		{
			result = AC_COM_FAILED;  // AC COM connection failure
		}
		else
		{
			if (flock(uart0_filestream, LOCK_EX | LOCK_NB) != 0)
			{
				close(uart0_filestream);
				uart0_filestream = -1;
				return AC_COM_FAILED;
			}

			// configured based on https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
			struct termios options;
			tcgetattr(uart0_filestream, &options);
			options.c_cflag = B9600 | CS8 |CSTOPB| CLOCAL | CREAD;
			options.c_iflag = IGNPAR;
			options.c_oflag = 0;
			options.c_lflag = 0;
			options.c_cc[VTIME]= 0;//10;
			options.c_cc[VMIN] = 0;
			tcflush(uart0_filestream, TCIFLUSH);
			tcsetattr(uart0_filestream, TCSANOW, &options);
		}
	}
    return(result);
}

static void AC_close(void)
{
	if (uart0_filestream >= 0)
	{
		if (close(uart0_filestream) != 0)
		{
			AC_Log("AC_CLOSE_FAIL pid=%d errno=%d", getpid(), errno);
		}
		uart0_filestream = -1;
	}
}

static ptrdiff_t AC_SystemWrite(int fd, const char *buffer, size_t length)
{
	return (ptrdiff_t)write(fd, buffer, length);
}

static unsigned char AC_writeAll(const char *buffer, size_t length)
{
	size_t written = 0;
	int errorNumber = 0;
	unsigned char result = AC_WriteAllWith(uart0_filestream, buffer, length,
		AC_SystemWrite, &written, &errorNumber);
	streamlength = (int)written;
	if (result == AC_SUCCESS)
	{
		AC_Log("AC_WRITE_OK requested=%zu written=%zu payload=%.*s", length, written,
			(int)length, buffer);
		return AC_SUCCESS;
	}
	AC_Log("AC_WRITE_FAIL requested=%zu written=%zu errno=%d payload=%.*s",
		length, written, errorNumber, (int)length, buffer);
	return result;
}

// write bytestream to COM Port
unsigned char AC_write(char* buffer, int length)
{
	if (buffer == NULL || length < 0 || uart0_filestream < 0)
	{
		errno = uart0_filestream < 0 ? EBADF : EINVAL;
		AC_Log("AC_WRITE_FAIL requested=%d written=0 errno=%d payload=%s",
			length, errno, buffer == NULL ? "<null>" : buffer);
		streamlength = 0;
		return AC_TX_FAILED;
    }
	return AC_writeAll(buffer, (size_t)length);
}

// read bytestream from COM Port
unsigned char AC_read(char* buffer, int length)
{
	unsigned char result = AC_SUCCESS;
	int bytes_read = 0;
	systemtimer timeoutTimer;
	streamlength = 0;
	if (buffer == NULL || length <= 0 || uart0_filestream < 0)
	{
		return AC_RX_FAILED;
	}
	memset(buffer, 0, (size_t)length);
	startMeasurement(&timeoutTimer); // start on signal timer
	while(streamlength < length)
	{
		bytes_read = (int)read(uart0_filestream, &buffer[streamlength], (size_t)(length - streamlength));
		if(bytes_read >0)
		{
			streamlength += bytes_read;
			if (memchr(buffer, ')', (size_t)streamlength) != NULL)
			{
				break;
			}
		}
		else if (bytes_read < 0 && errno != EINTR)
		{
			result = AC_RX_FAILED;
			break;
		}
		if(isExpired(1000000,&timeoutTimer)== 1)
		{
			AC_Log("AC_read timeout: pid=%d errno=%d streamlength=%d", getpid(), errno, streamlength);
			if (streamlength == 0)
			{
				result = AC_RX_TIMEOUT;
			}
			break;
		}
	}
	if (result == AC_SUCCESS && streamlength == 0)
	{
		result = AC_RX_TIMEOUT;
	}
	AC_Log("AC_READ bytes=%d payload=%.*s result=%u", streamlength, streamlength, buffer, result);
	return(result);
}

unsigned char AC_Teach(int index)
{
	unsigned char result = AC_SUCCESS;
	unsigned char tempDat = 0;
	unsigned char taughtProfile = (unsigned char)((index >> 16) & 0x0f);
	if (taughtProfile >= 10)
	{
		return AC_OUT_OF_RANGE;
	}
	tempDat = (unsigned char)((index >> 12) & 0x0f);                    // audio channel 4
	aC_Data.audioProfiles[taughtProfile][3] = (unsigned char)(SYSTEM_map(tempDat,0,10, 0, 63));
	tempDat = (unsigned char)((index >> 8) & 0x0f);                     // audio channel 3
	aC_Data.audioProfiles[taughtProfile][2] = (unsigned char)(SYSTEM_map(tempDat,0,10, 0, 63));
	tempDat = (unsigned char)((index >> 4) & 0x0f);                     // audio channel 2
	aC_Data.audioProfiles[taughtProfile][1] = (unsigned char)(SYSTEM_map(tempDat,0,10, 0, 63));
	tempDat = (unsigned char)(index & 0x0f);                            // audio channel 1
	aC_Data.audioProfiles[taughtProfile][0] = (unsigned char)(SYSTEM_map(tempDat,0,10, 0, 63));
	result = AC_writeDatFile(); // write teach data into binary file
	AC_Log("TEACH_DATA profile=%u values=%d,%d,%d,%d",
		taughtProfile,
		aC_Data.audioProfiles[taughtProfile][0],
		aC_Data.audioProfiles[taughtProfile][1],
		aC_Data.audioProfiles[taughtProfile][2],
		aC_Data.audioProfiles[taughtProfile][3]);
	if (result == AC_SUCCESS)
	{
		result = AC_open();
	}
	if (result == AC_SUCCESS)
	{
		result = AC_Profile(taughtProfile);
	}
	if (result == AC_SUCCESS)
	{
		result = AC_writeDatFile();
	}
	return(result);
}

// read requested volume for profile
unsigned char AC_Request(unsigned char index)
{
	unsigned char result = AC_SUCCESS;
	if(index < 10)
	{
		volumeRequest[0] = aC_Data.audioProfiles[index][0]; // audio channel 1
		if((volumeRequest[0] < 0)||(volumeRequest[0] > 63))    // plausibility check
		{
			volumeRequest[0] = 0;
		}
		volumeRequest[1] = aC_Data.audioProfiles[index][1]; // audio channel 1
		if((volumeRequest[1] < 0)||(volumeRequest[1] > 63))    // plausibility check
		{
			volumeRequest[1] = 0;
		}
		volumeRequest[2] = aC_Data.audioProfiles[index][2]; // audio channel 1
		if((volumeRequest[2] < 0)||(volumeRequest[2] > 63))    // plausibility check
		{
			volumeRequest[2] = 0;
		}
		volumeRequest[3] = aC_Data.audioProfiles[index][3]; // audio channel 1
		if((volumeRequest[3] < 0)||(volumeRequest[3] > 63))    // plausibility check
		{
			volumeRequest[3] = 0;
		}
		AC_Log("REQUESTED_PROFILE index=%u values=%d,%d,%d,%d",
			index,
			aC_Data.audioProfiles[index][0],
			aC_Data.audioProfiles[index][1],
			aC_Data.audioProfiles[index][2],
			aC_Data.audioProfiles[index][3]);
	}
	else
	{
		result = AC_OUT_OF_RANGE;
	}
	return(result);
}

static void AC_LogInvalidVolumeResponse(int channel, const char *response, int responseLength,
		const char *reason)
{
	char escaped[AC_VOLUME_RESPONSE_BUFFER_LENGTH * 4 + 1];
	size_t output = 0;
	for (int i = 0; i < responseLength && output + 1 < sizeof(escaped); ++i)
	{
		unsigned char value = (unsigned char)response[i];
		if (isprint(value) && value != '\\')
		{
			escaped[output++] = (char)value;
		}
		else if (output + 4 < sizeof(escaped))
		{
			int written = snprintf(&escaped[output], sizeof(escaped) - output, "\\x%02X", value);
			if (written > 0)
			{
				output += (size_t)written;
			}
		}
	}
	escaped[output] = '\0';
	AC_Log("AC_VOLUME_RESPONSE_INVALID expectedChannel=%d bytes=%d raw=%s reason=%s",
		channel, responseLength, escaped, reason);
}

static unsigned char AC_GetVolumeChannel(int channel, int *volume, int maxAttempts)
{
	char response[AC_VOLUME_RESPONSE_BUFFER_LENGTH];
	char *command;
	int commandLength;
	unsigned char result = AC_SUCCESS;
	const char *reason = NULL;

	switch (channel)
	{
		case 1: command = VOL1; commandLength = sizeof(VOL1); break;
		case 2: command = VOL2; commandLength = sizeof(VOL2); break;
		case 3: command = VOL3; commandLength = sizeof(VOL3); break;
		case 4: command = VOL4; commandLength = sizeof(VOL4); break;
		default: return AC_OUT_OF_RANGE;
	}

	for (int attempt = 1; attempt <= maxAttempts; ++attempt)
	{
		memset(response, 0, sizeof(response));
		if (tcflush(uart0_filestream, TCIFLUSH) != 0)
		{
			return AC_RX_FAILED;
		}
		result = AC_write(command, commandLength);
		if (result != AC_SUCCESS)
		{
			return result;
		}
		wait(30000);
		result = AC_read(response, sizeof(response));
		if (result == AC_SUCCESS &&
			AC_ParseVolumeResponse(response, (size_t)streamlength, channel, volume, &reason))
		{
			AC_Log("AC_VOLUME_RESPONSE_OK channel=%d value=%d raw=%.*s",
				channel, *volume, streamlength, response);
			return AC_SUCCESS;
		}
		if (result == AC_SUCCESS)
		{
			AC_LogInvalidVolumeResponse(channel, response, streamlength, reason);
			result = AC_INVALID_RESPONSE;
		}
		else
		{
			AC_LogInvalidVolumeResponse(channel, response, streamlength,
				result == AC_RX_TIMEOUT ? "incomplete response" : "receive failure");
		}
		if (result == AC_RX_FAILED)
		{
			return result;
		}
		if (attempt < maxAttempts)
		{
			AC_Log("AC_VOLUME_RETRY pid=%d channel=%d attempt=%d result=%u",
				getpid(), channel, attempt + 1, result);
		}
	}
	return result;
}

// get volume state from all four audio mix channels
unsigned char AC_GetVolume(void)
{
	unsigned char result = AC_SUCCESS;
	for (int channel = 1; channel <= 4; ++channel)
	{
		int value = 0;
		result = AC_GetVolumeChannel(channel, &value, AC_VOLUME_MAX_ATTEMPTS);
		if (result != AC_SUCCESS)
		{
			return result;
		}
		volumeState[channel - 1] = value;
		AC_Log("AC_GetVolume CH%d=%d", channel, value);
	}
	AC_Log("GET_VOLUME profile=%u state=%d,%d,%d,%d",
		aC_Data.activeAudioProfile,
		volumeState[0], volumeState[1], volumeState[2], volumeState[3]);
	return AC_SUCCESS;
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
		if(result == AC_SUCCESS && volumeSteps[0] != volumeRequest[0])
		{
			if((volumeSteps[0] > volumeRequest[0])&&(volumeSteps[0] >0))
			{
				volumeSteps[0]--;
			}
			else if((volumeSteps[0] < volumeRequest[0])&&(volumeSteps[0] < 63))
			{
				volumeSteps[0]++;
			}
			else
			{
				volumeRequest[0] = volumeSteps[0];
			}
			commandString[7] = '1';
			sprintf(&commandString[12],"%02d)",volumeSteps[0]);
			result = AC_write(&commandString[0],commandlength);
			profileFade |=1;
			wait(30000); //wait 30 ms
			commandString[7] = 'x';
		}

		if(result == AC_SUCCESS && volumeSteps[1] != volumeRequest[1])
		{
			if((volumeSteps[1] > volumeRequest[1])&&(volumeSteps[1] >0))
			{
				volumeSteps[1]--;
			}
			else if((volumeSteps[1] < volumeRequest[1])&&(volumeSteps[1] < 63))
			{
				volumeSteps[1]++;
			}
			else
			{
				volumeRequest[1] = volumeSteps[1];
			}
			commandString[8] = '2';
			sprintf(&commandString[12],"%02d)",volumeSteps[1]);
			result = AC_write(&commandString[0],commandlength);
			profileFade |=1;
			wait(30000); //wait 30 ms
			commandString[8] = 'x';
		}

		if(result == AC_SUCCESS && volumeSteps[2] != volumeRequest[2])
		{
			if((volumeSteps[2] > volumeRequest[2])&&(volumeSteps[2] >0))
			{
				volumeSteps[2]--;
			}
			else if((volumeSteps[2] < volumeRequest[2])&&(volumeSteps[2] < 63))
			{
				volumeSteps[2]++;
			}
			else
			{
				volumeRequest[2] = volumeSteps[2];
			}

			commandString[9] = '3';
			sprintf(&commandString[12],"%02d)",volumeSteps[2]);
			result = AC_write(&commandString[0],commandlength);
			profileFade |=1;
			wait(30000); //wait 30 ms
			commandString[9] = 'x';
		}

		if(result == AC_SUCCESS && volumeSteps[3] != volumeRequest[3])
		{
			if((volumeSteps[3] > volumeRequest[3])&&(volumeSteps[3] >0))
			{
				volumeSteps[3]--;
			}
			else if((volumeSteps[3] < volumeRequest[3])&&(volumeSteps[3] < 63))
			{
				volumeSteps[3]++;
			}
			else
			{
				volumeRequest[3] = volumeSteps[3];
			}

			commandString[10] = '4';
			sprintf(&commandString[12],"%02d)",volumeSteps[3]);
			result = AC_write(&commandString[0],commandlength);
			profileFade |=1;
			wait(30000); //wait 30 ms
			commandString[10] = 'x';
		}
		if (result != AC_SUCCESS)
		{
			break;
		}
		if(isExpired(20000000,&timeoutTimer)== 1)
		{
			result = AC_RX_TIMEOUT;
			break;
		}
	}while(profileFade >0);
	return(result);
}

static void AC_FormatVolumeValues(const int values[4], char *buffer, size_t bufferLength)
{
	snprintf(buffer, bufferLength, "%d,%d,%d,%d", values[0], values[1], values[2], values[3]);
}

// execute and verify an audio mix profile
unsigned char AC_Profile(unsigned char index)
{
	unsigned char result = AC_SUCCESS;
	int requested[4] = {0, 0, 0, 0};
	int actual[4] = {0, 0, 0, 0};
	char requestedText[48];
	char actualText[48];
	int actualAvailable = 0;
	int attempt = 1;

	result = AC_Request(index);
	memcpy(requested, volumeRequest, sizeof(requested));
	AC_FormatVolumeValues(requested, requestedText, sizeof(requestedText));
	AC_Log("AC_PROFILE_BEGIN pid=%d profile=%u attempt=1 requested=%s actual=unavailable result=%u",
		getpid(), index, requestedText, result);
	AC_Log("AC_PROFILE_REQUEST pid=%d profile=%u attempt=1 requested=%s actual=unavailable result=%u",
		getpid(), index, requestedText, result);
	if (result == AC_SUCCESS && uart0_filestream < 0)
	{
		result = AC_COM_FAILED;
	}
	if (result != AC_SUCCESS)
	{
		goto profile_done;
	}

	for (attempt = 1; attempt <= AC_PROFILE_MAX_ATTEMPTS; ++attempt)
	{
		actualAvailable = 0;
		AC_Log("AC_PROFILE_READ_BEFORE pid=%d profile=%u attempt=%d requested=%s actual=pending result=%u",
			getpid(), index, attempt, requestedText, AC_SUCCESS);
		result = AC_GetVolume();
		if (result != AC_SUCCESS)
		{
			AC_Log("AC_PROFILE_READ_BEFORE pid=%d profile=%u attempt=%d requested=%s actual=unavailable result=%u",
				getpid(), index, attempt, requestedText, result);
			break;
		}
		memcpy(actual, volumeState, sizeof(actual));
		actualAvailable = 1;
		AC_FormatVolumeValues(actual, actualText, sizeof(actualText));
		AC_Log("AC_PROFILE_READ_BEFORE pid=%d profile=%u attempt=%d requested=%s actual=%s result=%u",
			getpid(), index, attempt, requestedText, actualText, AC_SUCCESS);
		memcpy(volumeSteps, actual, sizeof(volumeSteps));

		AC_Log("AC_PROFILE_WRITE_BEGIN pid=%d profile=%u attempt=%d requested=%s actual=%s result=%u",
			getpid(), index, attempt, requestedText, actualText, AC_SUCCESS);
		result = AC_Control();
		AC_Log("AC_PROFILE_WRITE_END pid=%d profile=%u attempt=%d requested=%s actual=unverified steps=%d,%d,%d,%d result=%u",
			getpid(), index, attempt, requestedText,
			volumeSteps[0], volumeSteps[1], volumeSteps[2], volumeSteps[3], result);
		if (result != AC_SUCCESS)
		{
			break;
		}

		wait(AC_PROFILE_SETTLE_US);
		result = AC_GetVolume();
		if (result != AC_SUCCESS)
		{
			actualAvailable = 0;
			AC_Log("AC_PROFILE_READ_AFTER pid=%d profile=%u attempt=%d requested=%s actual=unavailable result=%u",
				getpid(), index, attempt, requestedText, result);
			break;
		}
		memcpy(actual, volumeState, sizeof(actual));
		actualAvailable = 1;
		AC_FormatVolumeValues(actual, actualText, sizeof(actualText));
		AC_Log("AC_PROFILE_READ_AFTER pid=%d profile=%u attempt=%d requested=%s actual=%s result=%u",
			getpid(), index, attempt, requestedText, actualText, AC_SUCCESS);

		int matched = AC_ProfileValuesMatch(requested, actual);
		AC_ProfileDecision decision = AC_GetProfileDecision(AC_SUCCESS, matched, attempt);
		AC_Log("AC_PROFILE_VERIFY pid=%d profile=%u attempt=%d requested=%s actual=%s match=%d result=%u",
			getpid(), index, attempt, requestedText, actualText, matched,
			decision == AC_PROFILE_DECISION_FAIL ? AC_PROFILE_VERIFY_FAILED : AC_SUCCESS);
		if (decision == AC_PROFILE_DECISION_SUCCESS)
		{
			// This summary/diagnostic value advances only after verified mixer state.
			aC_Data.activeAudioProfile = index;
			result = AC_SUCCESS;
			AC_Log("AC_PROFILE_SUCCESS pid=%d profile=%u attempt=%d requested=%s actual=%s result=%u",
				getpid(), index, attempt, requestedText, actualText, result);
			break;
		}
		if (decision == AC_PROFILE_DECISION_RETRY)
		{
			AC_Log("AC_PROFILE_RETRY pid=%d profile=%u attempt=%d requested=%s actual=%s result=%u",
				getpid(), index, attempt + 1, requestedText, actualText, AC_SUCCESS);
			result = AC_SUCCESS;
			continue;
		}
		result = AC_PROFILE_VERIFY_FAILED;
	}

profile_done:
	if (result != AC_SUCCESS)
	{
		if (actualAvailable)
		{
			AC_FormatVolumeValues(actual, actualText, sizeof(actualText));
		}
		else
		{
			snprintf(actualText, sizeof(actualText), "unavailable");
		}
		AC_Log("AC_PROFILE_FAIL pid=%d profile=%u attempt=%d requested=%s actual=%s result=%u",
			getpid(), index, attempt > AC_PROFILE_MAX_ATTEMPTS ? AC_PROFILE_MAX_ATTEMPTS : attempt,
			requestedText, actualText, result);
	}
	AC_close();
	return result;
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
		case AUDIO_RESET:
			AC_Log("AUDIO RESET: pid=%d caller=AC_Execute", getpid());
			if (result == AC_SUCCESS)
			{
				result = AC_write(&RESET[0],sizeof(RESET));  // send reset command to Audiomix
			}
			AC_close();
			return result;
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
	AC_close();
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

	if (result == AC_SUCCESS)
	{
		result = AC_write(MUTESTATE,sizeof(MUTESTATE));
	}


	if(result == AC_SUCCESS)
	{
		result =  AC_read(&testString[0],sizeof(testString));
	}
	AC_close();
	return(result);
}
