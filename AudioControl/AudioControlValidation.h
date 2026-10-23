#ifndef AUDIOCONTROL_VALIDATION_H_
#define AUDIOCONTROL_VALIDATION_H_

#include <stddef.h>
#include "AudioControl.h"

typedef enum
{
	AC_PROFILE_DECISION_FAIL,
	AC_PROFILE_DECISION_SUCCESS,
	AC_PROFILE_DECISION_RETRY
} AC_ProfileDecision;

int AC_ParseVolumeResponse(const char *response, size_t responseLength,
		int expectedChannel, int *parsedVolume, const char **reason);
int AC_ProfileValuesMatch(const int requested[4], const int actual[4]);
AC_ProfileDecision AC_GetProfileDecision(unsigned char communicationResult,
		int matched, int attempt);

#endif