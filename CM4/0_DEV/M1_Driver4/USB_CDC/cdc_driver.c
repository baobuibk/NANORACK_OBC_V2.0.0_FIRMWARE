/*
 * usb_driver.c
 *
 *  Created on: Mar 1, 2025
 *      Author: CAO HIEU
 */

#include "cdc_driver.h"
#include "RingBuffer/ring_buffer.h"

/*************************************************
 *          Static RingBuffer Define             *
 *************************************************/
//RX
static RingBufElement cdcRxBuffer[CDC_RX_RING_BUFFER_SIZE];
static s_RingBufferType cdcRxRingBuffer;
static TaskHandle_t xCDCRxTaskHandle = NULL;
//TX
static RingBufElement cdcTxBuffer[CDC_TX_RING_BUFFER_SIZE];
static s_RingBufferType cdcTxRingBuffer;
static TaskHandle_t xCDCTxTaskHandle = NULL;

static TaskHandle_t xUSBCheckTaskHandle = NULL;

static _Bool CDC_TX_RingBuffer_Put(uint8_t data);
static _Bool CDC_RX_RingBuffer_Put(uint8_t data);

/*************************************************
 *                 Function Define               *
 *************************************************/

void CDC_RingBuffer_Init(void)
{
	RingBuffer_Create(&cdcTxRingBuffer, 3, "CDC_TX", cdcTxBuffer, CDC_TX_RING_BUFFER_SIZE);
    RingBuffer_Create(&cdcRxRingBuffer, 4, "CDC_RX", cdcRxBuffer, CDC_RX_RING_BUFFER_SIZE);
}
/*************************************************/
static _Bool CDC_TX_RingBuffer_Put(uint8_t data)
{
    return RingBuffer_Put(&cdcTxRingBuffer, data);
}

static _Bool CDC_RX_RingBuffer_Put(uint8_t data)
{
    return RingBuffer_Put(&cdcRxRingBuffer, data);
}
/*************************************************/
uint32_t CDC_TX_RingBuffer_PutBuffer(const uint8_t* data, uint32_t len)
{
    uint32_t count = 0;
    for(uint32_t i = 0; i < len; i++)
    {
        if(CDC_TX_RingBuffer_Put(data[i]))
        {
            count++;
        }
    }
    return count;
}

uint32_t CDC_RX_RingBuffer_PutBuffer(uint8_t* data, uint32_t len)
{
    uint32_t count = 0;
    for(uint32_t i = 0; i < len; i++)
    {
        if(CDC_RX_RingBuffer_Put(data[i]))
        {
            count++;
        }
    }
    return count;
}
/*************************************************/
_Bool CDC_TX_RingBuffer_Get(uint8_t* data)
{
    return RingBuffer_Get(&cdcTxRingBuffer, data);
}

_Bool CDC_RX_RingBuffer_Get(uint8_t* data)
{
    return RingBuffer_Get(&cdcRxRingBuffer, data);
}
/*************************************************/
void CDC_TX_RegisterTask(TaskHandle_t taskHandle)
{
    xCDCTxTaskHandle = taskHandle;
}

void CDC_RX_RegisterTask(TaskHandle_t taskHandle)
{
	xCDCRxTaskHandle = taskHandle;
}

void USB_Check_RegisterTask(TaskHandle_t taskHandle)
{
    xUSBCheckTaskHandle = taskHandle;
}
/*************************************************/
void CDC_TX_Notify_toTrans(void)
{
    if(xCDCTxTaskHandle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(xCDCTxTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void CDC_RX_Notify_toRecv(void)
{
    if(xCDCRxTaskHandle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(xCDCRxTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void USB_Check_Notify(void)
{
    if (xUSBCheckTaskHandle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(xUSBCheckTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/*************************************************
 *                    API Define                 *
 *************************************************/

BaseType_t CDC_SendString(const char *pStr, unsigned short len)
{
    CDC_TX_RingBuffer_PutBuffer((const uint8_t*)pStr, len);
    CDC_TX_Notify_toTrans();
    return pdTRUE;
}

BaseType_t CDC_SendChar(char c)
{
    CDC_TX_RingBuffer_Put((uint8_t)c);
    CDC_TX_Notify_toTrans();
    return pdTRUE;
}
