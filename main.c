#include <fcntl.h>	/* fcntl, open */
#include <stdlib.h>	/* atexit, getenv, malloc */
#include <stdio.h>	/* fputs, printf, puts, snprintf */
#include <string.h>	/* memcpy */
#include <errno.h>	/* errno */
#include <sys/types.h>	/* uid_t */
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
	if (ooi_path)
		unlink(ooi_path);
}

static char *
build_ooi_path(const char *dir)
{
	size_t dirlen;
	const char *sep;
	uid_t uid;
	int needed;
	char *path;

	if (dir == NULL)
		return NULL;
	dirlen = strlen(dir);
	if (dirlen == 0)
		return NULL;
	sep = (dir[dirlen - 1] == '/') ? "" : "/";
	uid = getuid();

	needed = snprintf(NULL, 0, "%s%s%s-%lu", dir, sep, INSTANCE_LOCK, (unsigned long)uid);
	if (needed < 0)
		return NULL;

	path = malloc((size_t)needed + 1);
	if (path == NULL)
		return NULL;

	snprintf(path, (size_t)needed + 1, "%s%s%s-%lu", dir, sep, INSTANCE_LOCK, (unsigned long)uid);
	return path;
}

/* Exit if another instance of this program is running. */
void
only_one_instance(void)
{
	struct flock fl;
	int fd;
	const char *dir;
	const char *xdg;
	const char *dirs[3];
	int ndirs;
	int i;

	/*
	 * Place the lock in a per-user writable directory:
	 * XDG_RUNTIME_DIR, then HOME, then /tmp.
	 * The filename includes the uid to avoid cross-user collisions.
	 */
	ndirs = 0;
	xdg = getenv("XDG_RUNTIME_DIR");
	if (xdg != NULL && xdg[0] == '/')
		dirs[ndirs++] = xdg;
	dir = getenv("HOME");
	if (dir != NULL && dir[0] == '/')
		dirs[ndirs++] = dir;
	dirs[ndirs++] = "/tmp";

	fd = -1;
	for (i = 0; i < ndirs; i++) {
		char *candidate;

		candidate = build_ooi_path(dirs[i]);
		if (candidate == NULL)
			continue;

		fd = open(candidate, O_RDWR | O_CREAT, 0600);
		if (fd >= 0) {
			ooi_path = candidate;
			break;
		}

		free(candidate);
	}

	if (fd < 0) {
		fputs("Could not create instance lockfile.\n", stderr);
		perror("open");
		exit(1);
	}

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLK, &fl) < 0)
	{
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
		if (SYSTEM_Initialize() != 0)
		{
			return RASPI_AUTOMATION_CALL_FAILURE;
		}
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
