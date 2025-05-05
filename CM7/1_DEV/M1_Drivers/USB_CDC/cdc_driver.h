/*
 * usb_driver.c
 *
 *  Created on: Mar 1, 2025
 *      Author: CAO HIEU
 */

#ifndef M1_DRIVERS_USB_USB_DRIVER_C_
#define M1_DRIVERS_USB_USB_DRIVER_C_

#include "stdint.h"
#include "stdbool.h"
#include "FreeRTOS.h"
#include "task.h"

#define CDC_RX_RING_BUFFER_SIZE 1024
#define CDC_TX_RING_BUFFER_SIZE 2048

void CDC_RingBuffer_Init(void);

// Đưa một ký tự vào buffer TX

// Đưa một buffer vào buffer TX
uint32_t CDC_TX_RingBuffer_PutBuffer(const uint8_t* data, uint32_t len);

// Đưa một ký tự vào buffer RX

// Đưa một buffer vào buffer RX
uint32_t CDC_RX_RingBuffer_PutBuffer(uint8_t* data, uint32_t len);

// Lấy một ký tự từ buffer TX
_Bool CDC_TX_RingBuffer_Get(uint8_t* data);
// Lấy một ký tự từ buffer RX
_Bool CDC_RX_RingBuffer_Get(uint8_t* data);

// Đăng ký Task xử lý dữ liệu TX
void CDC_TX_RegisterTask(TaskHandle_t taskHandle);
// Đăng ký Task xử lý dữ liệu RX
void CDC_RX_RegisterTask(TaskHandle_t taskHandle);
void USB_Check_RegisterTask(TaskHandle_t taskHandle);
// Gửi thông báo tới Task TX để xử lý dữ liệu
void CDC_TX_Notify_toTrans(void);
// Gửi thông báo tới Task RX để xử lý dữ liệu
void CDC_RX_Notify_toRecv(void);

void USB_Check_Notify(void);

// API để gửi dữ liệu
BaseType_t CDC_SendString(const char *pStr, unsigned short len);
BaseType_t CDC_SendChar(char c);


#endif /* M1_DRIVERS_USB_USB_DRIVER_C_ */
