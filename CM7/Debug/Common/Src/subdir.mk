################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.c 

C_DEPS += \
./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.d 

OBJS += \
./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o 


# Each subdirectory must supply rules for building sources it contributes
Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o: D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.c Common/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCORE_CM7 -DUSE_HAL_DRIVER -DSTM32H745xx -DUSE_PWR_LDO_SUPPLY -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M1_Drivers/Shared_REG" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M5_ThirdParty/MIN_R01" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M2_System/FileSystem" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M2_System" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M5_ThirdParty/FatFS_R015/Core" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M5_ThirdParty/FatFS_R015/Drivers" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M5_ThirdParty/FatFS_R015/Option" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M2_System" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M3_Devices" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/_Target" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M4_Utils" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M0_App/Mgmt" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M0_App/NoOS" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M0_App/OS" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M0_App/Structs" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M0_App/Tasks" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M0_App" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M1_Drivers/I2C" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M1_Drivers/IPC" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M1_Drivers/SDMMC" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M1_Drivers/SPI" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M1_Drivers/UART" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M1_Drivers/UART_DMA" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M5_ThirdParty/FreeRTOS/FreeRTOS_Config" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M5_ThirdParty/FreeRTOS/FreeRTOS-Kernel/include" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M5_ThirdParty/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M5_ThirdParty/FreeRTOS/FreeRTOS-Kernel/portable/MemMang" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM7/1_DEV/M0_App" -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Common-2f-Src

clean-Common-2f-Src:
	-$(RM) ./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.cyclo ./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.d ./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.o ./Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.su

.PHONY: clean-Common-2f-Src

