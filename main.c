#include <stdio.h>
#include "IRcontrol.h"
#include "ComInt.h"
#include "timer.h"
#include "system.h"

#define RASPI_AUTOMATION_SUCCESS 0 // return value application finished successful
#define RASPI_AUTOMATION_CALL_FAILURE 1 // error in application call to much, less or wrong arguments


int main(int argc, char *argv[])
{

	unsigned char result = RASPI_AUTOMATION_SUCCESS;
	if(argc == 3)
	{
		SYSTEM_Initialize();
		initSystemTimer();
		result = IR_init();
		result = CIexecuteCommand(argv);
	}
	else
	{
		result = RASPI_AUTOMATION_CALL_FAILURE;
	}
	return result;
}
