################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32F7xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include 0_DEV/M5_ThirdParty/MIN_R01/min_app/subdir.mk
-include 0_DEV/M5_ThirdParty/MIN_R01/min/subdir.mk
-include 0_DEV/M4_Util/RingBuffer/subdir.mk
-include 0_DEV/M4_Util/Logger/subdir.mk
-include 0_DEV/M4_Util/DateTime/subdir.mk
-include 0_DEV/M3_Device/SPI_SlaveOfOBC/subdir.mk
-include 0_DEV/M2_System/MIN_Process/subdir.mk
-include 0_DEV/M2_System/CLI_Terminal/CLI_Src/subdir.mk
-include 0_DEV/M2_System/CLI_Terminal/CLI_Setup/subdir.mk
-include 0_DEV/M2_System/CLI_Terminal/CLI_Command/subdir.mk
-include 0_DEV/M1_Driver/UART/subdir.mk
-include 0_DEV/M1_Driver/QSPI/subdir.mk
-include 0_DEV/M0_App/Task/subdir.mk
-include 0_DEV/M0_App/Struct/subdir.mk
-include 0_DEV/M0_App/OS/Scheduler/subdir.mk
-include 0_DEV/M0_App/Mgmt/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := f7disco_simEXP
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
f7disco_simEXP.elf \

MAP_FILES += \
f7disco_simEXP.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
f7disco_simEXP.list \

OBJCOPY_HEX += \
f7disco_simEXP.hex \


# All Target
all: main-build

# Main-build Target
main-build: f7disco_simEXP.elf secondary-outputs

# Tool invocations
f7disco_simEXP.elf f7disco_simEXP.map: $(OBJS) $(USER_OBJS) D:\LAB_PROJECT_7.7.7.7.7.7.7\stm32cubeide\f7disco_simEXP\STM32F746NGHX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "f7disco_simEXP.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m7 -T"D:\LAB_PROJECT_7.7.7.7.7.7.7\stm32cubeide\f7disco_simEXP\STM32F746NGHX_FLASH.ld" --specs=nosys.specs -Wl,-Map="f7disco_simEXP.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

f7disco_simEXP.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "f7disco_simEXP.list"
	@echo 'Finished building: $@'
	@echo ' '

f7disco_simEXP.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "f7disco_simEXP.hex"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) default.size.stdout f7disco_simEXP.elf f7disco_simEXP.hex f7disco_simEXP.list f7disco_simEXP.map
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
