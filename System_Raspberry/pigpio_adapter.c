#include "pigpio_adapter.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* pigpio direct API — requires root (or gpio group) and pigpiod must NOT be running.
   Stop daemon before launching: sudo systemctl stop pigpiod */
typedef int  (*gpioInitialise_fn)(void);
typedef void (*gpioTerminate_fn)(void);
typedef int  (*gpioSetMode_fn)(unsigned, unsigned);
typedef int  (*gpioWrite_fn)(unsigned, unsigned);
typedef int  (*gpioRead_fn)(unsigned);
typedef int  (*i2cOpen_fn)(unsigned, unsigned, unsigned);
typedef int  (*i2cClose_fn)(unsigned);
typedef int  (*i2cReadByteData_fn)(unsigned, unsigned);
typedef int  (*i2cWriteByteData_fn)(unsigned, unsigned, unsigned);
typedef int  (*gpioServo_fn)(unsigned, unsigned);

typedef struct PigpioApi {
	void *handle;
	gpioInitialise_fn gpioInitialise;
	gpioTerminate_fn  gpioTerminate;
	gpioSetMode_fn    gpioSetMode;
	gpioWrite_fn      gpioWrite;
	gpioRead_fn       gpioRead;
	i2cOpen_fn        i2cOpen;
	i2cClose_fn       i2cClose;
	i2cReadByteData_fn  i2cReadByteData;
	i2cWriteByteData_fn i2cWriteByteData;
	gpioServo_fn      gpioServo;
} PigpioApi;

static PigpioApi pigpioApi = {0};
static int pigpioReady = 0;
static char pigpioLastError[256] = "pigpio has not been initialized";

static void set_last_error(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsnprintf(pigpioLastError, sizeof(pigpioLastError), format, args);
	va_end(args);
}

static int ensure_pigpio_ready(void)
{
	if (!pigpioReady)
	{
		set_last_error("pigpio not initialized");
		return -1;
	}

	if (pigpioApi.handle == NULL)
	{
		set_last_error("pigpio runtime library not loaded");
		return -1;
	}

	return 0;
}

const char *RPI_LastError(void)
{
	return pigpioLastError;
}

static int map_legacy_pin_to_bcm(int pin)
{
	switch (pin)
	{
		case 0: return 17;
		case 1: return 18;
		case 2: return 27;
		case 3: return 22;
		case 4: return 23;
		case 5: return 24;
		case 6: return 25;
		case 7: return 4;
		case 8: return 2;
		case 9: return 3;
		case 10: return 8;
		case 11: return 7;
		case 12: return 10;
		case 13: return 9;
		case 14: return 11;
		case 15: return 14;
		case 16: return 15;
		case 21: return 5;
		case 22: return 6;
		case 23: return 13;
		case 24: return 19;
		case 25: return 26;
		case 26: return 12;
		case 27: return 16;
		case 28: return 20;
		case 29: return 21;
		case 30: return 0;
		case 31: return 1;
		default: return -1;
	}
}

static int load_symbol(void **target, const char *symbol)
{
	*target = dlsym(pigpioApi.handle, symbol);
	if (*target == NULL)
	{
		set_last_error("pigpio symbol %s not found: %s", symbol, dlerror());
		return -1;
	}
	return 0;
}

static int load_pigpio_library(void)
{
	/* pigpio direct API is in libpigpio.so */
	static const char *candidates[] = { "libpigpio.so.1", "libpigpio.so" };
	size_t i;

	if (pigpioApi.handle != NULL)
	{
		return 0;
	}

	for (i = 0; i < (sizeof(candidates) / sizeof(candidates[0])); i++)
	{
		pigpioApi.handle = dlopen(candidates[i], RTLD_NOW | RTLD_LOCAL);
		if (pigpioApi.handle != NULL)
		{
			break;
		}
	}

	if (pigpioApi.handle == NULL)
	{
		set_last_error("Unable to load pigpio library (libpigpio.so). dlerror=%s", dlerror());
		return -1;
	}

	if (load_symbol((void **)&pigpioApi.gpioInitialise, "gpioInitialise") < 0 ||
		load_symbol((void **)&pigpioApi.gpioTerminate,  "gpioTerminate")  < 0 ||
		load_symbol((void **)&pigpioApi.gpioSetMode,    "gpioSetMode")    < 0 ||
		load_symbol((void **)&pigpioApi.gpioWrite,      "gpioWrite")      < 0 ||
		load_symbol((void **)&pigpioApi.gpioRead,       "gpioRead")       < 0 ||
		load_symbol((void **)&pigpioApi.i2cOpen,        "i2cOpen")        < 0 ||
		load_symbol((void **)&pigpioApi.i2cClose,       "i2cClose")       < 0 ||
		load_symbol((void **)&pigpioApi.i2cReadByteData,  "i2cReadByteData")  < 0 ||
		load_symbol((void **)&pigpioApi.i2cWriteByteData, "i2cWriteByteData") < 0 ||
		load_symbol((void **)&pigpioApi.gpioServo,      "gpioServo")      < 0)
	{
		dlclose(pigpioApi.handle);
		memset(&pigpioApi, 0, sizeof(pigpioApi));
		return -1;
	}

	return 0;
}

