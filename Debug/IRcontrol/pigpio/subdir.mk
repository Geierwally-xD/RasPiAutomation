################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../IRcontrol/pigpio/command.c \
../IRcontrol/pigpio/pig2vcd.c \
../IRcontrol/pigpio/pigpio.c \
../IRcontrol/pigpio/pigpiod.c \
../IRcontrol/pigpio/pigpiod_if.c \
../IRcontrol/pigpio/pigpiod_if2.c \
../IRcontrol/pigpio/pigs.c \
../IRcontrol/pigpio/x_pigpio.c \
../IRcontrol/pigpio/x_pigpiod_if.c \
../IRcontrol/pigpio/x_pigpiod_if2.c 

OBJS += \
./IRcontrol/pigpio/command.o \
./IRcontrol/pigpio/pig2vcd.o \
./IRcontrol/pigpio/pigpio.o \
./IRcontrol/pigpio/pigpiod.o \
./IRcontrol/pigpio/pigpiod_if.o \
./IRcontrol/pigpio/pigpiod_if2.o \
./IRcontrol/pigpio/pigs.o \
./IRcontrol/pigpio/x_pigpio.o \
./IRcontrol/pigpio/x_pigpiod_if.o \
./IRcontrol/pigpio/x_pigpiod_if2.o 

C_DEPS += \
./IRcontrol/pigpio/command.d \
./IRcontrol/pigpio/pig2vcd.d \
./IRcontrol/pigpio/pigpio.d \
./IRcontrol/pigpio/pigpiod.d \
./IRcontrol/pigpio/pigpiod_if.d \
./IRcontrol/pigpio/pigpiod_if2.d \
./IRcontrol/pigpio/pigs.d \
./IRcontrol/pigpio/x_pigpio.d \
./IRcontrol/pigpio/x_pigpiod_if.d \
./IRcontrol/pigpio/x_pigpiod_if2.d 


# Each subdirectory must supply rules for building sources it contributes
IRcontrol/pigpio/%.o: ../IRcontrol/pigpio/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -I"D:\SysGCC\GCC64\GCC-LI~1.11-\usr\include" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\IRcontrol\pigpio\inc" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\IRcontrol\inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


