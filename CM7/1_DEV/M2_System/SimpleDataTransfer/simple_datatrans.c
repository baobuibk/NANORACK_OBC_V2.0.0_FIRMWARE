/************************************************
 *  @file     : simple_datatrans.c
 *  @date     : Jul 23, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    [-]
 ************************************************/

#include "simple_datatrans.h"
#include "logger/bscript_logger.h"
#include "DateTime/date_time.h"
#include "modfsp.h"
#include "port/bscript_port.h"

#include "FileSystem/filesystem.h"

#include "SPI_MasterOfEXP/spi_master.h"
#include "SPI_SlaveOfCM4/spi_slave.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "MIN_Process/min_process.h"

#define MODFSP_TYPE_CHUNK_CMD   		0x21
#define MODFSP_TYPE_CURRENT_CMD        	0x22
#define MODFSP_TYPE_LOG_CMD        		0x23


#define DATAREADY_PIN_PORT       		EXPOUT_OBCIN_DATAREADY_GPIO_Port
#define DATAREADY_PIN            		EXPOUT_OBCIN_DATAREADY_Pin

#define READDONE_PIN_PORT				OBCOUT_EXPIN_READDONE_GPIO_Port
#define READDONE_PIN					OBCOUT_EXPIN_READDONE_Pin


__attribute__((section(".ram_data_bridge"), aligned(4))) uint8_t g_simple_ram_d3_buffer[DATA_CHUNK_SIZE];

extern MODFSP_Data_t cm4_protocol;

static uint32_t g_successful_transfers = 0;
static uint32_t g_failed_transfers = 0;
static uint32_t g_crc_errors = 0;

static volatile bool g_master_ack_received = false;
static volatile bool g_master_ack_success = false;

/*************************************************
 *              PRIVATE FUNCTIONS                *
 *************************************************/
static SimpleTransferResult_t ExecuteSingleTransfer(SimpleDataType_t data_type,
                                                   uint16_t chunk_id,
                                                   const char* base_filename);

/*************************************************
 *              PUBLIC FUNCTIONS                 *
 *************************************************/

/**
 * @brief Initialize Simple Data Transfer system
 */
Std_ReturnType SimpleDataTransfer_Init(void)
{
    // Clear RAM D3 buffer
    memset(g_simple_ram_d3_buffer, 0, DATA_CHUNK_SIZE);
    LL_GPIO_ResetOutputPin(READDONE_PIN_PORT, READDONE_PIN);
    // Reset statistics
    g_successful_transfers = 0;
    g_failed_transfers = 0;
    g_crc_errors = 0;

    // Reset acknowledgment flags
    g_master_ack_received = false;
    g_master_ack_success = false;

    BScript_Log("[SimpleDataTransfer] Initialized successfully");
    return E_OK;
}

/**
 * @brief Transfer single chunk of data (BLOCKING)
 */
SimpleTransferResult_t SimpleDataTransfer_ExecuteTransfer(SimpleDataType_t data_type,
														 uint16_t chunk_id,
                                                         const char* base_filename)
{
    if (!base_filename || data_type > DATA_TYPE_LOG) {
        return SIMPLE_TRANSFER_ERROR_INVALID_PARAMS;
    }

    BScript_Log("[SimpleDataTransfer] Starting %s transfer - chunk %u",
                SimpleDataTransfer_GetTypeString(data_type), chunk_id);


    SimpleTransferResult_t result = ExecuteSingleTransfer(data_type, chunk_id, base_filename);


    if (result == SIMPLE_TRANSFER_SUCCESS) {
        g_successful_transfers++;
        BScript_Log("[SimpleDataTransfer] Transfer completed successfully");
    } else {
        g_failed_transfers++;
        BScript_Log("[SimpleDataTransfer] Transfer failed: %s",
                    SimpleDataTransfer_GetResultString(result));
    }

    return result;
}

/**
 * @brief Transfer multiple chunks (BLOCKING)
 */
