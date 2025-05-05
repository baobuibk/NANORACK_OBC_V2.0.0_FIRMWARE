/*
 * management.c
 *
 *  Created on: Feb 26, 2025
 *      Author: CAO HIEU
 */

#include "management.h"
#include "rtos_tasks.h"
#include "utils.h"
#include "rtos.h"
#include "uart_driver_dma.h"
#include "main.h"
#include "board.h"
#include "common_state.h"

#include "filesystem.h"
#include "CLI_Terminal/CLI_Setup/cli_setup.h"
#include "SysLog_Queue/syslog_queue.h"
#include "DateTime/date_time.h"
#include "cdc_driver.h"
#include "devices.h"
#include "SPI_FRAM/fram_spi.h"
#include "IO_ExWD-TPL5010/wd_tpl5010.h"

SystemStatus_t Mgmt_GetSystemStatus(void);

Std_ReturnType Mgmt_HardwareSystemPreparing(void)
{
	Std_ReturnType ret = E_ERROR;
	system_status.init_state = INIT_STATE_STEP_PREPARING;

	RV3129_Driver_Init(I2C_RTC);
	FRAM_SPI_Driver_Init(SPI_MEM, FRAM_CS_Port, FRAM_CS);
	ret = UART_DMA_Driver_Init();

    Watchdog_Device_Init();

	Utils_SoftTime_Init();
	CDC_RingBuffer_Init();

	SYSLOG_NOTICE_POLL("STM32-OBC Boot-up!");
	SYSLOG_INFO_POLL("STM32-OBC Boot-up!");
	SYSLOG_DEBUG_POLL("STM32-OBC Boot-up!");
	SYSLOG_WARN_POLL("STM32-OBC Boot-up!");
	SYSLOG_ERROR_POLL("STM32-OBC Boot-up!");
	SYSLOG_FATAL_POLL("STM32-OBC Boot-up!");

	return ret;
}

void Mgmt_SystemStart(void){
	SYSLOG_NOTICE_POLL("OS starting");
	OBC_RTOS_Start();
}

//********************************************** RTOS Applied ****************************************************

/*************************************************
 *                   RTOS Control                *
 *************************************************/
Std_ReturnType Mgmt_SystemInitStepZero(void)
{
	Std_ReturnType ret = E_ERROR;
	system_status.init_state = INIT_STATE_STEP_ZERO;

	ret = Utils_SoftTime_Sync();
	if(Utils_SoftTime_Sync() == E_OK){
		UART_Driver_SendString(UART_DEBUG,"\r\n[Sync Time!]\r\n");
	}else{
		system_status.last_error = ret;
		system_status.init_state = INIT_STATE_FAILED;
	}

	if(ret != E_OK) return ret;
	UART_Driver_SendString(UART_DEBUG, "System-Init Step 0: ");
	UART_Driver_SendString(UART_DEBUG, "OK!\r\n");
	return ret;
}

Std_ReturnType Mgmt_SystemInitStepOne(void)
{
	system_status.init_state = INIT_STATE_STEP_ONE;
	UART_Driver_SendString(UART_DEBUG, "System-Init Step 1: ");
	Std_ReturnType ret = E_ERROR;

	ret = SystemCLI_Init();
	if(ret != E_OK){
		UART_Driver_SendString(UART_DEBUG, "\r\n[CLI Init Fail]\r\n");
		system_status.last_error = ret;
		system_status.init_state = INIT_STATE_FAILED;
	}

	ret = Link_SDFS_Driver();
	if(ret != E_OK){
		UART_Driver_SendString(UART_DEBUG, "\r\n[Link FATFS Fail]\r\n");
		system_status.last_error = ret;
		system_status.init_state = INIT_STATE_FAILED;
	}else{
		UART_Driver_SendString(UART_DEBUG, "\r\n[Link FATFS Successfully]\r\n");
	}

	SysLogQueue_Init();

	if(ret != E_OK) return ret;
	UART_Driver_SendString(UART_DEBUG, "OK!\r\n");
	return ret;
}

Std_ReturnType Mgmt_SystemInitStepTwo(void)
{
	system_status.init_state = INIT_STATE_STEP_TWO;
	UART_Driver_SendString(UART_DEBUG, "System-Init Step 2: ");
	UART_Driver_SendString(UART_DEBUG, "OK!\r\n");
	return E_OK;
}

Std_ReturnType Mgmt_SystemInitFinal(void)
{
	system_status.init_state = INIT_STATE_FINAL;
	UART_Driver_SendString(UART_DEBUG, "System-Init Final: ");
	UART_Driver_SendString(UART_DEBUG, "OK!\r\n");
	system_status.init_state = INIT_STATE_COMPLETED;
	return E_OK;
}

SystemStatus_t Mgmt_GetSystemStatus(void)
{
    return system_status;
}
