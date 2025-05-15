/*
 * min_command.h
 *
 *  Created on: Apr 22, 2025
 *      Author: CAO HIEU
 */

#ifndef M5_THIRDPARTY_MIN_R01_MIN_APP_MIN_COMMAND_H_
#define M5_THIRDPARTY_MIN_R01_MIN_APP_MIN_COMMAND_H_

#include <stdint.h>
#include "min_app.h"

// =================================================================
// Command IDs (Maximum ID: 63)
// =================================================================
#define READ_TEMP_CMD         0x01  ///< Request temperature reading
#define TEMP_RESPONSE         0x02  ///< Response with temperature data
#define CONTROL_TEMP_CMD      0x03  ///< Control temperature setting
#define CONTROL_TEMP_ACK      0x04  ///< Acknowledgment for control command
#define HEARTBEAT_CMD         0x05  ///< Heartbeat request
#define HEARTBEAT_ACK         0x06  ///< Heartbeat acknowledgment
#define GET_STATUS_CMD        0x07  ///< Request system status
#define STATUS_RESPONSE       0x08  ///< Response with status data
#define RESET_CMD             0x09  ///< Reset system
#define PING_CMD              0x0A  ///< Ping request
#define PONG_CMD              0x0B  ///< Pong response
#define DUMMY_CMD_1           0x0C  ///< Dummy command 1
#define DUMMY_CMD_2           0x0D  ///< Dummy command 2
#define CUSTOM_CMD_1          0x0E  ///< Custom command 1
#define CUSTOM_CMD_1_ACK      0x0F  ///< Acknowledgment for custom command 1
#define CUSTOM_CMD_2          0x10  ///< Custom command 2
#define CUSTOM_CMD_2_ACK      0x11  ///< Acknowledgment for custom command 2

#define COLLECT_DATA_CMD	  0x12
#define PRE_CHUNK_CMD		  0x13
#define PRE_CHUNK_ACK		  0x14
#define PRE_DATA_CMD		  0x15
#define PRE_DATA_ACK		  0x16
#define SAMPLERATE_SET_CMD 	  0x17
#define SAMPLERATE_GET_CMD    0x18
#define SAMPLERATE_GET_ACK	  0x19
#define COLLECT_PACKAGE_CMD   0x1A
#define COLLECT_PACKAGE_ACK   0x1B

#define OVER				  0x3B
#define GOT_IT				  0x3C
#define WRONG				  0x3D
#define FAIL				  0x3E
#define	DONE				  0x3F

/**
 * @brief Command handler function type.
 * @param ctx Pointer to the MIN context.
 * @param payload Pointer to the received payload data.
 * @param len Length of the payload.
 */
typedef void (*MIN_CommandHandler)(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len);

/**
 * @brief Structure to map command IDs to their handlers.
 */
typedef struct {
    uint8_t id;                    ///< Command ID
    MIN_CommandHandler handler;    ///< Handler function for the command
} MIN_Command_t;

/**
 * @brief Gets the command table.
 * @return Pointer to the command table.
 */
const MIN_Command_t *MIN_GetCommandTable(void);

/**
 * @brief Gets the size of the command table.
 * @return Number of entries in the command table.
 */
int MIN_GetCommandTableSize(void);


#endif /* M5_THIRDPARTY_MIN_R01_MIN_APP_MIN_COMMAND_H_ */