SimpleTransferResult_t SimpleDataTransfer_ExecuteMultipleChunks(SimpleDataType_t data_type,
                                                               uint16_t total_chunks,
                                                               const char* base_filename)
{
    if (!base_filename || data_type > DATA_TYPE_LOG || total_chunks == 0 || total_chunks > 256) {
        return SIMPLE_TRANSFER_ERROR_INVALID_PARAMS;
    }

    // For CURRENT and LOG, force single chunk
    if (data_type != DATA_TYPE_CHUNK) {
        total_chunks = 1;
    }

    BScript_Log("[SimpleDataTransfer] Starting %s transfer - %u chunks",
                SimpleDataTransfer_GetTypeString(data_type), total_chunks);

    for (uint16_t chunk_id = 0; chunk_id < total_chunks; chunk_id++) {
        SimpleTransferResult_t result = ExecuteSingleTransfer(data_type, chunk_id, base_filename);

        if (result != SIMPLE_TRANSFER_SUCCESS) {
            BScript_Log("[SimpleDataTransfer] Failed at chunk %u/%u: %s",
                       chunk_id + 1, total_chunks, SimpleDataTransfer_GetResultString(result));
            g_failed_transfers++;
            return result;
        }

        g_successful_transfers++;
        BScript_Log("[SimpleDataTransfer] Chunk %u/%u completed", chunk_id + 1, total_chunks);

        // Small delay between chunks to prevent overwhelming the system
        if (total_chunks > 1 && chunk_id < (total_chunks - 1)) {
            BScript_Delayms(200);
        }
    }

    BScript_Log("[SimpleDataTransfer] All %u chunks transferred successfully", total_chunks);
    return SIMPLE_TRANSFER_SUCCESS;
}

/**
 * @brief Execute single transfer with full sequence
 */
