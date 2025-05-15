/*
 * mgmt.c
 *
 *  Created on: Apr 17, 2025
 *      Author: CAO HIEU
 */

#include "Mgmt/mgmt.h"
#include "Logger/log.h"
#include "stdio.h"
#include "main.h"

/*************************************************
 *                    Helpper                    *
 *************************************************/
#include "OS/Scheduler/scheduler.h"
#include "Task/scheduler_tasks.h"
#include "MIN_R01/min_proto.h"
#include "MIN_Process/min_process.h"
#include "UART/uart_driver.h"
#include "Struct/gpio_state.h"
#include "CLI_Terminal/CLI_Setup/cli_setup.h"
#include "QSPI/quadspi.h"
#include "SPI_SlaveOfOBC/spi_slave.h"
/*************************************************
 *                Native Define                  *
 *************************************************/

/*************************************************
 *                  Init Step                    *
 *************************************************/
void Mgmt_SystemInitStepZero(void)
{
	toOBC_Init();
	Utils_SoftTime_Init();
	(void)SPI_SlaveDevice_Init();
	SystemCLI_Init();
    LOG("Step 0A - OK!");
    if (CSP_QUADSPI_Init() != HAL_OK){
    	LOG("QuadSPI - Error!");
    };
	UART_Driver_Init();
	MIN_Process_Init();
    LOG("Step 0Z - OK!");
}

void Mgmt_SystemInitFinal(void)
{
//	SystemCLI_Init();
	SCH_Initialize();
	SchedulerTasks_Create();
    LOG("Step Final - OK!");
}

void Mgmt_SystemStart(void)
{
    LOG("Start System!");
	SCH_StartScheduler();
	while(1){
		SCH_HandleScheduledTask();
	}
}

/*************************************************
 *                     Tester                    *
 *************************************************/

void __Mgmt_TESTER(void){
	while (1)
	{
		LL_GPIO_ResetOutputPin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin);
		HAL_Delay(1000);
		LL_GPIO_SetOutputPin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin);
		HAL_Delay(1000);
	}
}

