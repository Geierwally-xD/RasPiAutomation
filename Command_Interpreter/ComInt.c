/*==============================================================================

 Name:           ComInt.c
 Description:    command line interpreter for 64 bit Rspberry Pi
 Copyright:      Geierwally, 2020(c)

==============================================================================*/
#include <stdlib.h>
#include "ComInt.h"
#include "IRcontrol.h"
#include "AudioControl.h"

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
		default:
			result = CI_WRONG_ARG; // command line argument undefined
		break;
	}
	return(result);
}



