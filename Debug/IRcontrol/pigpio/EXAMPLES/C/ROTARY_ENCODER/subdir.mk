################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../IRcontrol/pigpio/EXAMPLES/C/ROTARY_ENCODER/rotary_encoder.c \
../IRcontrol/pigpio/EXAMPLES/C/ROTARY_ENCODER/test_rotary_encoder.c 

OBJS += \
./IRcontrol/pigpio/EXAMPLES/C/ROTARY_ENCODER/rotary_encoder.o \
./IRcontrol/pigpio/EXAMPLES/C/ROTARY_ENCODER/test_rotary_encoder.o 

C_DEPS += \
./IRcontrol/pigpio/EXAMPLES/C/ROTARY_ENCODER/rotary_encoder.d \
./IRcontrol/pigpio/EXAMPLES/C/ROTARY_ENCODER/test_rotary_encoder.d 


# Each subdirectory must supply rules for building sources it contributes
IRcontrol/pigpio/EXAMPLES/C/ROTARY_ENCODER/%.o: ../IRcontrol/pigpio/EXAMPLES/C/ROTARY_ENCODER/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -I"D:\SysGCC\GCC64\GCC-LI~1.11-\usr\include" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\IRcontrol\pigpio\inc" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\IRcontrol\inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


