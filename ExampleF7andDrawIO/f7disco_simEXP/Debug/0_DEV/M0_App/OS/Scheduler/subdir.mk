################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../0_DEV/M0_App/OS/Scheduler/scheduler.c 

OBJS += \
./0_DEV/M0_App/OS/Scheduler/scheduler.o 

C_DEPS += \
./0_DEV/M0_App/OS/Scheduler/scheduler.d 


# Each subdirectory must supply rules for building sources it contributes
0_DEV/M0_App/OS/Scheduler/%.o 0_DEV/M0_App/OS/Scheduler/%.su 0_DEV/M0_App/OS/Scheduler/%.cyclo: ../0_DEV/M0_App/OS/Scheduler/%.c 0_DEV/M0_App/OS/Scheduler/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DSTM32F746xx -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -c -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M5_ThirdParty" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M3_Device" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M2_System" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M1_Driver" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M4_Util" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/f7disco_simEXP/0_DEV/M0_App" -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-0_DEV-2f-M0_App-2f-OS-2f-Scheduler

clean-0_DEV-2f-M0_App-2f-OS-2f-Scheduler:
	-$(RM) ./0_DEV/M0_App/OS/Scheduler/scheduler.cyclo ./0_DEV/M0_App/OS/Scheduler/scheduler.d ./0_DEV/M0_App/OS/Scheduler/scheduler.o ./0_DEV/M0_App/OS/Scheduler/scheduler.su

.PHONY: clean-0_DEV-2f-M0_App-2f-OS-2f-Scheduler

