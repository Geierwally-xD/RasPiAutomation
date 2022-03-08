################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../TestProg/TestProg.c 

C_DEPS += \
./TestProg/TestProg.d 

OBJS += \
./TestProg/TestProg.o 


# Each subdirectory must supply rules for building sources it contributes
TestProg/%.o: ../TestProg/%.c TestProg/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	aarch64-linux-gnu-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-TestProg

clean-TestProg:
	-$(RM) ./TestProg/TestProg.d ./TestProg/TestProg.o

.PHONY: clean-TestProg

