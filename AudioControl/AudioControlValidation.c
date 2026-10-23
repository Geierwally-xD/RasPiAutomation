#include "AudioControlValidation.h"

int AC_ParseVolumeResponse(const char *response, size_t responseLength,
		int expectedChannel, int *parsedVolume, const char **reason)
{
	if (reason != NULL)
	{
		*reason = "invalid argument";
	}
	if (response == NULL || parsedVolume == NULL || expectedChannel < 1 || expectedChannel > 4)
	{
		return 0;
	}
	if (responseLength != 9)
	{
		if (reason != NULL)
		{
			*reason = "invalid length";
		}
		return 0;
	}
	if (response[0] != '(' || response[1] != 'V' || response[2] != 'O' || response[3] != 'L' || response[5] != ':')
	{
		if (reason != NULL)
		{
			*reason = "invalid prefix";
		}
		return 0;
	}
	if (response[4] != (char)('0' + expectedChannel))
	{
		if (reason != NULL)
		{
			*reason = "unexpected channel";
		}
		return 0;
	}
	if (response[6] < '0' || response[6] > '9' || response[7] < '0' || response[7] > '9')
	{
		if (reason != NULL)
		{
			*reason = "volume is not two decimal digits";
		}
		return 0;
	}
	if (response[8] != ')')
	{
		if (reason != NULL)
		{
			*reason = "invalid terminator";
		}
		return 0;
	}
	int volume = (response[6] - '0') * 10 + (response[7] - '0');
	if (volume > 63)
	{
		if (reason != NULL)
		{
			*reason = "volume out of range";
		}
		return 0;
	}
	*parsedVolume = volume;
	if (reason != NULL)
	{
		*reason = "ok";
	}
	return 1;
}

int AC_ProfileValuesMatch(const int requested[4], const int actual[4])
{
	if (requested == NULL || actual == NULL)
	{
		return 0;
	}
	for (int channel = 0; channel < 4; ++channel)
	{
		if (requested[channel] != actual[channel])
		{
			return 0;
		}
	}
	return 1;
}

AC_ProfileDecision AC_GetProfileDecision(unsigned char communicationResult,
		int matched, int attempt)
{
	if (communicationResult != AC_SUCCESS)
	{
		return AC_PROFILE_DECISION_FAIL;
	}
	if (matched)
	{
		return AC_PROFILE_DECISION_SUCCESS;
	}
	return attempt < AC_PROFILE_MAX_ATTEMPTS ? AC_PROFILE_DECISION_RETRY : AC_PROFILE_DECISION_FAIL;
}