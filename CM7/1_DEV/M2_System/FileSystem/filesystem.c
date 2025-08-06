/*
 * filesystem.c - Simplified Version
 *
 *  Created on: Mar 2, 2025
 *      Author: CAO HIEU
 */
#include "ff.h"
#include "mmc_diskio.h"
#include "filesystem.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"

#include "board.h"
#include "utils.h"
#include "SysLog/syslog.h"
#include "DateTime/date_time.h"

#include "OBC_Config/obc_config.h"

#define LOG_RETENTION_DAYS 3
#define DATE_TIME_FILENAME "date_time.txt"
#define SECONDS_PER_DAY (24 * 60 * 60)

#include "reinit.h"
#include "lwl.h"
/*************************************************
 *                 FATFS Variable                *
 *************************************************/
__attribute__((section(".dma_buffer"))) FATFS MMC1FatFs;  /* File system object for SD card logical drive */
__attribute__((section(".dma_buffer"))) FIL MyFile1;     /* File object */
char MMC1Path[4]; /* SD card logical drive path */

/*************************************************
 *               Simplified Mutex               *
 *************************************************/
SemaphoreHandle_t fsMutex;

/*************************************************
 *              Physical Status SDMMC            *
 *************************************************/
SDFS_StateTypedef SDFS_State = SDFS_READY;

SDFS_StateTypedef SDFS_GetStatus(void){
	return SDFS_State;
}


uint8_t SD_Lockin(void)
{
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        SYSLOG_ERROR("Failed to acquire FS mutex before lockin");
        return 0;
    }

    if (SDFS_State == SDFS_READY) {
        xSemaphoreGive(fsMutex);
        return 1;
    }

    GPIO_SetHigh(SD_Detect_Port, SD_Detect);
    vTaskDelay(pdMS_TO_TICKS(20));
    GPIO_SetHigh(SD_InOut_Port, SD_InOut);
    vTaskDelay(200);

    SDMMC1_Init();

    Std_ReturnType ret = Link_SDFS_Driver();
    if (ret == E_OK) {
        SDFS_State = SDFS_READY;
        SYSLOG_DEBUG_POLL("[SDFS] Filesystem mounted successfully");
    } else {
        SDFS_State = SDFS_ERROR;
        SYSLOG_ERROR_POLL("[SDFS] Failed to mount filesystem");
        xSemaphoreGive(fsMutex);
        return 0;
    }

    s_DateTime now;
    Utils_GetRTC(&now);
    LWL_Log(OBC_STM32_LOCKIN, LWL_1(now.day), LWL_1(now.month), LWL_1(now.year), LWL_1(now.hour), LWL_1(now.minute), LWL_1(now.second));


    xSemaphoreGive(fsMutex);
    return 1;
}

uint8_t SD_Release(void)
{
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        SYSLOG_ERROR("Failed to acquire FS mutex before release");
        return 0;
    }

    vTaskDelay(200);
    f_sync(NULL);

    vTaskDelay(pdMS_TO_TICKS(20));

    f_mount(NULL, MMC1Path, 0);
    FATFS_UnLinkDriver(MMC1Path);

    vTaskDelay(pdMS_TO_TICKS(20));

    SDMMC1_DeInit();

    vTaskDelay(100);

    GPIO_SetLow(SD_InOut_Port, SD_InOut);
    vTaskDelay(pdMS_TO_TICKS(20));
    GPIO_SetLow(SD_Detect_Port, SD_Detect);

    SDFS_State = SDFS_RELEASE;

    s_DateTime now;
    Utils_GetRTC(&now);
    LWL_Log(OBC_STM32_RELEASE, LWL_1(now.day), LWL_1(now.month), LWL_1(now.year), LWL_1(now.hour), LWL_1(now.minute), LWL_1(now.second));


    xSemaphoreGive(fsMutex);

    return 1;
}


/*************************************************
 *                 Simplified Init              *
 *************************************************/
void FS_Init(void) {
    fsMutex = xSemaphoreCreateMutex();
    if (fsMutex == NULL) {
        SYSLOG_ERROR_POLL("Failed to create filesystem mutex");
        return;
    }
//    SYSLOG_INFO_POLL("Filesystem initialized successfully");
}

/*************************************************
 *              Simplified Write API             *
 *************************************************/
