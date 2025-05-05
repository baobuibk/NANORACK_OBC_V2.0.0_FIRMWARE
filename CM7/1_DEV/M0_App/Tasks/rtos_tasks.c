/*
 * rtos_task.c
 *
 *  Created on: Feb 21, 2025
 *      Author: CAO HIEU
 */

/************************************************************************************************************/
/* @CaoHieu:------------------------------------------------------------------------------------------------*/
/* - Todo: Add Return function  for Task, (not NULL anymore)               								v   */
/* - Todo: Add UART Debug									                  							v	*/
/* - Todo: Syslog																						v	*/
/* - Todo: Change Ringbuffer_Uart -> Ringbuffer Utils (Init Size/Maxsize) + -> init to Driver UART  	v	*/
/* - Todo: UART-> UART DMA using FREERTOS QUEUE								  							v	*/
/* - Todo: New Commandline								  												v	*/
/* - Todo: Add Protocol Link board to board								 									*/
/* - Todo: Build TCP FreeRTOS							  													*/
/* - Todo: MMC DRIVER - FATFS							  												v	*/
/* - Todo: Bootloader to EXP							  												?	*/
/* - Todo: Bootloader for OBC							  												?	*/
/* - Todo: Tiny USB + RNDIS 							  												?	*/
/* - Todo: Thread Safe Printf							  												v	*/
/* - Todo: Reason why rtc time jump ramdomly			  												v	*/
/************************************************************************************************************/
#include "stdio.h"
#include "main.h"

#include "rtos_tasks.h"
#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "management.h"
#include "common_state.h"
#include "utils.h"
#include "uart_driver_dma.h"
#include "cdc_driver.h"
#include "usbd_cdc_if.h"
/*************************************************
 *                     TEST                      *
 *************************************************/
#include "system.h"
#include "mode.h"

#include "SysLog_Queue/syslog_queue.h"
#include "DateTime/date_time.h"
#include "CLI_Terminal/CLI_Src/embedded_cli.h"
#include "CLI_Terminal/CLI_Setup/cli_setup.h"
#include "SPI_FRAM/fram_spi.h"
#include "IO_ExWD-TPL5010/wd_tpl5010.h"
#include "FileSystem/filesystem.h"
#include "SPI_SlaveOfCM4/spi_slave.h"
#include "SPI_MasterOfEXP/spi_master.h"
#include "min_proto.h"
#include "MIN_Process/min_process.h"
//#include "inter_cpu_comm.h"
#include "CLI_Terminal/CLI_Auth/simple_shield.h"
/*************************************************
 *                    Header                     *
 *************************************************/

ShieldInstance_t auth_uart;

ShieldInstance_t auth_usb;

static void writeChar_auth_USB(char c) {
	CDC_SendChar(c);
}

static void writeChar_auth_UART(char c) {
    uint8_t c_to_send = c;
    UART_Driver_Write(UART_DEBUG, c_to_send);
}


Std_ReturnType OBC_AppInit(void);
#define MIN_STACK_SIZE	configMINIMAL_STACK_SIZE
#define ROOT_PRIORITY   5

#define CREATE_TASK(task_func, task_name, stack, param, priority, handle) \
    if (xTaskCreate(task_func, task_name, stack, param, priority, handle) != pdPASS) { \
        return E_ERROR; \
    }

/*************************************************
 *               TASK DEFINE                     *
 *************************************************/
void vSoft_RTC_Task(void *pvParameters);
void UART_DEBUG_DMA_RX_Task(void *pvParameters);
void UART_EXP_DMA_RX_Task(void *pvParameters);
void MIN_Process_Task(void *pvParameters);
void CLI_Handle_Task(void *pvParameters);
void vTask1_handler(void *pvParameters);
void vTask2_handler(void *pvParameters);
void vTask3_handler(void *pvParameters);
void vUART_bufferTest(void *pvParameters);
void vUSBCheck_Task(void *argument);
void CDC_TX_Task(void *pvParameters);
void CDC_RX_Task(void *pvParameters);
void vWatchdogTask(void *pvParameters);

/*************************************************
 *               	Root Task	                 *
 *************************************************/
void OBC_RootTask(void *pvParameters);

void OBC_RootTask(void *pvParameters)
{
    SYSLOG_NOTICE("Root task started");
    if (OBC_AppInit() != E_OK)
    {
        SYSLOG_ERROR_POLL("Application initialization failed");
        system_status.program_state = PROGRAM_STATE_ERROR;
    }
    else
    {
        SYSLOG_NOTICE("App Init successfully");
    }
    vTaskDelete(NULL);
}

