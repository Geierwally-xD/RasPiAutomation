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
	#define AC_WRITE_FAILED 35 // AC write data file failed
	#define AC_READ_FAILED 36 // AC read data file failed
	#define AC_OUT_OF_RANGE 37 // AC data out of range
    #define AC_RX_TIMEOUT 38   // AC receive data from Audio Mix timed out

    #define AC_SINGLE_SEQUENCE 0 // execute single command
    #define AC_INIT_SEQUENCE 1  // execute initialization sequence (Reset, Set Active, Set Volume)
    #define AC_FADER_SEQUENCE 2 // execute fader sequence (active fader up , all other faders down)

    #define ACTIVE 0x10
    #define VOLUME 0x20
    #define FADEUP 0x30
    #define FADEDOWN 0x40
    #define FADESTOP 0x50
    #define AUDIO_RESET 0x60
    #define AUDIO_PROFILE  0x70
    #define AC_SLIDERSHOW  0x00 // audio profile 1
    #define AC_WORSHIP     0x01 // audio profile 2
    #define AC_PREACHING   0x02 // audio profile 3
    #define AC_TEXT        0x03 // audio profile 4
    #define AC_BAND        0x04 // audio profile 5
    #define AC_VIDEO_CLIP  0x05 // audio profile 6

    extern unsigned char AC_init(void);
    extern unsigned char AC_write(char* buffer, int length); // prototype write to COM Port
    extern unsigned char AC_read(char* buffer, int  length);  // prototype read from COM Port
    extern unsigned char AC_Execute(unsigned char index);    // execute command
    extern unsigned char AC_GetVersion (void);
    extern unsigned char AC_Test (void);
    extern unsigned char AC_Profile(unsigned char index);
    extern unsigned char AC_Teach(int index);
    extern unsigned char AC_writeDatFile(void);

    typedef struct
	{
		unsigned char activeAudioProfile; // for audio switch summary signal with channel 2, 3, or 4
		unsigned char audioProfiles[10][4];
	}AC_Data;

	extern unsigned char ACinit(void);
	extern AC_Data aC_Data;
#endif /* AUDIOCONTROL_AUDIOCONTROL_H_ */
