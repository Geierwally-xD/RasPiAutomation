/*==============================================================================

 Name:           AudioCommands.h
 Description:    declaration of Audio Command Constants
 Copyright:      Geierwally, 2020(c)

==============================================================================*/

#ifndef AUDIOCONTROL_AUDIOCOMMANDS_H_
#define AUDIOCONTROL_AUDIOCOMMANDS_H_

    #define ACTIVE 0x10
    #define VOLUME 0x20
    #define FADEUP 0x30
    #define FADEDOWN 0x40
    #define FADESTOP 0x50
    #define AUDIO_RESET 0x60

    char GET_VERSION[] = "(MX?:VERS)";     // get software version
	char SET_ACTIVE[]  = "(MX*:M:----)";
	char SET_VOLUME[]  = "(MX*:S:xxxx:63)"; // set volume of all inputs (range 00 - 63)
	char FADE_UP[]     = "(MX*:>:xxxx:5)";  // fade up with speed 5  (range 0 - 9)
	char FADE_DOWN[]   = "(MX*:<:xxxx:5)";  // fade down with speed 5(range 0 - 9)
	char FADE_STOP[]   = "(MX*:*:xxxx)";
	char RESET[]       = "(MX*:RES!)";      // reset Audioswitch sets all adjustments to 0
	char MUTESTATE[]   = "(MX?:MUTE)";
	char VOL1[]        = "(MX?:VOL1)";

#endif /* AUDIOCONTROL_AUDIOCOMMANDS_H_ */