void OBC_RootGrowUp(void)
{
	if (xTaskCreate(OBC_RootTask, "OBC_RootTask", MIN_STACK_SIZE * 10, NULL, ROOT_PRIORITY, NULL) != pdPASS)
	{
		SYSLOG_ERROR_POLL("Cannot Start Root-Task!!!");
	}
	vTaskStartScheduler();
}
/*************************************************
 *               	TASK INIT	                 *
 *************************************************/

Std_ReturnType OBC_AppInit(void)
{

//	Std_ReturnType ret = E_OK;

	if (Mgmt_SystemInitStepZero() != E_OK)
	{
	    SYSLOG_ERROR_POLL("System init step zero failed");
	    return E_ERROR;
	}
	if (Mgmt_SystemInitStepOne() != E_OK) {
		SYSLOG_ERROR_POLL("System init step one failed");
	    return E_ERROR;
	}

	if (Mgmt_SystemInitStepTwo() != E_OK) {
		SYSLOG_ERROR_POLL("System init step two failed");
	   return E_ERROR;
	}

	if (Mgmt_SystemInitFinal() != E_OK) {
		SYSLOG_ERROR_POLL("System init final failed");
	   return E_ERROR;
	}

	FS_Init();

	SPI_SlaveDevice_Init();

	if (SPI_MasterDevice_Init(SPI6, SPI6_EXP_CS_GPIO_Port, SPI6_EXP_CS_Pin) != E_OK) {
		SYSLOG_ERROR_POLL("SPI Master Init Failed");
	   return E_ERROR;
	}

	MIN_Process_Init();

    CREATE_TASK(FS_Gatekeeper_Task, 	"FS_Gatekeeper", 	MIN_STACK_SIZE * 20, 	NULL, 									2, NULL);

    CREATE_TASK(MIN_Process_Task, 		"MIN_Process", 		MIN_STACK_SIZE * 20, 	NULL, 									1, NULL);

    CREATE_TASK(SysLog_Task, 			"SysLog_Task", 		MIN_STACK_SIZE * 10, 	NULL, 									1, NULL);	// Syslog Queue from syslog_queue.c

    CREATE_TASK(vSoft_RTC_Task, 		"Soft_RTC_Task", 	MIN_STACK_SIZE * 2, 	NULL, 									1, NULL);

    CREATE_TASK(UART_DEBUG_DMA_RX_Task, "DEBUG_RX_Task", 	MIN_STACK_SIZE * 10, 	(void*)UART_DMA_Driver_Get(UART_DEBUG), 1, NULL);

    CREATE_TASK(UART_EXP_DMA_RX_Task, 	"EXP_RX_Task",	 	MIN_STACK_SIZE * 10, 	(void*)UART_DMA_Driver_Get(UART_EXP), 	1, NULL);

    CREATE_TASK(CLI_Handle_Task, 		"CLI_Handle_Task", 	MIN_STACK_SIZE * 10, 	NULL, 									1, NULL);

    CREATE_TASK(vTask1_handler, 		"vTask1", 			MIN_STACK_SIZE, 		NULL, 									1, NULL);

    CREATE_TASK(vTask2_handler, 		"vTask2", 			MIN_STACK_SIZE, 		NULL, 									1, NULL);

    CREATE_TASK(vTask3_handler, 		"vTask3", 			MIN_STACK_SIZE, 		NULL, 									1, NULL);

    CREATE_TASK(vUSBCheck_Task, 		"vUSBCheck_Task", 	MIN_STACK_SIZE * 2, 	NULL, 									1, NULL);

    CREATE_TASK(CDC_TX_Task, 			"USB_CDC_TX_Task", 	MIN_STACK_SIZE * 5, 	NULL, 									1, NULL);

    CREATE_TASK(CDC_RX_Task, 			"USB_CDC_RX_Task", 	MIN_STACK_SIZE * 5, 	NULL,									1, NULL);

    CREATE_TASK(vWatchdogTask, 			"Watchdog_Task", 	MIN_STACK_SIZE, 		NULL, 									1, NULL);

    vTaskDelay(pdMS_TO_TICKS(1));

	Shield_Init(&auth_uart, writeChar_auth_UART);

	Shield_Init(&auth_usb, writeChar_auth_USB);

    return E_OK;
}

/*************************************************
 *               TASK LIST                       *
 *************************************************/
void vSoft_RTC_Task(void *pvParameters)
{
    static uint32_t countingSyncTime = 0;
    while(1)
    {
        Utils_SoftTime_Update();
        countingSyncTime++;
        if(countingSyncTime > 7200)
        {
            countingSyncTime = 0;
            if(Utils_SoftTime_Sync() == E_OK)
            {
                UART_Driver_SendString(UART_DEBUG, "\r\n[Sync Time!]\r\n");
            }
        }
        vTaskDelay(1000);
    }
}

