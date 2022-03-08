################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Command_Interpreter/ComInt.c 

C_DEPS += \
./Command_Interpreter/ComInt.d 

OBJS += \
./Command_Interpreter/ComInt.o 


# Each subdirectory must supply rules for building sources it contributes
Command_Interpreter/%.o: ../Command_Interpreter/%.c Command_Interpreter/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-Command_Interpreter

clean-Command_Interpreter:
	-$(RM) ./Command_Interpreter/ComInt.d ./Command_Interpreter/ComInt.o

.PHONY: clean-Command_Interpreter

