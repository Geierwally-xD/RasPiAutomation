/*==============================================================================

 Name:           ComInt.c
 Description:    command line interpreter for 64 bit Rspberry Pi
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include "ComInt.h"
#include "IRcontrol.h"
#include "AudioControl.h"



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
			result |= AC_Execute((unsigned char)FADEUP + 0x01); // fade up audio channel 1 and fade down 2 - 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5); // Beamer HDMI 1
			}
		break;
		case CI_TIMER:         //2  timer sequence
			result |= AC_Execute((unsigned char)FADEUP + 0x01); // fade up audio channel 1 and fade down 2 - 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5); // Beamer HDMI 1
			}
		break;
		case CI_PPP_VIEW:       //3  power point view
			result = AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 2 and fade down 1, 3 and 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5); // Beamer HDMI 1
			}
		break;
		case CI_GOPRO_VIEW:     //4  gopro action cam view
			result = AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 2 and fade down 1, 3 and 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(1);  // HDMI switch channel 2 (GoPro)
			}
		break;
		case CI_POSCAM_VIEW:    //5  camcorder with position control view
			result = AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 2 and fade down 1, 3 and 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(2);  // HDMI switch channel 3 (camcorder with position control)
			}
		break;
		case CI_PREACHER_VIEW:  //6  camcorder 2 control view
			result = AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 2 and fade down 1, 3 and 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(3);  // HDMI switch channel 4 (camcorder preacher view)
			}
		break;
		case CI_PRAYER_VIEW:    //7  combination ppp view with gopro action cam view
			result = AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 2 and fade down 1, 3 and 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(4);   // HDMI switch channel 5 (combination PPP and GoPro)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5);   // Beamer HDMI 1
			}
		break;
		case CI_AUDIO_SUMARY:  //8 audioswitch activate sumary signal (channel 2)
			aC_Data.activeAudioChannel = 0x02;
			result |= AC_writeDatFile();
			result |= AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 2 and fade down 1, 3 and 4
		break;
		case CI_AUDIO_INPUT_3: //9 audioswitch activate channel 3
			aC_Data.activeAudioChannel = 0x04;
			result |= AC_writeDatFile();
			result |= AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 3 and fade down 1, 3 and 4
		break;
		case CI_AUDIO_INPUT_4: //10 audioswitch activate channel 4
			aC_Data.activeAudioChannel = 0x08;
			result |= AC_writeDatFile();
			result |= AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 4 and fade down 1, 3 and 4
		break;
		case CI_RESET: //11 reset audio - to sumary signal and IR to laptop view
			aC_Data.activeAudioChannel = 0x02;
			result |= AC_writeDatFile();
			result |= AC_Execute((unsigned char)FADEUP + aC_Data.activeAudioChannel);  // fade up audio channel 2 and fade down 1, 3 and 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5); // Beamer HDMI 1
			}
		break;
		case CI_BEAMER_HDMI_1:  //12 switch Beamer to HDMI 1 (PPP View)
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(5); // Beamer HDMI 1
			}
		break;
		case CI_BEAMER_HDMI_2:  //13 switch Beamer to HDMI 2 (live stream View)
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(6); // Beamer HDMI 2
			}
		break;
		case CI_BEAMER_ANALOG:  //14 switch Beamer to analog input (video from CD)
			result |= AC_Execute((unsigned char)FADEUP + 0x01); // fade up audio channel 1 and fade down 2 - 4
			result |= IR_init();
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(0); // HDMI switch channel 1 (laptop)
			}
			if(result == IR_SUCCESS)
			{
				result |= IR_SequenceOut(7); // Beamer analog
			}
		break;
		case CI_SHUTDOWN:  //15 switch Beamer, HDMI switch, Backuprecorder off and shut down Raspberry Pi
			result |= IR_init();
			result |= IR_SequenceOut(9); // switch off Beamer
			result |= IR_SequenceOut(9); // switch off Beamer twice necessary
			result |= IR_SequenceOut(10); // switch off HDMI switch
			result |= IR_SequenceOut(11); // switch off backup recorder
			system("sudo shutdown -h now");
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
		case CI_CAMERA: // camera move command
		break;
		case CI_AUDIO_SWITCH: // audio switch command
			result = AC_Execute((unsigned char)index);
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







