################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../PositionControl/PositionControl.c \
../PositionControl/PositionDirect.c 

OBJS += \
./PositionControl/PositionControl.o \
./PositionControl/PositionDirect.o 

C_DEPS += \
./PositionControl/PositionControl.d \
./PositionControl/PositionDirect.d 


# Each subdirectory must supply rules for building sources it contributes
PositionControl/%.o: ../PositionControl/%.c PositionControl/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


