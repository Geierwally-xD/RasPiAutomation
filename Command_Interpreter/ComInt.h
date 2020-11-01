/*==============================================================================

 Name:           ComInt.h
 Description:    declaration of command line interpreter
                 for 64 bit Raspberry Pi
 Copyright:      Geierwally, 2020(c)

==============================================================================*/

#ifndef COMMAND_INTERPRETER_COMINT_H_
#define COMMAND_INTERPRETER_COMINT_H_

	#define CI_SUCCESS 0 // command line interpreter successful finished
	#define CI_WRONG_ARG 2 // command line argument undefined

	#define CI_IR 10 // IR control command
	#define CI_IR_TEACH 11 // IR teach command
    #define CI_CAMERA 20 // camera move command
    #define CI_CAMERA_TEACH 21 // camera teach command
	#define CI_AUDIO_SWITCH 30  // audio switch command

    extern unsigned char CIexecuteCommand(char *argv[]);



#endif /* COMMAND_INTERPRETER_COMINT_H_ */
