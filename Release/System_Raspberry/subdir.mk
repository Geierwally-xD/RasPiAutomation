################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../System_Raspberry/cssl.c \
../System_Raspberry/hw.c \
../System_Raspberry/system.c 

C_DEPS += \
./System_Raspberry/cssl.d \
./System_Raspberry/hw.d \
./System_Raspberry/system.d 

OBJS += \
./System_Raspberry/cssl.o \
./System_Raspberry/hw.o \
./System_Raspberry/system.o 


# Each subdirectory must supply rules for building sources it contributes
System_Raspberry/%.o: ../System_Raspberry/%.c System_Raspberry/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-System_Raspberry

clean-System_Raspberry:
	-$(RM) ./System_Raspberry/cssl.d ./System_Raspberry/cssl.o ./System_Raspberry/hw.d ./System_Raspberry/hw.o ./System_Raspberry/system.d ./System_Raspberry/system.o

.PHONY: clean-System_Raspberry

