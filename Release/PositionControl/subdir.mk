################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../PositionControl/PositionControl.c \
../PositionControl/PositionDirect.c \
../PositionControl/ZoomControl.c 

C_DEPS += \
./PositionControl/PositionControl.d \
./PositionControl/PositionDirect.d \
./PositionControl/ZoomControl.d 

OBJS += \
./PositionControl/PositionControl.o \
./PositionControl/PositionDirect.o \
./PositionControl/ZoomControl.o 


# Each subdirectory must supply rules for building sources it contributes
PositionControl/%.o: ../PositionControl/%.c PositionControl/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-PositionControl

clean-PositionControl:
	-$(RM) ./PositionControl/PositionControl.d ./PositionControl/PositionControl.o ./PositionControl/PositionDirect.d ./PositionControl/PositionDirect.o ./PositionControl/ZoomControl.d ./PositionControl/ZoomControl.o

.PHONY: clean-PositionControl

