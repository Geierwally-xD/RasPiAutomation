################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../IRcontrol/IRcontrol.c 

C_DEPS += \
./IRcontrol/IRcontrol.d 

OBJS += \
./IRcontrol/IRcontrol.o 


# Each subdirectory must supply rules for building sources it contributes
IRcontrol/%.o: ../IRcontrol/%.c IRcontrol/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-IRcontrol

clean-IRcontrol:
	-$(RM) ./IRcontrol/IRcontrol.d ./IRcontrol/IRcontrol.o

.PHONY: clean-IRcontrol

