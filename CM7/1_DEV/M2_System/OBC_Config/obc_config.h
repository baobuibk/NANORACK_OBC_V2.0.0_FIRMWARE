/************************************************
 *  @file     : obc_config.h
 *  @date     : Aug 4, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    [-]
 ************************************************/

#ifndef M2_SYSTEM_OBC_CONFIG_OBC_CONFIG_H_
#define M2_SYSTEM_OBC_CONFIG_OBC_CONFIG_H_

#include "stdint.h"
#include "stdbool.h"

typedef enum {
    CLI_MODE_DISABLE_LOG = 0,				// 0
    CLI_MODE_ENABLE_LOG_ONLY_DMESG,			// 1
	CLI_MODE_ENABLE_LOG_ONLY_BSCRIPT,		// 2
	CLI_MODE_ENABLE_LOG_ALL					// 3
} OBC_CliMode_t;

_Bool OBC_Config_Init(void);
OBC_CliMode_t OBC_Config_GetCliMode(void);
uint8_t OBC_Config_GetCleanThreshold(void) ;
void OBC_Config_GetCleanDate(uint8_t *dd, uint8_t *mm, uint8_t *yy) ;
_Bool OBC_Config_SetCliMode(OBC_CliMode_t mode);
_Bool OBC_Config_SetCleanThreshold(uint8_t interval);
_Bool OBC_Config_SetCleanDate(uint8_t dd, uint8_t mm, uint8_t yy);

#endif /* M2_SYSTEM_OBC_CONFIG_OBC_CONFIG_H_ */