static SimpleTransferResult_t ExecuteSingleTransfer(SimpleDataType_t data_type,
                                                   uint16_t chunk_id,
                                                   const char* base_filename)
{
    char filename[64];
    uint16_t expected_crc = 0;
    uint16_t calculated_crc = 0;

    // Generate filename
    SimpleDataTransfer_GenerateFilename(data_type, base_filename, chunk_id, filename);

    // CRC retry loop (skip for LOG data)
    uint8_t retry_count = 0;
    bool crc_check_needed = (data_type != DATA_TYPE_LOG);

    do {
        // Step 0: Send get chunk
    	BScript_Log("[SimpleDataTransfer] Step 0: Try to sending command %u...", chunk_id);
        uint8_t chunk_response[1];
        uint8_t chunk_response_len = 0;
		switch (data_type) {
		  case DATA_TYPE_CHUNK: {
				if (!MIN_Send_GET_CHUNK_CMD_WithData((uint8_t)chunk_id, chunk_response, &chunk_response_len)) {
					BScript_Log("[SimpleDataTransfer] Failed to Send get-chunk from slave %u", chunk_id);
					retry_count++;
					continue;
				}
			break;
		  }

		  case DATA_TYPE_CURRENT: {
				if (!MIN_Send_GET_LASER_CURRENT_CRC_CMD_WithData(chunk_response, &chunk_response_len)) {
					BScript_Log("[SimpleDataTransfer] Failed to Send get-current from slave");
					retry_count++;
					continue;
				}
			break;
		  }

		  case DATA_TYPE_LOG: {
				if (!MIN_Send_GET_LOG_CMD_WithData(chunk_response, &chunk_response_len)) {
					BScript_Log("[SimpleDataTransfer] Failed to Send get-log from slave");
					retry_count++;
					continue;
				}
			break;
		  }

		  default:
			break;
		}

        // Step 1: Wait for DATAREADY to become inactive
        BScript_Log("[SimpleDataTransfer] Step 1: Waiting for Data ready...");
        if (SimpleDataTransfer_WaitDataReady(TRANSFER_TIMEOUT_MS) != E_OK) {
            BScript_Log("[SimpleDataTransfer] DATAREADY timeout");
            return SIMPLE_TRANSFER_ERROR_DATAREADY_TIMEOUT;
        }

        // Step 2: Read data from slave via SPI DMA
        BScript_Log("[SimpleDataTransfer] Step 2: Reading %u bytes from slave (attempt %u)...",
                   DATA_CHUNK_SIZE, retry_count + 1);

        if (SPI_MasterDevice_ReadDMA((uint32_t)g_simple_ram_d3_buffer, DATA_CHUNK_SIZE) != E_OK) {
            BScript_Log("[SimpleDataTransfer] SPI read failed");
            return SIMPLE_TRANSFER_ERROR_SPI_READ_FAILED;
        }

        // Step 2.5: Handshake: Signal READDONE high and wait for DATAREADY to go low
        BScript_Log("[SimpleDataTransfer] Step 2.5: Signaling READDONE and waiting for DATAREADY low...");
        LL_GPIO_SetOutputPin(READDONE_PIN_PORT, READDONE_PIN);

        TickType_t handshake_start_time = xTaskGetTickCount();
        TickType_t handshake_timeout_ticks = pdMS_TO_TICKS(TRANSFER_TIMEOUT_MS);

        while (LL_GPIO_IsInputPinSet(DATAREADY_PIN_PORT, DATAREADY_PIN)) {
            if ((xTaskGetTickCount() - handshake_start_time) >= handshake_timeout_ticks) {
                BScript_Log("[SimpleDataTransfer] Timeout waiting for DATAREADY to go low after READDONE");
                LL_GPIO_ResetOutputPin(READDONE_PIN_PORT, READDONE_PIN); // Reset pin on timeout
                return SIMPLE_TRANSFER_ERROR_DATAREADY_TIMEOUT; // Re-use timeout error
            }
            BScript_Delayms(1); // Yield to other tasks
        }

        // Handshake complete, reset READDONE pin for the next transfer cycle
        LL_GPIO_ResetOutputPin(READDONE_PIN_PORT, READDONE_PIN);
        BScript_Log("[SimpleDataTransfer] Handshake complete. DATAREADY is low.");

        // Step 3: CRC verification (skip for LOG data)
        if (crc_check_needed) {
            BScript_Log("[SimpleDataTransfer] Step 3: Verifying CRC...");

            // Get CRC from slave
            uint8_t crc_response[2];
            uint8_t crc_response_len = 0;

            if(data_type == DATA_TYPE_CHUNK){
				if (!MIN_Send_GET_CHUNK_CRC_CMD_WithData(chunk_id, crc_response, &crc_response_len)) {
					BScript_Log("[SimpleDataTransfer] Failed to get CRC from slave");
					retry_count++;
					continue;
				}
            }else if (data_type == DATA_TYPE_CURRENT){
				if (!MIN_Send_GET_LASER_CURRENT_CRC_CMD_WithData(crc_response, &crc_response_len)) {
					BScript_Log("[SimpleDataTransfer] Failed to get CRC from slave");
					retry_count++;
					continue;
				}
            }

            if (crc_response_len != 2 ) {
                BScript_Log("[SimpleDataTransfer] Invalid CRC response length");
                retry_count++;
                continue;
            }

            expected_crc = (crc_response[0] << 8) | crc_response[1];
            calculated_crc = SimpleDataTransfer_CalculateCRC16(g_simple_ram_d3_buffer, DATA_CHUNK_SIZE);

            if (expected_crc != calculated_crc) {
                BScript_Log("[SimpleDataTransfer] CRC mismatch - Expected: 0x%04X, Calculated: 0x%04X",
                           expected_crc, calculated_crc);
                g_crc_errors++;
                retry_count++;

                if (retry_count < MAX_CRC_RETRY_COUNT) {
                    BScript_Log("[SimpleDataTransfer] Retrying CRC check (%u/%u)...",
                               retry_count, MAX_CRC_RETRY_COUNT);
                    BScript_Delayms(100);
                    continue;
                }

                BScript_Log("[SimpleDataTransfer] Max CRC retries exceeded");
                return SIMPLE_TRANSFER_ERROR_CRC_MISMATCH;
            }

            BScript_Log("[SimpleDataTransfer] CRC verified: 0x%04X", calculated_crc);
        }

        break; // Success or no CRC check needed

    } while (retry_count < MAX_CRC_RETRY_COUNT);

    // Step 4: Save data to file
    Std_ReturnType fs_result = FS_Write_Direct(filename, g_simple_ram_d3_buffer, DATA_CHUNK_SIZE);
    if (fs_result == E_BUSY) {
        // Filesystem busy, wait a bit and retry once
        BScript_Log("[SimpleDataTransfer] Filesystem busy, retrying...");
        BScript_Delayms(10);
        fs_result = FS_Write_Direct(filename, g_simple_ram_d3_buffer, DATA_CHUNK_SIZE);
    }

    if (fs_result != E_OK) {
        BScript_Log("[SimpleDataTransfer] File save failed: %s",
                   (fs_result == E_BUSY) ? "Filesystem busy" : "Write error");
        return SIMPLE_TRANSFER_ERROR_FILE_SAVE_FAILED;
    }

    uint8_t master_retry = 0;
    do {
		// Step 5: Send data to master via SPI slave
		BScript_Log("[SimpleDataTransfer] Step 5: Sending data to master...");
		if (SPI_SlaveDevice_ResetDMA((uint32_t)g_simple_ram_d3_buffer, DATA_CHUNK_SIZE) != E_OK) {
			BScript_Log("[SimpleDataTransfer] SPI slave setup failed");
			return SIMPLE_TRANSFER_ERROR_MASTER_TRIGGER_FAILED;
		}

		// Step 6: Trigger master to read data
		BScript_Log("[SimpleDataTransfer] Step 6: Triggering master...");
		g_master_ack_received = false;
		g_master_ack_success = false;

		switch (data_type) {
		  case DATA_TYPE_CHUNK: {
			uint8_t trigger_data[4];
			trigger_data[0] = (uint8_t)((chunk_id >> 8) & 0xFF);
			trigger_data[1] = (uint8_t)(chunk_id & 0xFF);
			uint16_t crc_for_master = crc_check_needed ? calculated_crc : 0;
			trigger_data[2] = (uint8_t)((crc_for_master >> 8) & 0xFF);
			trigger_data[3] = (uint8_t)(crc_for_master & 0xFF);
			if (!MODFSP_Send(&cm4_protocol, MODFSP_TYPE_CHUNK_CMD,
							 trigger_data, sizeof(trigger_data))) {
			  BScript_Log("[SimpleDataTransfer] Master trigger failed");
			  return SIMPLE_TRANSFER_ERROR_MASTER_TRIGGER_FAILED;
			}
			break;
		  }

		  case DATA_TYPE_CURRENT: {
			uint8_t trigger_data[2];
			uint16_t crc_for_master = crc_check_needed ? calculated_crc : 0;
			trigger_data[0] = (uint8_t)((crc_for_master >> 8) & 0xFF);
			trigger_data[1] = (uint8_t)(crc_for_master & 0xFF);
			if (!MODFSP_Send(&cm4_protocol, MODFSP_TYPE_CURRENT_CMD,
							 trigger_data, sizeof(trigger_data))) {
			  BScript_Log("[SimpleDataTransfer] Master trigger failed");
			  return SIMPLE_TRANSFER_ERROR_MASTER_TRIGGER_FAILED;
			}
			break;
		  }

		  case DATA_TYPE_LOG: {
			uint8_t trigger_data[1];
			trigger_data[0] = 0;
			if (!MODFSP_Send(&cm4_protocol, MODFSP_TYPE_LOG_CMD, trigger_data, sizeof(trigger_data))) {
			  BScript_Log("[SimpleDataTransfer] Master trigger failed");
			  return SIMPLE_TRANSFER_ERROR_MASTER_TRIGGER_FAILED;
			}
			break;
		  }

		  default:
			break;
		}

		// Step 7: Wait for master acknowledgment
		BScript_Log("[SimpleDataTransfer] Step 7: Waiting for master acknowledgment...");

		TickType_t start_time = xTaskGetTickCount();
		TickType_t timeout_ticks = pdMS_TO_TICKS(TRANSFER_TIMEOUT_MS);

		while (!g_master_ack_received) {
			if ((xTaskGetTickCount() - start_time) >= timeout_ticks) {
				BScript_Log("[SimpleDataTransfer] Master acknowledgment timeout");
				return SIMPLE_TRANSFER_ERROR_MASTER_ACK_TIMEOUT;
			}
			BScript_Delayms(10);
		}

        if (g_master_ack_success) {
            BScript_Log("[SimpleDataTransfer] Transfer completed successfully");
            return SIMPLE_TRANSFER_SUCCESS;
        } else {
            BScript_Log("[SimpleDataTransfer] Master NACK received, retrying (%u/%u)",
                        master_retry+1, MAX_MASTER_RETRIES);
            master_retry++;
            BScript_Delayms(100);
            g_master_ack_received = false;
        }

    } while (master_retry < MAX_MASTER_RETRIES);

    BScript_Log("[SimpleDataTransfer] Max master retries exceeded");
    return SIMPLE_TRANSFER_ERROR_MASTER_ACK_TIMEOUT;
}

