/*
 * scheduler_tasks.c
 *
 *  Created on: Apr 17, 2025
 *      Author: CAO HIEU
 */

#include "scheduler_tasks.h"
#include "OS/Scheduler/scheduler.h"

typedef struct {
    SCH_TASK_HANDLE           taskHandle;
    SCH_TaskPropertyTypedef   taskProperty;
} TaskEntry;

static SCH_TASK_HANDLE asyncTaskHandle;

/*************************************************
 *                 User Include                  *
 *************************************************/
#include "main.h"
#include "Logger/log.h"
#include "CLI_Terminal/CLI_Command/cli_command.h"
#include "CLI_Terminal/CLI_Setup/cli_setup.h"
#include "MIN_Process/min_process.h"
/*************************************************
 *                  Task Define                  *
 *************************************************/
extern UART_HandleTypeDef huart1;

static void BlinkLed_Task(void);
static void HelloWorld_Task(void);
static void CliProcess_Task(void);
static void MINProcess_Task(void);


/*************************************************
 *                   Task Infor                  *
 *************************************************/

static TaskEntry schedulerTasks[] = {
    { SCH_DEFAULT_TASK_HANDLE, { SCH_TASK_SYNC, SCH_TASK_PRIO_0, 1000, 	BlinkLed_Task, 			0 } },
    { SCH_DEFAULT_TASK_HANDLE, { SCH_TASK_SYNC, SCH_TASK_PRIO_0, 100,  	HelloWorld_Task, 		0 } },
    { SCH_DEFAULT_TASK_HANDLE, { SCH_TASK_SYNC, SCH_TASK_PRIO_0, 1,  	CliProcess_Task, 		0 } },
    { SCH_DEFAULT_TASK_HANDLE, { SCH_TASK_SYNC, SCH_TASK_PRIO_0, 1,  	MINProcess_Task, 		0 } }

};

/*************************************************
 *                    Task List                  *
 *************************************************/


static void BlinkLed_Task(void)
{
    static uint8_t ledState = 0;
    if (ledState == 0)
    {
    	LL_GPIO_SetOutputPin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin);
        ledState = 1;
    }
    else
    {
    	LL_GPIO_ResetOutputPin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin);
        ledState = 0;
    }
}

static void HelloWorld_Task(void)
{
//	LOG("HelloWorld!");
}

static uint8_t rx_byte;
static uint8_t inited = E_ERROR;

static void CliProcess_Task(void)
{
	if(inited == E_ERROR){
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
        inited = E_OK;
	}
    embeddedCliProcess(getCliPointer());
}

static void MINProcess_Task(void)
{
	MIN_Processing();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
    	embeddedCliReceiveChar(getCliPointer(), (char)rx_byte);
        HAL_UART_Receive_IT(huart, &rx_byte, 1);
    }
}


/*************************************************
 *                  Helper Define                *
 *************************************************/

#define NUM_SCHEDULER_TASKS (sizeof(schedulerTasks) / sizeof(schedulerTasks[0]))

void SchedulerTasks_Create(void)
{
    for (uint8_t i = 0; i < NUM_SCHEDULER_TASKS; i++) {
        SCH_TASK_CreateTask(&schedulerTasks[i].taskHandle, &schedulerTasks[i].taskProperty);
        if (schedulerTasks[i].taskProperty.taskType == SCH_TASK_ASYNC) {
            asyncTaskHandle = schedulerTasks[i].taskHandle;
        }
    }
}