void CLI_Handle_Task(void *pvParameters)
{
	while (1)
	{
    	ShieldAuthState_t auth_state;
    	auth_state = Shield_GetState(&auth_uart);
    	if(auth_state == AUTH_ADMIN || auth_state == AUTH_USER){
        	if(auth_uart.initreset == 1){
                embeddedCliPrint(getUartCm4CliPointer(), "");
                auth_uart.initreset = 0;
        	}
//			embeddedCliProcess(getUsbCdcCliPointer());
			embeddedCliProcess(getUartCm4CliPointer());
    	}

    	auth_state = Shield_GetState(&auth_usb);
    	if(auth_state == AUTH_ADMIN || auth_state == AUTH_USER){
        	if(auth_usb.initreset == 1){
                embeddedCliPrint(getUsbCdcCliPointer(), "");
                auth_usb.initreset = 0;
        	}
			embeddedCliProcess(getUsbCdcCliPointer());
//			embeddedCliProcess(getUartCm4CliPointer());
    	}

		vTaskDelay(500);
	}
}

void UART_DEBUG_DMA_RX_Task(void *pvParameters)
{
    UART_DMA_Driver_t *driver = (UART_DMA_Driver_t *)pvParameters;
    for (;;)
    {
        if (xSemaphoreTake(driver->rxSemaphore, portMAX_DELAY) == pdTRUE)
        {
            int c;
            while ((c = UART_DMA_Driver_Read(driver->uart)) != -1)
            {
                ForwardMode_t mode = ForwardMode_Get();
                if (mode == FORWARD_MODE_UART) {
                    // Forward mode: USART2 (rx) - UART7 (tx)
//                    UART_Driver_Write(UART7, (uint8_t)c);
                    UART_Driver_Write(UART_EXP, (uint8_t)c);

                    if (ForwardMode_ProcessReceivedByte((uint8_t)c)) {
                        embeddedCliPrint(getUartCm4CliPointer(), "Forward mode disabled due to 10 consecutive '\\'.");
                    }
                } else if (mode == FORWARD_MODE_LISTEN_CM4) {
                    if (ForwardMode_ProcessReceivedByte((uint8_t)c)) {
                        embeddedCliPrint(getUartCm4CliPointer(), "Forward mode disabled due to 10 consecutive '\\'.");
                    }
                    embeddedCliReceiveChar(getUartCm4CliPointer(), (char)c);
                    embeddedCliProcess(getUartCm4CliPointer());
                } else {
                    // Mode NORMAL: CLI
                	ShieldAuthState_t auth_state = Shield_GetState(&auth_uart);
                	if(auth_state == AUTH_ADMIN || auth_state == AUTH_USER){
                		Shield_ResetTimer(&auth_uart);
                        embeddedCliReceiveChar(getUartCm4CliPointer(), (char)c);
                        embeddedCliProcess(getUartCm4CliPointer());
                	}else{
                		Shield_ReceiveChar(&auth_uart, (char)c);
                	}
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void UART_EXP_DMA_RX_Task(void *pvParameters)
{
    UART_DMA_Driver_t *driver = (UART_DMA_Driver_t *)pvParameters;
    for (;;)
    {
        if (xSemaphoreTake(driver->rxSemaphore, 0) == pdTRUE)
        {
            int c;
            while ((c = UART_DMA_Driver_Read(driver->uart)) != -1)
            {
                ForwardMode_t mode = ForwardMode_Get();
                if (mode == FORWARD_MODE_UART) {
                    // Forward mode (CM4): UART7 RX -> UART_DEBUG
                    UART_Driver_Write(UART_DEBUG, (uint8_t)c);
//                    embeddedCliReceiveChar(getUartCm4CliPointer(), (char)c);
                } else if (mode == FORWARD_MODE_USB) {
                    // Forward mode (USB): UART7 -> to CDC
                    CDC_SendChar((char)c);
                } else if (mode == FORWARD_MODE_LISTEN_CM4) {
                    // Listen mode (CM4): UART7 RX -> UART_DEBUG
                    UART_Driver_Write(UART_DEBUG, (uint8_t)c);
                } else if (mode == FORWARD_MODE_LISTEN_USB) {
                    // Listen mode (USB):UART7 RX -> CDC
                    CDC_SendChar((char)c);
                } else {
                    // NORMAL mode: Default processing (e.g., logging or ignoring data)
                }
//                if (ForwardMode_ProcessReceivedByte((uint8_t)c)) {
//                    if (mode == FORWARD_MODE_UART || mode == FORWARD_MODE_LISTEN_CM4) {
//                        embeddedCliPrint(getUartCm4CliPointer(), "Listen/Forward mode disabled due to 10 consecutive '\\'.");
//                    } else if (mode == FORWARD_MODE_USB || mode == FORWARD_MODE_LISTEN_USB) {
//                        embeddedCliPrint(getUsbCdcCliPointer(), "Listen/Forward mode disabled due to 10 consecutive '\\'.");
//                    }
//                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


void CDC_RX_Task(void *pvParameters)
{
    TaskHandle_t xThisTask = xTaskGetCurrentTaskHandle();
    CDC_RX_RegisterTask(xThisTask);

    uint8_t rxChar;
    for(;;)
    {
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10)) > 0)
        {
            while (CDC_RX_RingBuffer_Get(&rxChar))
            {
                ForwardMode_t mode = ForwardMode_Get();
                if (mode == FORWARD_MODE_USB) {
                    // Forward CDC rx -> UART7 tx
                    UART_Driver_Write(UART_EXP, rxChar);
                    if (ForwardMode_ProcessReceivedByte(rxChar)) {
                        embeddedCliPrint(getUsbCdcCliPointer(), "Forward mode disabled due to 10 consecutive '\\'.");
                    }
                } else if ( mode == FORWARD_MODE_LISTEN_USB){
                    // Mode NORMAL: CLI
                    if (ForwardMode_ProcessReceivedByte(rxChar)) {
                        embeddedCliPrint(getUsbCdcCliPointer(), "Forward mode disabled due to 10 consecutive '\\'.");
                    }
                    embeddedCliReceiveChar(getUsbCdcCliPointer(), (char)rxChar);
                    embeddedCliProcess(getUsbCdcCliPointer());
                } else {
                    // Mode NORMAL: CLI
                	ShieldAuthState_t auth_state = Shield_GetState(&auth_usb);
                	if(auth_state == AUTH_ADMIN || auth_state == AUTH_USER){
                		Shield_ResetTimer(&auth_usb);
                        embeddedCliReceiveChar(getUsbCdcCliPointer(), (char)rxChar);
                        embeddedCliProcess(getUsbCdcCliPointer());
                	}else{
                		Shield_ReceiveChar(&auth_usb, (char)rxChar);
                	}
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void CDC_TX_Task(void *pvParameters)
{
    TaskHandle_t xThisTask = xTaskGetCurrentTaskHandle();
    CDC_TX_RegisterTask(xThisTask);

    uint8_t txChar;
    uint8_t txBuffer[1];
    for(;;)
    {
        if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)) > 0)
        {
            while(CDC_TX_RingBuffer_Get(&txChar))
            {
                txBuffer[0] = txChar;
                uint8_t timeout = 0;
                while(CDC_Transmit_FS(txBuffer, 1) != USBD_OK)
                {
                    vTaskDelay(pdMS_TO_TICKS(1));
                    if (++timeout > 1000)
                    {
                        break;
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void vUSBCheck_Task(void *argument)
{
    TaskHandle_t xThisTask = xTaskGetCurrentTaskHandle();
    USB_Check_RegisterTask(xThisTask);

    for (;;)
    {
        if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
        {
            if(CDC_ComPort_isOpen())
            {
                LogQueue_NOTICE("USB_Open");
            }
        }
    }
}

void MIN_Process_Task(void *pvParameters)
{
	while(1){
		MIN_Processing();
	    vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void vTask1_handler(void *pvParameters)
{
	while (1)
	{
		GPIO_SetLow(LED0_Port, LED0);
		vTaskDelay(1000);

		GPIO_SetHigh(LED0_Port, LED0);
		vTaskDelay(1000);
	}
}

void vTask2_handler(void *pvParameters)
{
	while (1)
	{
		GPIO_SetHigh(LED1_Port, LED1);
		vTaskDelay(1000);
		GPIO_SetLow(LED1_Port, LED1);
		vTaskDelay(1000);
	}
}

void vTask3_handler(void *pvParameters)
{
	while (1)
	{
		vTaskDelay(1000);
	}
}

void vWatchdogTask(void *pvParameters)
{
    for (;;) {
        Watchdog_Device_Update();

        if(Watchdog_Device_GetState() == WATCHDOG_STATE_HIGH)
        {
            vTaskDelay(pdMS_TO_TICKS(HIGH_PERIOD));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(LOW_PERIOD));
        }
    }
}


/*************************************************
 *                    END                        *
 *************************************************/


