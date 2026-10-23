#include "AudioControlWrite.h"

#include <errno.h>

unsigned char AC_WriteAllWith(int fd, const char *buffer, size_t length,
		AC_WriteFunction writeFunction, size_t *written, int *errorNumber)
{
	if (written == NULL || errorNumber == NULL || writeFunction == NULL ||
		(buffer == NULL && length != 0))
	{
		return AC_TX_FAILED;
	}
	*written = 0;
	*errorNumber = 0;
	while (*written < length)
	{
		ptrdiff_t bytesWritten = writeFunction(fd, buffer + *written, length - *written);
		if (bytesWritten > 0 && (size_t)bytesWritten <= length - *written)
		{
			*written += (size_t)bytesWritten;
			continue;
		}
		if (bytesWritten < 0 && errno == EINTR)
		{
			continue;
		}
		*errorNumber = bytesWritten == 0 || bytesWritten > 0 ? EIO : errno;
		return AC_TX_FAILED;
	}
	return AC_SUCCESS;
}