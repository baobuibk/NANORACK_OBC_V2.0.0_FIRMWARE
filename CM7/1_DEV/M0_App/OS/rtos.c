/*
 * rtos.c
 *
 *  Created on: Feb 21, 2025
 *      Author: CAO HIEU
 */

#include "rtos.h"
#include "rtos_tasks.h"
#include "FreeRTOS.h"
#include "task.h"

/*--------------------Star RTOS--------------*/
void OBC_RTOS_Start(void)
{
	OBC_RootGrowUp();
}

/*--------------------RTOS Task List--------------*/

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   while(1){}
   //NVIC_SystemReset
}


