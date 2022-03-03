/*==============================================================================

 Name:           ComInt.c
 Description:    command line interpreter for 64 bit Rspberry Pi
 Copyright:      Geierwally, 2021(c)

==============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "ComInt.h"
#include "IRcontrol.h"
#include "AudioControl.h"
#include "timer.h"
#include "PositionControl.h"
#include "ZoomControl.h"

CI_Data cI_Data;


// write toggle status into datfile
unsigned char CI_writeDatFile(void)
{
	unsigned char result = AC_SUCCESS;
	FILE *write_ptr;

	write_ptr = fopen("CI_Dat.bin","wb");  // w for write, b for binary
	if(write_ptr != NULL)
	{
		fwrite(&cI_Data,sizeof(cI_Data),1,write_ptr); // write dat file
		fclose(write_ptr);
	}
	else
	{
		result = CI_WRITE_FAILED;
	}
	return(result);
}

// read toggle status from datfile
unsigned char CI_readDatFile(void)
{
	unsigned char result = AC_SUCCESS;
	FILE *read_ptr;

	read_ptr = fopen("CI_Dat.bin","rb");  // r for read, b for binary
	if(read_ptr != NULL)
	{
		fread(&cI_Data,sizeof(cI_Data),1,read_ptr); // read dat file
		fclose(read_ptr);
	}
	else
	{
		result = CI_READ_FAILED;
	}
	return(result);
}

// test comnand sequencer in while loop
void CItestSequence(unsigned char testParam)
{
	unsigned long long waitTime = (testParam & 0xfc) * 1000 ;
	unsigned char result = CI_SUCCESS;
	int retryCounter = testParam & 0x03;
	result = IR_init();
	while(1)
	{
        for(int i = 0; i< retryCounter; i++)
        {
        	wait(waitTime);
        	result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
        }
    	wait(waitTime);
		result |= IR_SequenceOut(5); // Beamer HDMI 1
		wait(2000000);

		for(int i = 0; i< retryCounter; i++)
        {
        	wait(waitTime);
    		result |= IR_SequenceOut(1);  // HDMI switch channel 2 (GoPro)
        }
    	wait(waitTime);
		result |= IR_SequenceOut(6); // Beamer HDMI 2
		wait(2000000);


		for(int i = 0; i< retryCounter; i++)
        {
        	wait(waitTime);
    		result |= IR_SequenceOut(2);  // HDMI switch channel 3 (camcorder with position control)
        }
    	wait(waitTime);
		result |= IR_SequenceOut(5); // Beamer HDMI 1
		wait(2000000);


		for(int i = 0; i< retryCounter; i++)
        {
        	wait(waitTime);
    		result |= IR_SequenceOut(4);   // HDMI switch channel 5 (combination PPP and GoPro)
        }
    	wait(waitTime);
		result |= IR_SequenceOut(6); // Beamer HDMI 1
		wait(2000000);

	}
}

// excute command sequence
unsigned char CIsequencer(unsigned char index)
{
	unsigned char result = CI_SUCCESS;
	switch(index)
	{
		case CI_PAUSE:         //1  pause sequence
			result |= AC_Execute((unsigned char)AUDIO_PROFILE + AC_SLIDERSHOW); // audio profile Diashow
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5); // Beamer HDMI 1
				cI_Data.toggleStateBeamerLive = 0;
				result |= CI_writeDatFile();
			}
		break;
		case CI_TIMER:         //2  timer sequence
			result |= AC_Execute((unsigned char)AUDIO_PROFILE + AC_SLIDERSHOW); // audio profile Diashow
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(12); // start backup recorder
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5); // Beamer HDMI 1
				cI_Data.toggleStateBeamerLive = 0;
				result |= CI_writeDatFile();
			}
		break;
		case CI_PPP_VIEW:       //3  power point view with audio profile Band
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_BAND);  // audio profile Band
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
		break;
		case CI_TEXT_VIEW:   //3  power point view with audio profile Text
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_TEXT);  // audio profile text
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
		break;
		case CI_GOPRO_VIEW:     //4  gopro action cam view
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_WORSHIP);  // audio profile Gottesdienst
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(1);  // HDMI switch channel 2 (GoPro)
			}
		break;
		case CI_POSCAM_VIEW:    //5  camcorder with position control view
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_WORSHIP);  // audio profile Gottesdienst
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(2);  // HDMI switch channel 3 (camcorder with position control)
			}
		break;
		case CI_PREACHER_VIEW:  //6  camcorder 2 control view
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_PREACHING);  // audio profile Predigt
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(3);  // HDMI switch channel 4 (camcorder preacher view)
			}
		break;
		case CI_PRAYER_VIEW:    //7  combination ppp view with gopro action cam view
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_WORSHIP );  // audio profile Gottesdienst
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(4);   // HDMI switch channel 5 (combination PPP and GoPro)
			}
		break;
		case CI_READERS_VIEW:    //7  combination ppp view with gopro action cam view
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_PREACHING );  // audio profile Predigt
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(4);   // HDMI switch channel 5 (combination PPP and GoPro)
			}
		break;
		case CI_SONG_VIEW:    //7  combination ppp view with gopro action cam view
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_BAND );  // audio profile Band
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(4);   // HDMI switch channel 5 (combination PPP and GoPro)
			}
		break;
		case CI_RESET: //11 reset audio - to sumary signal and IR to laptop view
			result = AC_Execute((unsigned char)AUDIO_RESET);  // reset audio mix
			aC_Data.activeAudioProfile = AC_WORSHIP;
			result |= AC_writeDatFile();
			result = AC_Execute((unsigned char)AUDIO_PROFILE + AC_WORSHIP);  // audio profile worship
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5); // Beamer HDMI 1
			}
			cI_Data.toggleStateBeamerLive = 0;
			result |= CI_writeDatFile();
		break;
		case CI_LIVE_VIDEO:  // switch HDMI and audio to Laptop
			result |= AC_Execute((unsigned char)AUDIO_PROFILE + AC_VIDEO_CLIP);  // audio profile laptop video clip
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}

		break;
		case CI_BEAMER_TOGGLE:  //13 toggle Beamer between HDMI1 (laptop) and HDMI 2 (live stream View)
			result |= IR_init();
			result |= CI_readDatFile();
			if(result == IR_SUCCESS)
			{
				if(cI_Data.toggleStateBeamerLive == 0)
				{
					result |= IR_SequenceOut(6); // Beamer HDMI 2
					cI_Data.toggleStateBeamerLive = 1;
				}
				else
				{
					result |= IR_SequenceOut(5); // Beamer HDMI 1
					cI_Data.toggleStateBeamerLive = 0;
				}
				result |= CI_writeDatFile();
			}
		break;
		case CI_BEAMER_ANALOG:  //14 switch Beamer to analog input (video from CD)
			result |= AC_Execute((unsigned char)AUDIO_PROFILE + AC_SLIDERSHOW); // audio profile Diashow
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(7); // Beamer analog
			}
			cI_Data.toggleStateBeamerLive = 2;
			result |= CI_writeDatFile();
		break;
		case CI_BEAMER_MUTE:  //16 mute/demute Beamer
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(8); // Mute/ Demute Beamer
			}
		break;
		case CI_BEAMER_ON:  //19 switch on Beamer
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(14); // switch on Beamer
			}
		break;
		case CI_START_BACKUP:  //18 starts the backup-recorder
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(12); // start backup recorder
			}
		break;
		case CI_STOP_BACKUP:  //17 stopps the backup-recorder
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(13); // stopp backup recorder
			}
		break;
		case CI_SWITCH_BACKUP:  //18 starts the backup-recorder
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(11); // switch on/ off backup recorder
			}
		break;
		case CI_SHUTDOWN:  //15 switch Beamer, HDMI switch, Backuprecorder off and shut down Raspberry Pi
			result |= IR_init();
			result |= IR_SequenceOut(9);  // switch off Beamer
			result |= IR_SequenceOut(9);  // switch off Beamer twice necessary
			result |= IR_SequenceOut(10); // switch off HDMI switch
			result |= IR_SequenceOut(13); // stop backup recorder
			result |= IR_SequenceOut(11); // switch off backup recorder
			PC_shutdown();
			IR_shutdown();                // switch off IR output
			ZoomControl_shutdown();        // switch off PWM
			//system("sudo shutdown -h now"); moved to shell script
			result = 99;
		break;
		default:
			if(index > 50)
			{
				CItestSequence(index);
			}
			result = CI_WRONG_ARG; // command line argument undefined
		break;
	}
	return(result);
}


// interpret command line argument and call sub component
unsigned char CIexecuteCommand(char *argv[])
{
	unsigned char result = CI_SUCCESS;
	int command = atoi(&argv[1][0]);
	int index = atoi(&argv[2][0]);
	switch(command)
	{
		case CI_IR: // IR control command
			result = IR_init();
			if(result == IR_SUCCESS)
			{
				result = IR_SequenceOut((unsigned char)index);
			}
		break;
		case CI_IR_TEACH: // IR teach command
			result = IR_init();
			if(result == IR_SUCCESS)
			{
				result = IR_SequenceIn((unsigned char)index);
			}
		break;
		case CI_POS_CONTROL: // position control move to position
			result = IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(2); // switch hdmi to camcorder 1
			}
			result |= PC_Init();
			if(result == PC_SUCCESS)
			{
				result |= PC_Move((unsigned char)index);
			}
		break;
		case CI_POS_TEACH: // position control teach position
			result = IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(2); // switch hdmi to camcorder 1
			}
			result |= PC_Init();
			if(result == PC_SUCCESS)
			{
				result |= PC_Teach((unsigned char)index);
			}
		break;
		case CI_POS_TEST:
			result = IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(2); // switch hdmi to camcorder 1
			}
			result |= PC_Init();
			if(result == PC_SUCCESS)
			{
				if(index == 0)
				{
					PC_Test_Pos();
				}
				else
				{
					PC_Test();
				}
			}
		break;
		case CI_POS_SEQUENCE: // command sequence position control
			result = IR_init();
			result |= PC_Init();
			if(result == PC_SUCCESS)
			{
				result = PC_Sequencer(index);
			}
		break;
		case CI_POS_MOVE_BTN: // position control move button pressed
			result = PC_Init();
			if(result == PC_SUCCESS)
			{
				result = PC_MoveButton((unsigned char)index);
			}
	    break;
		case CI_ZOOM_MOVE:  // autozoom move to position
			result = ZoomControl_init();
			if(result == AZ_SUCCESS)
			{
				result = ZoomControl_MoveToPos((uint8_t)index);
			}
		break;
		case CI_ZOOM_CALIB: // calibrate autozoom
			result = ZoomControl_init();
			if(result == AZ_SUCCESS)
			{
				result = ZoomControl_Calibrate();
			}
		break;
		case CI_ZOOM_TEST:  // autozoom test positions
			result = ZoomControl_init();
			if(result == AZ_SUCCESS)
			{
				result = ZoomControl_TestPositions();
			}
		break;
		case CI_ZOOM_SERVO_MOVE: // autozoom move servo to position
			result = ZoomControl_init();
			if(result == AZ_SUCCESS)
			{
				result = ZoomControl_MoveServo((uint8_t)index);
			}
		break;
		case CI_AUDIO_SWITCH: // audio switch command
			result = AC_Execute((unsigned char)index);
		break;
		case CI_AUDIO_TEACH: // teach audio profile
			result = AC_Teach(index);
		break;
		case CI_SEQUENCE: // command sequence
			result = CIsequencer((unsigned char)index);
		break;

		default:
			result = CI_WRONG_ARG; // command line argument undefined
		break;
	}
	return(result);
}






