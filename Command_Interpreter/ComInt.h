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

	#define CI_IR            10 // IR control command
	#define CI_IR_TEACH      11 // IR teach command
    #define CI_CAMERA        20 // camera move command
    #define CI_CAMERA_TEACH  21 // camera teach command
	#define CI_AUDIO_SWITCH  30 // audio switch command
    #define CI_AUDIO_TEACH   31 // teach audio profile
	#define CI_POS_CONTROL   40 // position control command
    #define CI_POS_TEACH     41 // position control teach command
	#define CI_SEQUENCE      50	// execute command sequence
    #define CI_PAUSE          1 // pause sequence
    #define CI_TIMER          2 // timer sequence
    #define CI_PPP_VIEW       3 // power point view
    #define CI_GOPRO_VIEW     4 // gopro action cam view
    #define CI_POSCAM_VIEW    5 // camcorder with position control view
    #define CI_PREACHER_VIEW  6 // camcorder 2 control view
    #define CI_PRAYER_VIEW    7 // combination ppp view with gopro action cam view with audio worship
    #define CI_READERS_VIEW   8 // combination ppp view with gopro action cam view with audio preaching
    #define CI_SONG_VIEW      9 // combination ppp view with gopro action cam view with audio Band
    #define CI_RESET          11// reset audio - to sumary signal and IR to laptop view
    #define CI_BEAMER_HDMI_1  12// switch Beamer to HDMI 1 (PPP View)
    #define CI_BEAMER_HDMI_2  13// switch Beamer to HDMI 2 (live stream View)
    #define CI_BEAMER_ANALOG  14// switch Beamer to analog input (video from CD)
    #define CI_BEAMER_MUTE    16// mute/demute Beamer
    #define CI_STOP_BACKUP    17// stopps the backup-recorder
    #define CI_SHUTDOWN       15// switch Beamer, HDMI switch, Backuprecorder off and shut down Raspberry Pi

	extern unsigned char CIexecuteCommand(char *argv[]);


#endif /* COMMAND_INTERPRETER_COMINT_H_ */
