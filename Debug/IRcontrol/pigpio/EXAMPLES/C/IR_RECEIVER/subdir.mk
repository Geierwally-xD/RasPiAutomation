################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../IRcontrol/pigpio/EXAMPLES/C/IR_RECEIVER/ir_hasher.c \
../IRcontrol/pigpio/EXAMPLES/C/IR_RECEIVER/test_ir_hasher.c 

OBJS += \
./IRcontrol/pigpio/EXAMPLES/C/IR_RECEIVER/ir_hasher.o \
./IRcontrol/pigpio/EXAMPLES/C/IR_RECEIVER/test_ir_hasher.o 

C_DEPS += \
./IRcontrol/pigpio/EXAMPLES/C/IR_RECEIVER/ir_hasher.d \
./IRcontrol/pigpio/EXAMPLES/C/IR_RECEIVER/test_ir_hasher.d 


# Each subdirectory must supply rules for building sources it contributes
IRcontrol/pigpio/EXAMPLES/C/IR_RECEIVER/%.o: ../IRcontrol/pigpio/EXAMPLES/C/IR_RECEIVER/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -I"D:\SysGCC\GCC64\GCC-LI~1.11-\usr\include" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\IRcontrol\pigpio\inc" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\IRcontrol\inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


