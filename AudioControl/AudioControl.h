/*==============================================================================

 Name:           AudioControl.h
 Description:    declaration of Audioswitch control driver
 Copyright:      Geierwally, 2020(c)

==============================================================================*/

#ifndef AUDIOCONTROL_AUDIOCONTROL_H_
#define AUDIOCONTROL_AUDIOCONTROL_H_

	#define AC_SUCCESS     0 // AC function successful finished
	#define AC_COM_FAILED 31 // AC COM connection failure
    #define AC_TX_FAILED  32 // AC COM transmit failure
    #define AC_RX_FAILED  33 // AC COM receive failure
    #define AC_EXEC_FAILED 34 // AC execute failure

    #define AC_SINGLE_SEQUENCE 0 // execute single command
    #define AC_INIT_SEQUENCE 1  // execute initialization sequence (Reset, Set Active, Set Volume)
    #define AC_FADER_SEQUENCE 2 // execute fader sequence (active fader up , all other faders down)

    extern unsigned char AC_init(void);
    extern unsigned char AC_write(char* buffer, int length); // prototype write to COM Port
    extern unsigned char AC_read(char* buffer, int  length);  // prototype read from COM Port
    extern unsigned char AC_Execute(unsigned char index);    // execute command
    extern unsigned char AC_GetVersion (void);
    extern unsigned char AC_Test (void);

#endif /* AUDIOCONTROL_AUDIOCONTROL_H_ */
