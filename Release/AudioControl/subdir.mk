################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../AudioControl/AudioControl.c 

OBJS += \
./AudioControl/AudioControl.o 

C_DEPS += \
./AudioControl/AudioControl.d 


# Each subdirectory must supply rules for building sources it contributes
AudioControl/%.o: ../AudioControl/%.c AudioControl/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


