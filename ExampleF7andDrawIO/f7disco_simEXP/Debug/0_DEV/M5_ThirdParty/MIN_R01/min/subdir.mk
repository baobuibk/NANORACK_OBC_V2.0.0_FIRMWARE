################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../0_DEV/M5_ThirdParty/MIN_R01/min/min.c 

OBJS += \
./0_DEV/M5_ThirdParty/MIN_R01/min/min.o 

C_DEPS += \
./0_DEV/M5_ThirdParty/MIN_R01/min/min.d 


# Each subdirectory must supply rules for building sources it contributes
0_DEV/M5_ThirdParty/MIN_R01/min/%.o 0_DEV/M5_ThirdParty/MIN_R01/min/%.su 0_DEV/M5_ThirdParty/MIN_R01/min/%.cyclo: ../0_DEV/M5_ThirdParty/MIN_R01/min/%.c 0_DEV/M5_ThirdParty/MIN_R01/min/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DSTM32F746xx -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -c -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M5_ThirdParty" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M3_Device" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M2_System" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M1_Driver" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M4_Util" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M0_App" -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-0_DEV-2f-M5_ThirdParty-2f-MIN_R01-2f-min

clean-0_DEV-2f-M5_ThirdParty-2f-MIN_R01-2f-min:
	-$(RM) ./0_DEV/M5_ThirdParty/MIN_R01/min/min.cyclo ./0_DEV/M5_ThirdParty/MIN_R01/min/min.d ./0_DEV/M5_ThirdParty/MIN_R01/min/min.o ./0_DEV/M5_ThirdParty/MIN_R01/min/min.su

.PHONY: clean-0_DEV-2f-M5_ThirdParty-2f-MIN_R01-2f-min

