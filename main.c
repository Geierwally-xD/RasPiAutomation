#include <fcntl.h>	/* fcntl, open */
#include <stdlib.h>	/* atexit, getenv, malloc */
#include <stdio.h>	/* fputs, printf, puts, snprintf */
#include <string.h>	/* memcpy */
#include <unistd.h>	/* sleep, unlink */
#include "IRcontrol.h"
#include "ComInt.h"
#include "timer.h"
#include "system.h"
#include "AudioControl.h"
#include "PositionControl.h"

#define RASPI_AUTOMATION_SUCCESS 0 // return value application finished successful
#define RASPI_AUTOMATION_CALL_FAILURE 1 // error in application call to much, less or wrong arguments

#define INSTANCE_LOCK "rosetta-code-lock"

void
fail(const char *message)
{
	perror(message);
	exit(1);
}

/* Path to only_one_instance() lock. */
static char *ooi_path;

void
ooi_unlink(void)
{
	unlink(ooi_path);
}
/* Exit if another instance of this program is running. */
void
only_one_instance(void)
{
	struct flock fl;
	size_t dirlen;
	int fd;
	char *dir;

	/*
	 * Place the lock in the home directory of this user;
	 * therefore we only check for other instances by the same
	 * user (and the user can trick us by changing HOME).
	 */
	dir = getenv("HOME");
	if (dir == NULL || dir[0] != '/') {
		fputs("Bad home directory.\n", stderr);
		exit(1);
	}
	dirlen = strlen(dir);

	ooi_path = malloc(dirlen + sizeof("/" INSTANCE_LOCK));
	if (ooi_path == NULL)
		fail("malloc");
	memcpy(ooi_path, dir, dirlen);
	memcpy(ooi_path + dirlen, "/" INSTANCE_LOCK,
	    sizeof("/" INSTANCE_LOCK));  /* copies '\0' */

	fd = open(ooi_path, O_RDWR | O_CREAT, 0600);
	if (fd < 0)
		fail(ooi_path);

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLK, &fl) < 0) {
		fputs("Another instance of this program is running.\n",
		    stderr);
		exit(1);
	}

	/*
	 * Run unlink(ooi_path) when the program exits. The program
	 * always releases locks when it exits.
	 */
	atexit(ooi_unlink);
}

int main(int argc, char *argv[])
{

	only_one_instance();

   /*
    * Endianess helper
    */
	//short i = 1;
	//char *p = (char*) &i;
	//char isLittleEndian = p[0];

	unsigned char result = RASPI_AUTOMATION_SUCCESS;
	if(argc == 3)
	{
		SYSTEM_Initialize();
		initSystemTimer();
		ACinit();
		result = CIexecuteCommand(argv);
	}
	else
	{
		result = RASPI_AUTOMATION_CALL_FAILURE;
	}
	return result;
}
