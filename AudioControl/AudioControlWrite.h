#ifndef AUDIOCONTROL_WRITE_H_
#define AUDIOCONTROL_WRITE_H_

#include <stddef.h>
#include "AudioControl.h"

typedef ptrdiff_t (*AC_WriteFunction)(int fd, const char *buffer, size_t length);

unsigned char AC_WriteAllWith(int fd, const char *buffer, size_t length,
		AC_WriteFunction writeFunction, size_t *written, int *errorNumber);

#endif