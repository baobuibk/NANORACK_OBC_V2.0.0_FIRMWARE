/*
 * quadspi.h
 *
 *  Created on: Apr 17, 2025
 *      Author: CAO HIEU
 */

#ifndef M1_DRIVER_QSPI_QUADSPI_H_
#define M1_DRIVER_QSPI_QUADSPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern QSPI_HandleTypeDef hqspi;

/* USER CODE BEGIN Private defines */

uint8_t CSP_QUADSPI_Init(void);
uint8_t CSP_QSPI_EraseSector(uint32_t EraseStartAddress, uint32_t EraseEndAddress);
uint8_t CSP_QSPI_Write(uint8_t* buffer, uint32_t address, uint32_t buffer_size);
uint8_t CSP_QSPI_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
uint8_t CSP_QSPI_Erase_Block(uint32_t BlockAddress);
uint8_t CSP_QSPI_EnableMemoryMappedMode(void);
uint8_t CSP_QSPI_Erase_Chip (void);


#ifdef __cplusplus
}
#endif

#endif /* M1_DRIVER_QSPI_QUADSPI_H_ */
