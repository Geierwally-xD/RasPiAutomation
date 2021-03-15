################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Timer/subdir.mk
-include TestProg/subdir.mk
-include System_Raspberry/subdir.mk
-include PositionControl/subdir.mk
-include IRcontrol/subdir.mk
-include Command_Interpreter/subdir.mk
-include AudioControl/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: RasPiAutomation

# Tool invocations
RasPiAutomation: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Linker'
	aarch64-linux-gnu-gcc -L"D:\SysGCC\GCC64\GCC-LI~1.11-\usr\lib" -lm -ldl -o "RasPiAutomation" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) RasPiAutomation
	-@echo ' '

.PHONY: all clean dependents

-include ../makefile.targets
