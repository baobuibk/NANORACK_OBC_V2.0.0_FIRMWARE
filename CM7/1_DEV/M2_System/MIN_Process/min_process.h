/*
 * min_process.h
 *
 *  Created on: Apr 18, 2025
 *      Author: CAO HIEU
 */

#ifndef M2_SYSTEM_MIN_PROCESS_MIN_PROCESS_H_
#define M2_SYSTEM_MIN_PROCESS_MIN_PROCESS_H_

void MIN_Process_Init(void);
void MIN_Processing(void);

void MIN_ResponseCallback(uint8_t min_id, const uint8_t *payload, uint8_t len);
// =================================================================
// Command Sending Functions
// =================================================================
void Min_Send_READ_TEMP_CMD(void);
void Min_Send_CONTROL_TEMP_CMD(uint8_t temperature);
void Min_Send_COLLECT_DATA(uint32_t sample);
void Min_Send_HEARTBEAT_CMD(void);
void Min_Send_GET_STATUS_CMD(void);
void Min_Send_RESET_CMD(void);
void Min_Send_PING_CMD(void);
void Min_Send_DUMMY_CMD_1(void);
void Min_Send_DUMMY_CMD_2(void);
void Min_Send_CUSTOM_CMD_1(void);
void Min_Send_CUSTOM_CMD_2(void);

void Min_Send_PRE_DATA(uint16_t chunk_size);
void Min_Send_PRE_CHUNK(uint8_t chunk);
void Min_Send_SAMPLERATE_SET(uint32_t sample_rate);
void Min_Send_SAMPLERATE_GET(void);
void Min_Send_COLLECT_PACKAGE(void);

#endif /* M2_SYSTEM_MIN_PROCESS_MIN_PROCESS_H_ */
