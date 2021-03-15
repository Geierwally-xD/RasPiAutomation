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
AudioControl/%.o: ../AudioControl/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\AudioControl" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\PositionControl" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\System_Raspberry" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\Command_Interpreter" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\Timer" -I"D:\RV\EMU\RasPiAutomation\RasPiAutomation\IRcontrol" -I"D:\SysGCC\GCC64\GCC-LI~1.11-\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


