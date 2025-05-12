#include "scheduler_tasks.h"

typedef struct
{
    SCH_TASK_HANDLE taskHandle;
    SCH_TaskPropertyTypedef taskProperty;
} TaskEntry;

static SCH_TASK_HANDLE asyncTaskHandle;

/*************************************************
 *                 User Include                  *
 *************************************************/
#include "main.h"

/*************************************************
 *                  Task Define                  *
 *************************************************/
static void BlinkLed1_Task(void);
static void BlinkLed2_Task(void);

/*************************************************
 *                   Task Infor                  *
 *************************************************/
/**
 * @brief Example task definitions. Users can modify or define their own tasks.
 * e.g BlinkLed 1000ms cycle, HelloWorld as Async trigger by BlinkLed->On
 */
static TaskEntry schedulerTasks[] = {
    {SCH_DEFAULT_TASK_HANDLE, {SCH_TASK_SYNC,  SCH_TASK_PRIO_0, 1000,    BlinkLed1_Task,     0}},
    {SCH_DEFAULT_TASK_HANDLE, {SCH_TASK_SYNC,  SCH_TASK_PRIO_0, 1000,    BlinkLed2_Task,     0}},
};

/*************************************************
 *                    Task List                  *
 *************************************************/
static void BlinkLed1_Task(void)
{
    static uint8_t ledState = 0;
    if (ledState == 0)
    {
        ledState = 1;
        LL_GPIO_SetOutputPin(MCU_IO_DEBUG_LED2_GPIO_Port, MCU_IO_DEBUG_LED2_Pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(MCU_IO_DEBUG_LED2_GPIO_Port, MCU_IO_DEBUG_LED2_Pin);
        ledState = 0;
    }
}

static void BlinkLed2_Task(void)
{
    static uint8_t ledState = 0;
    if (ledState == 0)
    {
        ledState = 1;
        LL_GPIO_ResetOutputPin(MCU_IO_DEBUG_LED3_GPIO_Port, MCU_IO_DEBUG_LED3_Pin);
    }
    else
    {
        LL_GPIO_SetOutputPin(MCU_IO_DEBUG_LED3_GPIO_Port, MCU_IO_DEBUG_LED3_Pin);
        ledState = 0;
    }
}





/*************************************************
 *                  Helper Define                *
 *************************************************/

#define NUM_SCHEDULER_TASKS (sizeof(schedulerTasks) / sizeof(schedulerTasks[0]))

void SchedulerTasks_Create(void)
{
    for (uint8_t i = 0; i < NUM_SCHEDULER_TASKS; i++)
    {
        SCH_TASK_CreateTask(&schedulerTasks[i].taskHandle, &schedulerTasks[i].taskProperty);
        if (schedulerTasks[i].taskProperty.taskType == SCH_TASK_ASYNC)
        {
            asyncTaskHandle = schedulerTasks[i].taskHandle;
        }
    }
}

SCH_Status SchedulerTasks_RegisterTask(SCH_TASK_HANDLE *pHandle, SCH_TaskPropertyTypedef *pTaskProperty)
{
    SCH_Status status = SCH_TASK_CreateTask(pHandle, pTaskProperty);
    if (status == SCH_DONE && pTaskProperty->taskType == SCH_TASK_ASYNC)
    {
        asyncTaskHandle = *pHandle;
    }
    return status;
}