SimpleTransferResult_t SimpleDataTransfer_ExecuteLogTransfer(const char* base_filename, uint8_t* log_data_ptr, uint32_t log_data_size)
{
    char filename[64];

    if (log_data_ptr == NULL) {
        BScript_Log("[SimpleDataTransfer] Error: Log data pointer is null.");
        return SIMPLE_TRANSFER_ERROR_INVALID_PARAMS;
    }

    SimpleDataTransfer_GenerateFilename(DATA_TYPE_LOG, base_filename,0 , filename);

    BScript_Log("[SimpleDataTransfer] Saving log to %s...", filename);
    Std_ReturnType fs_result = FS_Write_Direct(filename, log_data_ptr, log_data_size);
    if (fs_result == E_BUSY) {
        BScript_Log("[SimpleDataTransfer] Filesystem busy, retrying...");
        BScript_Delayms(10);
        fs_result = FS_Write_Direct(filename, log_data_ptr, log_data_size);
    }

    if (fs_result != E_OK) {
        BScript_Log("[SimpleDataTransfer] File save failed: %s",
                      (fs_result == E_BUSY) ? "Filesystem busy" : "Write error");
        return SIMPLE_TRANSFER_ERROR_FILE_SAVE_FAILED;
    }

    uint8_t master_retry = 0;
    do {
        BScript_Log("[SimpleDataTransfer] Step 5: Sending data to master...");
        if (SPI_SlaveDevice_ResetDMA((uint32_t)log_data_ptr, log_data_size) != E_OK) {
            BScript_Log("[SimpleDataTransfer] SPI slave setup failed");
            return SIMPLE_TRANSFER_ERROR_MASTER_TRIGGER_FAILED;
        }

        BScript_Log("[SimpleDataTransfer] Step 6: Triggering master...");
        g_master_ack_received = false;
        g_master_ack_success = false;

        uint8_t trigger_data[1] = {0xFF};
        if (!MODFSP_Send(&cm4_protocol, MODFSP_TYPE_LOG_CMD, trigger_data, sizeof(trigger_data))) {
            BScript_Log("[SimpleDataTransfer] Master trigger failed");
            return SIMPLE_TRANSFER_ERROR_MASTER_TRIGGER_FAILED;
        }

        BScript_Log("[SimpleDataTransfer] Step 7: Waiting for master acknowledgment...");
        TickType_t start_time = xTaskGetTickCount();
        TickType_t timeout_ticks = pdMS_TO_TICKS(TRANSFER_TIMEOUT_MS);

        while (!g_master_ack_received) {
            if ((xTaskGetTickCount() - start_time) >= timeout_ticks) {
                BScript_Log("[SimpleDataTransfer] Master acknowledgment timeout");
                return SIMPLE_TRANSFER_ERROR_MASTER_ACK_TIMEOUT;
            }
            BScript_Delayms(10);
        }

        if (g_master_ack_success) {
            BScript_Log("[SimpleDataTransfer] Transfer completed successfully for %s", filename);
            return SIMPLE_TRANSFER_SUCCESS;
        } else {
            BScript_Log("[SimpleDataTransfer] Master NACK received, retrying (%u/%u)",
                          master_retry + 1, MAX_MASTER_RETRIES);
            master_retry++;
            BScript_Delayms(100);
            g_master_ack_received = false;
        }

    } while (master_retry < MAX_MASTER_RETRIES);

    BScript_Log("[SimpleDataTransfer] Max master retries exceeded for %s", filename);
    return SIMPLE_TRANSFER_ERROR_MASTER_ACK_TIMEOUT;
}

