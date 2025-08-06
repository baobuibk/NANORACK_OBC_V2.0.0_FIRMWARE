/************************************************
 *  @file     : obc_config.c
 *  @date     : Aug 4, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    [-]
 ************************************************/

#include "obc_config.h"
#include "SPI_FRAM/fram_spi.h"
#include "logger/bscript_logger.h"

static OBC_CliMode_t currentCliMode = CLI_MODE_DISABLE_LOG;
static volatile uint8_t cleanThresholdPercent = 80;
static volatile uint8_t cleanDate = 1;
static volatile uint8_t cleanMonth = 1;
static volatile uint8_t cleanYear = 25;

#define FRAM_TOTAL_SIZE         (256 * 1024)    // 256KB total
#define FRAM_SPACE_SECTION_SIZE (17 * 1024)    // 17KB for scripts
#define FRAM_CONFIG_BASE_ADDR   (FRAM_TOTAL_SIZE - FRAM_SPACE_SECTION_SIZE)

#define FRAM_OBC_CLI_MODE_ADDR   		(FRAM_CONFIG_BASE_ADDR + 0)
#define FRAM_OBC_CLEAN_THRESHOLD_ADDR   (FRAM_CONFIG_BASE_ADDR + 1)
#define FRAM_OBC_DATE_CLEAN_DD_ADDR    	(FRAM_CONFIG_BASE_ADDR + 2)
#define FRAM_OBC_DATE_CLEAN_MM_ADDR    	(FRAM_CONFIG_BASE_ADDR + 3)
#define FRAM_OBC_DATE_CLEAN_YY_ADDR    	(FRAM_CONFIG_BASE_ADDR + 4)

#define FRAM_CONFIG_SIZE				1 + 1 + 1 + 1 + 1

_Bool OBC_Config_Init(void)
{
    currentCliMode = CLI_MODE_DISABLE_LOG;
    cleanThresholdPercent = 80;
    cleanDate = 1;
    cleanMonth = 1;
    cleanYear = 25;

    FRAM_SPI_HandleTypeDef* fram_handle  = FRAM_SPI_GetHandle();
    uint8_t currentConfigOBC[FRAM_CONFIG_SIZE];
    Std_ReturnType status = FRAM_SPI_ReadMem(fram_handle, FRAM_OBC_CLI_MODE_ADDR, currentConfigOBC, FRAM_CONFIG_SIZE);
    if (status != E_OK) {
        BScript_Log("[OBC-Config] Error: Failed to read from FRAM (status: %d)", status);
        return false;
    }

    currentCliMode = currentConfigOBC[0];

    if(currentConfigOBC[1] == 0 || currentConfigOBC[2] == 0 || currentConfigOBC[3] == 0 || currentConfigOBC[4] == 0){
    	cleanThresholdPercent = 80;
        cleanDate = 1;
        cleanMonth = 1;
        cleanYear = 25;
    }else{
    	cleanThresholdPercent = currentConfigOBC[1];
        cleanDate = currentConfigOBC[2];
        cleanMonth = currentConfigOBC[3];
        cleanYear = currentConfigOBC[4];
    }

    return true;
}

OBC_CliMode_t OBC_Config_GetCliMode(void) {
    return currentCliMode;
}

uint8_t OBC_Config_GetCleanThreshold(void) {
    return cleanThresholdPercent;
}

void OBC_Config_GetCleanDate(uint8_t *dd, uint8_t *mm, uint8_t *yy) {
    if (dd) *dd = cleanDate;
    if (mm) *mm = cleanMonth;
    if (yy) *yy = cleanYear;
}

_Bool OBC_Config_SetCliMode(OBC_CliMode_t mode) {
    currentCliMode = mode;
    FRAM_SPI_HandleTypeDef *fram = FRAM_SPI_GetHandle();
    return (FRAM_SPI_WriteMem(fram, FRAM_OBC_CLI_MODE_ADDR, (uint8_t*)&mode, 1) == E_OK);
}

_Bool OBC_Config_SetCleanThreshold(uint8_t threshold) {
	cleanThresholdPercent = threshold;
    FRAM_SPI_HandleTypeDef *fram = FRAM_SPI_GetHandle();
    return (FRAM_SPI_WriteMem(fram, FRAM_OBC_CLEAN_THRESHOLD_ADDR, &threshold, 1) == E_OK);
}

_Bool OBC_Config_SetCleanDate(uint8_t dd, uint8_t mm, uint8_t yy) {
    cleanDate = dd;
    cleanMonth = mm;
    cleanYear = yy;
    FRAM_SPI_HandleTypeDef *fram = FRAM_SPI_GetHandle();
    uint8_t buf[3] = {dd, mm, yy};
    return (FRAM_SPI_WriteMem(fram, FRAM_OBC_DATE_CLEAN_DD_ADDR, buf, 3) == E_OK);
}


