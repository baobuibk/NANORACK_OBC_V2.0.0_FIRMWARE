################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../USB_DEVICE/Target/usbd_conf.c 

OBJS += \
./USB_DEVICE/Target/usbd_conf.o 

C_DEPS += \
./USB_DEVICE/Target/usbd_conf.d 


# Each subdirectory must supply rules for building sources it contributes
USB_DEVICE/Target/%.o USB_DEVICE/Target/%.su USB_DEVICE/Target/%.cyclo: ../USB_DEVICE/Target/%.c USB_DEVICE/Target/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32H745xx -DUSE_PWR_LDO_SUPPLY -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM4/0_DEV/_Target4" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM4/0_DEV/M0_App4" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM4/0_DEV/M1_Driver4" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM4/0_DEV/M2_System4" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM4/0_DEV/M3_Device4" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM4/0_DEV/M4_Util4" -I"D:/LAB_PROJECT_7.7.7.7.7.7.7/stm32cubeide/04_obc_v111/CM4/0_DEV/M5_ThirdParty4" -I../../Drivers/STM32H7xx_HAL_Driver/Inc -I../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-USB_DEVICE-2f-Target

clean-USB_DEVICE-2f-Target:
	-$(RM) ./USB_DEVICE/Target/usbd_conf.cyclo ./USB_DEVICE/Target/usbd_conf.d ./USB_DEVICE/Target/usbd_conf.o ./USB_DEVICE/Target/usbd_conf.su

.PHONY: clean-USB_DEVICE-2f-Target