Std_ReturnType FS_Write_Direct(const char* filename, uint8_t* buffer, uint32_t size) {
    if (!filename || !buffer || size == 0) {
        return E_ERROR;
    }

    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return E_BUSY;
    }

    FIL file;
    FRESULT res;
    UINT byteswritten;
    Std_ReturnType result = E_ERROR;

    res = f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE);
    if (res == FR_OK) {
        res = f_write(&file, buffer, size, &byteswritten);

        if (res == FR_OK && byteswritten == size) {
            res = f_sync(&file);
        }

        FRESULT close_res = f_close(&file);

        if (res == FR_OK && byteswritten == size && close_res == FR_OK) {
            result = E_OK;
        }
    }

    xSemaphoreGive(fsMutex);
    return result;
}

/*************************************************
 *              Asynchronous Write API           *
 *************************************************/
typedef struct {
    char filename[64];
    uint8_t* buffer;
    uint32_t size;
    TaskHandle_t requester;
    volatile Std_ReturnType* result_ptr;
} FS_AsyncRequest_t;

static QueueHandle_t fsAsyncQueue = NULL;
static StaticQueue_t fsAsyncQueueStruct;
static uint8_t fsAsyncQueueBuffer[5 * sizeof(FS_AsyncRequest_t)]; // 5 slots max

void FS_Async_Task(void *pvParameters) {
    FS_AsyncRequest_t request;

    for (;;) {
        if (xQueueReceive(fsAsyncQueue, &request, portMAX_DELAY) == pdTRUE) {
            Std_ReturnType result = FS_Write_Direct(request.filename, request.buffer, request.size);

            // Notify requester with result
            if (request.result_ptr) {
                *request.result_ptr = result;
            }

            if (request.requester) {
                xTaskNotify(request.requester, result, eSetValueWithOverwrite);
            }
        }
    }
}

Std_ReturnType FS_Write_Async(const char* filename, uint8_t* buffer, uint32_t size,
                              volatile Std_ReturnType* result_ptr) {
    if (!filename || !buffer || size == 0) {
        return E_ERROR;
    }

    // Initialize async queue if not done
    if (fsAsyncQueue == NULL) {
        fsAsyncQueue = xQueueCreateStatic(5, sizeof(FS_AsyncRequest_t),
                                         fsAsyncQueueBuffer, &fsAsyncQueueStruct);
        if (fsAsyncQueue == NULL) {
            return E_ERROR;
        }
    }

    FS_AsyncRequest_t request = {
        .buffer = buffer,
        .size = size,
        .requester = xTaskGetCurrentTaskHandle(),
        .result_ptr = result_ptr
    };

    strncpy(request.filename, filename, sizeof(request.filename) - 1);
    request.filename[sizeof(request.filename) - 1] = '\0';

    if (xQueueSend(fsAsyncQueue, &request, pdMS_TO_TICKS(100)) != pdPASS) {
        return E_BUSY; // Queue full
    }

    return E_PENDING; // Operation queued successfully
}

/*************************************************
 *              Legacy Write API                 *
 *************************************************/
// Backward compatibility - maps to direct write
Std_ReturnType FS_Request_Write(const char* filename, uint8_t* buffer, uint32_t size) {
    return FS_Write_Direct(filename, buffer, size);
}

/*************************************************
 *                   Low layer API               *
 *************************************************/
Std_ReturnType Link_SDFS_Driver(void) {
#if defined(DUAL_MMC) || defined(ONLY_MMC1)
    const Diskio_drvTypeDef *mmc1_driver = MMC1_GetDriver();
    if (FATFS_LinkDriver(mmc1_driver, MMC1Path) == 0) {
        int ret1 = f_mount(&MMC1FatFs, (TCHAR const*)MMC1Path, 1);
        if (ret1 != FR_OK) {
            return E_ERROR;
        }
    }
#endif
#if defined(DUAL_MMC) || defined(ONLY_MMC2)
    const Diskio_drvTypeDef *mmc2_driver = MMC2_GetDriver();
    if (FATFS_LinkDriver(mmc2_driver, MMC2Path) == 0) {
        int ret2 = f_mount(&MMC2FatFs, (TCHAR const*)MMC2Path, 1);
        if (ret2 != FR_OK) {
            return E_ERROR;
        }
    }
#endif
    return E_OK;
}

_Bool FS_GetUsage(FS_UsageInfo_t *info) {
    FATFS *fs;
    DWORD free_clusters;
    FRESULT res;

    if (!info) return false;

    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }

    res = f_getfree(MMC1Path, &free_clusters, &fs);
    if (res != FR_OK) {
        xSemaphoreGive(fsMutex);
        return false;
    }


    uint64_t total_clusters = (uint64_t)(fs->n_fatent - 2);
    uint64_t cluster_size = (uint64_t)fs->csize * 512;


    uint64_t total_bytes = total_clusters * cluster_size;
    uint64_t free_bytes = (uint64_t)free_clusters * cluster_size;

    info->total_kb = (uint32_t)(total_bytes / 1024);
    info->free_kb = (uint32_t)(free_bytes / 1024);

    if (total_bytes > 0) {
        uint64_t used_bytes = total_bytes - free_bytes;
        info->used_percent = (uint8_t)((used_bytes * 100) / total_bytes);
    } else {
        info->used_percent = 0;
    }

    xSemaphoreGive(fsMutex);
    return true;
}
/*************************************************
 *              CLI Commands (Unchanged)         *
 *************************************************/