/*************************************************
 *              UTILITY FUNCTIONS                *
 *************************************************/

/**
 * @brief Check if DATAREADY pin is active
 */
bool SimpleDataTransfer_IsDataReady(void)
{
    return GPIO_IsInHigh(DATAREADY_PIN_PORT, DATAREADY_PIN);
}

/**
 * @brief Wait for DATAREADY to become ready
 */
Std_ReturnType SimpleDataTransfer_WaitDataReady(uint32_t timeout_ms)
{
    TickType_t start_time = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while (SimpleDataTransfer_IsDataReady()) {
        if ((xTaskGetTickCount() - start_time) >= timeout_ticks) {
            return E_TIMEOUT;
        }
        BScript_Delayms(10);
    }

    return E_OK;
}

/**
 * @brief Calculate CRC16 XMODEM
 */
uint16_t SimpleDataTransfer_CalculateCRC16(const uint8_t* data, uint32_t size)
{
    uint16_t crc = 0x0000;
    const uint16_t polynomial = 0x1021; // CRC16 XMODEM

    for (uint32_t i = 0; i < size; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief Generate filename based on type and chunk
 */
void SimpleDataTransfer_GenerateFilename(SimpleDataType_t data_type,
                                        const char* base_filename,
                                        uint16_t chunk_id,
                                        char* output_filename)
{
    const char* extension = "";

    switch (data_type) {
        case DATA_TYPE_CHUNK:
            extension = ".dat";
            snprintf(output_filename, 64, "%s_chunk%u%s", base_filename, chunk_id, extension);
            break;
        case DATA_TYPE_CURRENT:
            extension = ".cdat";
            snprintf(output_filename, 64, "%s%s", base_filename, extension);
            break;
        case DATA_TYPE_LOG:
            extension = ".log";
            snprintf(output_filename, 64, "%s%s", base_filename, extension);
            break;
    }
}

/**
 * @brief Get string representation of transfer result
 */
const char* SimpleDataTransfer_GetResultString(SimpleTransferResult_t result)
{
    switch (result) {
        case SIMPLE_TRANSFER_SUCCESS: return "SUCCESS";
        case SIMPLE_TRANSFER_ERROR_DATAREADY_TIMEOUT: return "MINBUSY_TIMEOUT";
        case SIMPLE_TRANSFER_ERROR_SPI_READ_FAILED: return "SPI_READ_FAILED";
        case SIMPLE_TRANSFER_ERROR_CRC_MISMATCH: return "CRC_MISMATCH";
        case SIMPLE_TRANSFER_ERROR_FILE_SAVE_FAILED: return "FILE_SAVE_FAILED";
        case SIMPLE_TRANSFER_ERROR_MASTER_TRIGGER_FAILED: return "MASTER_TRIGGER_FAILED";
        case SIMPLE_TRANSFER_ERROR_MASTER_ACK_TIMEOUT: return "MASTER_ACK_TIMEOUT";
        case SIMPLE_TRANSFER_ERROR_INVALID_PARAMS: return "INVALID_PARAMS";
        default: return "UNKNOWN_ERROR";
    }
}

/**
 * @brief Get string representation of data type
 */
const char* SimpleDataTransfer_GetTypeString(SimpleDataType_t type)
{
    switch (type) {
        case DATA_TYPE_CHUNK: return "CHUNK";
        case DATA_TYPE_CURRENT: return "CURRENT";
        case DATA_TYPE_LOG: return "LOG";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Get transfer statistics
 */
void SimpleDataTransfer_GetStatistics(uint32_t* successful_transfers,
                                     uint32_t* failed_transfers,
                                     uint32_t* crc_errors)
{
    if (successful_transfers) *successful_transfers = g_successful_transfers;
    if (failed_transfers) *failed_transfers = g_failed_transfers;
    if (crc_errors) *crc_errors = g_crc_errors;
}

/**
 * @brief Reset transfer statistics
 */
void SimpleDataTransfer_ResetStatistics(void)
{
    g_successful_transfers = 0;
    g_failed_transfers = 0;
    g_crc_errors = 0;
}

/**
 * @brief Print current statistics
 */
void SimpleDataTransfer_PrintStatistics(void)
{
    BScript_Log("=== SIMPLE DATA TRANSFER STATISTICS ===");
    BScript_Log("Successful Transfers: %u", g_successful_transfers);
    BScript_Log("Failed Transfers: %u", g_failed_transfers);
    BScript_Log("CRC Errors: %u", g_crc_errors);
}

void SimpleDataTransfer_HandleMasterAck(uint8_t frame_id, const uint8_t* data, uint32_t length)
{
    switch (frame_id) {
        case MODFSP_MASTER_ACK: // ACK - Transfer successful
            BScript_Log("[SimpleDataTransfer] Received master ACK");
            g_master_ack_received = true;
            g_master_ack_success = true;
            break;

        case MODFSP_MASTER_NAK: // NACK - CRC error
            BScript_Log("[SimpleDataTransfer] Received master NACK - CRC error");
            g_master_ack_received = true;
            g_master_ack_success = false;
            break;

        default:
            BScript_Log("[SimpleDataTransfer] Unknown master response: 0x%02X", frame_id);
            break;
    }
}