int RPI_Initialize(void)
{
	int rc;

	if (pigpioReady)
	{
		set_last_error("pigpio already initialized");
		return 0;
	}

	if (load_pigpio_library() < 0)
	{
		return -1;
	}

	/* Connect directly to hardware — pigpiod must NOT be running. */
	rc = pigpioApi.gpioInitialise();
	if (rc < 0)
	{
		set_last_error("gpioInitialise() failed with code %d. Stop pigpiod first: sudo systemctl stop pigpiod. Then run this application as root or with sudo.", rc);
		return -1;
	}

	pigpioReady = 1;
	set_last_error("pigpio initialized");
	return 0;
}

void RPI_Terminate(void)
{
	if (pigpioReady)
	{
		pigpioApi.gpioTerminate();
		pigpioReady = 0;
		set_last_error("pigpio terminated");
	}

	if (pigpioApi.handle != NULL)
	{
		dlclose(pigpioApi.handle);
		memset(&pigpioApi, 0, sizeof(pigpioApi));
	}
}

int RPI_SetMode(int pin, int mode)
{
	int rc;
	int bcmPin = map_legacy_pin_to_bcm(pin);
	if (ensure_pigpio_ready() < 0)
	{
		return -1;
	}
	if (bcmPin < 0)
	{
		set_last_error("Unsupported legacy GPIO pin %d", pin);
		return -1;
	}
	rc = pigpioApi.gpioSetMode((unsigned)bcmPin, (unsigned)mode);
	if (rc < 0)
	{
		set_last_error("gpioSetMode failed for legacy pin %d (BCM %d), mode %d: %d", pin, bcmPin, mode, rc);
	}
	return rc;
}

int RPI_Write(int pin, int value)
{
	int rc;
	int bcmPin = map_legacy_pin_to_bcm(pin);
	if (ensure_pigpio_ready() < 0)
	{
		return -1;
	}
	if (bcmPin < 0)
	{
		set_last_error("Unsupported legacy GPIO pin %d", pin);
		return -1;
	}
	rc = pigpioApi.gpioWrite((unsigned)bcmPin, (unsigned)(value != 0));
	if (rc < 0)
	{
		set_last_error("gpioWrite failed for legacy pin %d (BCM %d), value %d: %d", pin, bcmPin, value, rc);
	}
	return rc;
}

int RPI_Read(int pin)
{
	int rc;
	int bcmPin = map_legacy_pin_to_bcm(pin);
	if (ensure_pigpio_ready() < 0)
	{
		return -1;
	}
	if (bcmPin < 0)
	{
		set_last_error("Unsupported legacy GPIO pin %d", pin);
		return -1;
	}
	rc = pigpioApi.gpioRead((unsigned)bcmPin);
	if (rc < 0)
	{
		set_last_error("gpioRead failed for legacy pin %d (BCM %d): %d", pin, bcmPin, rc);
	}
	return rc;
}

int RPI_I2COpen(int address)
{
	int rc;
	if (ensure_pigpio_ready() < 0)
	{
		return -1;
	}
	rc = pigpioApi.i2cOpen(1u, (unsigned)address, 0u);
	if (rc < 0)
	{
		set_last_error("i2cOpen failed for address 0x%02X: %d", address & 0xFF, rc);
	}
	return rc;
}

int RPI_I2CClose(int handle)
{
	int rc;
	if (ensure_pigpio_ready() < 0)
	{
		return -1;
	}
	rc = pigpioApi.i2cClose((unsigned)handle);
	if (rc < 0)
	{
		set_last_error("i2cClose failed for handle %d: %d", handle, rc);
	}
	return rc;
}

int RPI_I2CReadReg8(int handle, int reg)
{
	int rc;
	if (ensure_pigpio_ready() < 0)
	{
		return -1;
	}
	rc = pigpioApi.i2cReadByteData((unsigned)handle, (unsigned)reg);
	if (rc < 0)
	{
		set_last_error("i2cReadByteData failed for handle %d, reg 0x%02X: %d", handle, reg & 0xFF, rc);
	}
	return rc;
}

int RPI_I2CWriteReg8(int handle, int reg, int value)
{
	int rc;
	if (ensure_pigpio_ready() < 0)
	{
		return -1;
	}
	rc = pigpioApi.i2cWriteByteData((unsigned)handle, (unsigned)reg, (unsigned)value);
	if (rc < 0)
	{
		set_last_error("i2cWriteByteData failed for handle %d, reg 0x%02X: %d", handle, reg & 0xFF, rc);
	}
	return rc;
}

int RPI_ServoPulse(int pin, unsigned pulseWidthMicros)
{
	int rc;
	int bcmPin = map_legacy_pin_to_bcm(pin);
	if (ensure_pigpio_ready() < 0)
	{
		return -1;
	}
	if (bcmPin < 0)
	{
		set_last_error("Unsupported legacy GPIO pin %d", pin);
		return -1;
	}
	rc = pigpioApi.gpioServo((unsigned)bcmPin, pulseWidthMicros);
	if (rc < 0)
	{
		set_last_error("gpioServo failed for legacy pin %d (BCM %d), pulse %u: %d", pin, bcmPin, pulseWidthMicros, rc);
	}
	return rc;
}

int RPI_ServoOff(int pin)
{
	return RPI_ServoPulse(pin, 0u);
}