int Cat_SDFS(EmbeddedCli *cli, const char *filename) {
    FIL file;
    FRESULT res;
    ALIGN_32BYTES(uint8_t rtext[96]);
    UINT bytesread;
    char buffer[128];

    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        embeddedCliPrint(cli, "Failed to acquire FS mutex");
        return -1;
    }

    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        snprintf(buffer, sizeof(buffer), "Failed to open file %s: %d", filename, res);
        embeddedCliPrint(cli, buffer);
        xSemaphoreGive(fsMutex);
        return -1;
    }

    embeddedCliPrint(cli, "");
    do {
        memset(rtext, 0, sizeof(rtext));
        res = f_read(&file, rtext, sizeof(rtext) - 1, &bytesread);
        if (res != FR_OK) {
            snprintf(buffer, sizeof(buffer), "Failed to read file %s: %d", filename, res);
            embeddedCliPrint(cli, buffer);
            f_close(&file);
            xSemaphoreGive(fsMutex);
            return -1;
        }
        rtext[bytesread] = '\0';
        embeddedCliPrint(cli, (char *)rtext);
    } while (bytesread > 0);

    embeddedCliPrint(cli, "");
    f_close(&file);
    xSemaphoreGive(fsMutex);
    return 0;
}

int Vim_SDFS(EmbeddedCli *cli, const char *filename, const char *content) {
    FIL file;
    FRESULT res;
    char buffer[128];
    UINT byteswritten;

    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        embeddedCliPrint(cli, "Failed to acquire FS mutex");
        return -1;
    }

    res = f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE);
    if (res != FR_OK) {
        snprintf(buffer, sizeof(buffer), "Failed to open or create file %s: %d", filename, res);
        embeddedCliPrint(cli, buffer);
        xSemaphoreGive(fsMutex);
        return -1;
    }

    if (content && strlen(content) > 0) {
        res = f_write(&file, content, strlen(content), &byteswritten);
        if (res != FR_OK || byteswritten != strlen(content)) {
            snprintf(buffer, sizeof(buffer), "Failed to write to file %s: %d", filename, res);
            embeddedCliPrint(cli, buffer);
            f_close(&file);
            xSemaphoreGive(fsMutex);
            return -1;
        }
    }

    f_close(&file);
    xSemaphoreGive(fsMutex);
    snprintf(buffer, sizeof(buffer), "Successfully wrote to %s", filename);
    embeddedCliPrint(cli, buffer);
    return 0;
}

void FS_ListFiles_path(EmbeddedCli *cli) {
    FRESULT res;
    DIR dir;
    FILINFO fno;
    char *path = MMC1Path;
    char buffer[384];

    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        embeddedCliPrint(cli, "Failed to acquire FS mutex");
        return;
    }

    snprintf(buffer, sizeof(buffer), "Listing files in %s...", path);
    embeddedCliPrint(cli, buffer);

    res = f_opendir(&dir, path);
    if (res == FR_OK) {
        while (1) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;
            if (fno.fattrib & AM_DIR) {
                snprintf(buffer, sizeof(buffer), "  [DIR]  %s", fno.fname);
                embeddedCliPrint(cli, buffer);
            } else {
                snprintf(buffer, sizeof(buffer), "  [FILE] %s  %lu bytes",
                         fno.fname, (unsigned long)fno.fsize);
                embeddedCliPrint(cli, buffer);
            }
        }
        f_closedir(&dir);
    } else {
        snprintf(buffer, sizeof(buffer), "Failed to open directory %s: %d", path, res);
        embeddedCliPrint(cli, buffer);
    }
    xSemaphoreGive(fsMutex);
}

