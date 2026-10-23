#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "AudioControlValidation.h"
#include "AudioControlWrite.h"

static ptrdiff_t writeResults[8];
static int writeErrors[8];
static size_t writeResultCount;
static size_t writeCallIndex;

static ptrdiff_t fake_write(int fd, const char *buffer, size_t length)
{
	(void)fd;
	(void)buffer;
	(void)length;
	assert(writeCallIndex < writeResultCount);
	ptrdiff_t result = writeResults[writeCallIndex];
	errno = writeErrors[writeCallIndex];
	++writeCallIndex;
	return result;
}

static void set_write_script(const ptrdiff_t *results, const int *errors, size_t count)
{
	memcpy(writeResults, results, count * sizeof(results[0]));
	memcpy(writeErrors, errors, count * sizeof(errors[0]));
	writeResultCount = count;
	writeCallIndex = 0;
}

static void assert_valid(const char *response, int channel, int expectedVolume)
{
	int parsedVolume = -1;
	const char *reason = NULL;
	assert(AC_ParseVolumeResponse(response, strlen(response), channel, &parsedVolume, &reason));
	assert(parsedVolume == expectedVolume);
	assert(strcmp(reason, "ok") == 0);
}

static void assert_invalid(const char *response, size_t responseLength, int channel)
{
	int parsedVolume = 17;
	const char *reason = NULL;
	assert(!AC_ParseVolumeResponse(response, responseLength, channel, &parsedVolume, &reason));
	assert(parsedVolume == 17);
	assert(reason != NULL && strcmp(reason, "ok") != 0);
}

static void test_volume_response_parser(void)
{
	assert_valid("(VOL1:00)", 1, 0);
	assert_valid("(VOL2:50)", 2, 50);
	assert_valid("(VOL3:63)", 3, 63);
	assert_valid("(VOL4:31)", 4, 31);
	assert_invalid("(VOL1:00)", 9, 2);
	assert_invalid("(VOL2:99)", 9, 2);
	assert_invalid("(VOL2:5)", 8, 2);
	assert_invalid("(VOL2:AA)", 9, 2);
	assert_invalid("VOL2:50", 7, 2);
	assert_invalid("(VOL2:50", 8, 2);
	assert_invalid("", 0, 2);
	assert_invalid("(VOL2:5", 7, 2);
	assert_invalid("(VOL2:50)X", 10, 2);
}

static void test_profile_comparison(void)
{
	const int requested[4] = {0, 50, 0, 31};
	const int exact[4] = {0, 50, 0, 31};
	const int wrongProfile[4] = {0, 31, 0, 56};
	const int oneStepOff[4] = {0, 50, 0, 30};

	assert(AC_ProfileValuesMatch(requested, exact));
	assert(!AC_ProfileValuesMatch(requested, wrongProfile));
	assert(!AC_ProfileValuesMatch(requested, oneStepOff));
}

static void test_profile_retry_decisions(void)
{
	assert(AC_GetProfileDecision(AC_SUCCESS, 1, 1) == AC_PROFILE_DECISION_SUCCESS);
	assert(AC_GetProfileDecision(AC_SUCCESS, 0, 1) == AC_PROFILE_DECISION_RETRY);
	assert(AC_GetProfileDecision(AC_SUCCESS, 1, 2) == AC_PROFILE_DECISION_SUCCESS);
	assert(AC_GetProfileDecision(AC_SUCCESS, 0, 2) == AC_PROFILE_DECISION_FAIL);
	assert(AC_GetProfileDecision(AC_SUCCESS, 0, 3) == AC_PROFILE_DECISION_FAIL);
	assert(AC_PROFILE_VERIFY_FAILED == 40);
	assert(AC_GetProfileDecision(AC_INVALID_RESPONSE, 0, 1) == AC_PROFILE_DECISION_FAIL);
	assert(AC_GetProfileDecision(AC_TX_FAILED, 0, 1) == AC_PROFILE_DECISION_FAIL);
}

static void test_write_all(void)
{
	const char payload[] = "abcdef";
	size_t written = 0;
	int errorNumber = 0;
	const ptrdiff_t partialWrites[] = {2, 1, 3};
	const int noErrors[] = {0, 0, 0};
	const ptrdiff_t interruptedWrite[] = {-1, 6};
	const int interruptedErrors[] = {EINTR, 0};
	const ptrdiff_t failedAfterPartial[] = {2, -1};
	const int failedErrors[] = {0, EIO};
	const ptrdiff_t zeroWrite[] = {0};
	const int zeroErrors[] = {0};

	set_write_script(partialWrites, noErrors, 3);
	assert(AC_WriteAllWith(1, payload, 6, fake_write, &written, &errorNumber) == AC_SUCCESS);
	assert(written == 6 && errorNumber == 0);

	set_write_script(interruptedWrite, interruptedErrors, 2);
	assert(AC_WriteAllWith(1, payload, 6, fake_write, &written, &errorNumber) == AC_SUCCESS);
	assert(written == 6 && errorNumber == 0);

	set_write_script(failedAfterPartial, failedErrors, 2);
	assert(AC_WriteAllWith(1, payload, 6, fake_write, &written, &errorNumber) == AC_TX_FAILED);
	assert(written == 2 && errorNumber == EIO);

	set_write_script(zeroWrite, zeroErrors, 1);
	assert(AC_WriteAllWith(1, payload, 6, fake_write, &written, &errorNumber) == AC_TX_FAILED);
	assert(written == 0 && errorNumber == EIO);
}

int main(void)
{
	test_volume_response_parser();
	test_profile_comparison();
	test_profile_retry_decisions();
	test_write_all();
	puts("AudioControl validation tests passed");
	return 0;
}