/*==============================================================================

 Name:           IRcontrol.h
 Description:    declaration of IR remote control driver
 Copyright:      Geierwally, 2020(c)

==============================================================================*/


#ifndef IRCONTROL_H_
#define IRCONTROL_H_
	#define IR_OUT 11 // IR output signal
	#define IR_IN  31 // IR input signal
	#define NUM_SEQ 20 // number of IR sequences
	#define SEQ_SIZE 100 // number of IR data for each sequence

	#define IR_SUCCESS 0 // IR function successful finished
	#define IR_UNDEFINED 11 // IR function call with undefined parameter
	#define IR_SEQ_EMPTY 12 // IR sequence empty (not teached)
	#define IR_TIMEOUT 13   // IR function time overflow (9 seconds limit reached)
	#define IR_WRITE_FAILED 14 // IR write data file failed
	#define IR_READ_FAILED 15 // IR read data file failed

	#define IR_SCAN_TIMEOUT 9000000 // IR teach timeout 9 seconds
	#define IR_SEQUENCE_TEACHED 60000 // IR sequence teached timeout 60 ms


    #define _FREQUENCY_                 38  // The frequency of the IR signal in KHz
	#define _LEADING_PULSE_DURATION_  	9000 // The duration of the beginning pulse in microseconds
	#define _LEADING_GAP_DURATION_   	4500 // The duration of the gap in microseconds after the leading pulse
	#define _ON_PULSE_                	562  // The duration of a pulse in microseconds when sending a logical 1
	#define _OFF_PULSE_ 				562  // The duration of a pulse in microseconds when sending a logical 0
	#define _ON_GAP_ 					1688 // The duration of the gap in microseconds when sending a logical 1
	#define _OFF_GAP_  					562  // The duration of the gap in microseconds when sending a logical 0

	typedef struct
	{
		unsigned char signal;		// IR_OUT signal 0 or 1
		unsigned long long IR_Time; // The signal time in microseconds
	}IR_Data;

	static const unsigned char IR_RETRY[NUM_SEQ] = {2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1 };

    extern unsigned char IR_init(void);
    extern unsigned char IR_SequenceOut(unsigned char seqID);
    extern unsigned char IR_SequenceIn(unsigned char seqID);
    extern unsigned char IR_writeDatFile(void);
    extern unsigned char IR_readDatFile(void);
#endif /* IRCONTROL_H_ */