Std_ReturnType FS_FormatFull(void) {
    FRESULT res;
    BYTE work[FF_MAX_SS];

    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        SYSLOG_ERROR("[FS] Failed to acquire FS mutex for full format");
        return E_BUSY;
    }

    f_mount(NULL, MMC1Path, 0);

    MKFS_PARM opt = {
        .fmt = FM_FAT32,
        .n_fat = 1,
        .align = 0,
        .n_root = 0,
        .au_size = 0
    };

    res = f_mkfs(MMC1Path, &opt, work, sizeof(work));

    if (res == FR_OK) {
        res = f_mount(&MMC1FatFs, (TCHAR const*)MMC1Path, 1);
        if (res == FR_OK) {
            SYSLOG_INFO("[FS] Full format completed successfully.");
            xSemaphoreGive(fsMutex);
            return E_OK;
        } else {
            SYSLOG_ERROR("[FS] Failed to remount after full format.");
        }
    } else {
        SYSLOG_ERROR("[FS] f_mkfs failed during full format ");
    }

    f_mount(&MMC1FatFs, (TCHAR const*)MMC1Path, 1);
    xSemaphoreGive(fsMutex);
    return E_ERROR;
}


Std_ReturnType CreateOrUpdateDateFile(void) {
    uint32_t current_epoch;
    char epoch_str[16];

    // Get current epoch using existing function
    current_epoch = Utils_GetEpoch();
    snprintf(epoch_str, sizeof(epoch_str), "%lu", current_epoch);

    return FS_Write_Direct(DATE_TIME_FILENAME, (uint8_t*)epoch_str, strlen(epoch_str));
}

_Bool NeedCleanup(void) {
    FS_UsageInfo_t usage;

    if (!FS_GetUsage(&usage)) {
        SYSLOG_ERROR("NeedCleanup: Failed to get FS usage");
        return false;
    }

    uint8_t threshold = OBC_Config_GetCleanThreshold();

    return (usage.used_percent > threshold);
}


Std_ReturnType PerformCleanup(void) {
    FRESULT res;
    BYTE work[FF_MAX_SS]; // Work area for f_mkfs

    SYSLOG_INFO_POLL("[LogFetching] Starting filesystem cleanup...");

    // Take filesystem mutex
    if (xSemaphoreTake(fsMutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        SYSLOG_ERROR_POLL("[LogFetching] Failed to acquire FS mutex for cleanup");
        return E_BUSY;
    }

    // Unmount first
    f_mount(NULL, MMC1Path, 0);

    // Prepare MKFS_PARM structure for new FatFS API
    MKFS_PARM mkfs_opt = {
        .fmt = FM_FAT32,        // Format type
        .n_fat = 1,             // Number of FATs (0 = default)
        .align = 0,             // Data area alignment (0 = default)
        .n_root = 0,            // Number of root directory entries (0 = default)
        .au_size = 0            // Allocation unit size (0 = default)
    };

    s_DateTime now;
    Utils_GetRTC(&now);
    LWL_Log(OBC_STM32_CLEAN, LWL_1(now.day), LWL_1(now.month), LWL_1(now.year), LWL_1(now.hour), LWL_1(now.minute), LWL_1(now.second));

    // Format the entire filesystem - this deletes everything
    res = f_mkfs(MMC1Path, &mkfs_opt, work, sizeof(work));

    if (res == FR_OK) {
        // Remount the filesystem
        res = f_mount(&MMC1FatFs, (TCHAR const*)MMC1Path, 1);
        if (res == FR_OK) {
            SYSLOG_INFO_POLL("[LogFetching] Filesystem cleanup completed successfully");
            xSemaphoreGive(fsMutex);
            return E_OK;
        } else {
            SYSLOG_ERROR_POLL("[LogFetching] Failed to remount after cleanup");
        }
    } else {
        SYSLOG_ERROR_POLL("[LogFetching] f_mkfs failed during cleanup");
        // Try to remount anyway
        f_mount(&MMC1FatFs, (TCHAR const*)MMC1Path, 1);
    }

    xSemaphoreGive(fsMutex);
    return E_ERROR;
}

int FS_GetDaysUntilCleanup(void) {
    FIL file;
    FRESULT res;
    char epoch_str[16];
    UINT bytes_read;
    uint32_t stored_epoch = 0;
    uint32_t current_epoch;

    // Get current epoch using existing function
    current_epoch = Utils_GetEpoch();

    res = f_open(&file, DATE_TIME_FILENAME, FA_READ);
    if (res != FR_OK) {
        return -1; // File doesn't exist
    }

    res = f_read(&file, epoch_str, sizeof(epoch_str) - 1, &bytes_read);
    f_close(&file);

    if (res != FR_OK || bytes_read == 0) {
        return -1;
    }

    epoch_str[bytes_read] = '\0';

    if (sscanf(epoch_str, "%lu", &stored_epoch) != 1) {
        return -1; // Invalid format
    }

    uint32_t elapsed_seconds = current_epoch - stored_epoch;
    uint32_t elapsed_days = elapsed_seconds / SECONDS_PER_DAY;

    return LOG_RETENTION_DAYS - elapsed_days;
}
