################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Timer/timer.c 

C_DEPS += \
./Timer/timer.d 

OBJS += \
./Timer/timer.o 


# Each subdirectory must supply rules for building sources it contributes
Timer/%.o: ../Timer/%.c Timer/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Timer

clean-Timer:
	-$(RM) ./Timer/timer.d ./Timer/timer.o

.PHONY: clean-Timer

