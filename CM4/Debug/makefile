################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include USB_DEVICE/Target/subdir.mk
-include USB_DEVICE/App/subdir.mk
-include Middlewares/ST/STM32_USB_Device_Library/subdir.mk
-include Drivers/STM32H7xx_HAL_Driver/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include Common/Src/subdir.mk
-include 0_DEV/M4_Util4/Tick/subdir.mk
-include 0_DEV/M4_Util4/RingBuffer/subdir.mk
-include 0_DEV/M1_Driver4/USB_CDC/subdir.mk
-include 0_DEV/M1_Driver4/UART_DMA/subdir.mk
-include 0_DEV/M1_Driver4/Shared_REG/subdir.mk
-include 0_DEV/M0_App4/Task4/subdir.mk
-include 0_DEV/M0_App4/OS4/subdir.mk
-include 0_DEV/M0_App4/Mgmt4/subdir.mk
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


BUILD_ARTIFACT_NAME := 04_obc_v111_CM4
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
04_obc_v111_CM4.elf \

MAP_FILES += \
04_obc_v111_CM4.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
04_obc_v111_CM4.list \

OBJCOPY_HEX += \
04_obc_v111_CM4.hex \

OBJCOPY_BIN += \
04_obc_v111_CM4.bin \


# All Target
all: main-build

# Main-build Target
main-build: 04_obc_v111_CM4.elf secondary-outputs

# Tool invocations
04_obc_v111_CM4.elf 04_obc_v111_CM4.map: $(OBJS) $(USER_OBJS) D:\LAB_PROJECT_7.7.7.7.7.7.7\stm32cubeide\04_obc_v111\CM4\STM32H745ZITX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "04_obc_v111_CM4.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"D:\LAB_PROJECT_7.7.7.7.7.7.7\stm32cubeide\04_obc_v111\CM4\STM32H745ZITX_FLASH.ld" --specs=nosys.specs -Wl,-Map="04_obc_v111_CM4.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

04_obc_v111_CM4.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "04_obc_v111_CM4.list"
	@echo 'Finished building: $@'
	@echo ' '

04_obc_v111_CM4.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "04_obc_v111_CM4.hex"
	@echo 'Finished building: $@'
	@echo ' '

04_obc_v111_CM4.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "04_obc_v111_CM4.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) 04_obc_v111_CM4.bin 04_obc_v111_CM4.elf 04_obc_v111_CM4.hex 04_obc_v111_CM4.list 04_obc_v111_CM4.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
