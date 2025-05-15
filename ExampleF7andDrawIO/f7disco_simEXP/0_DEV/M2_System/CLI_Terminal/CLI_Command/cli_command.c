/*
 * cli_setup.c
 *
 *  Created on: Feb 27, 2025
 *      Author: CAO HIEU
 */
#include "cli_command.h"
#include "stdio.h"
#include "CLI_Terminal/CLI_Setup/cli_setup.h"
#include "stdlib.h"
#include "string.h"
#include "main.h"
/*************************************************
 *              More User Include                *
 *************************************************/
#include "DateTime/date_time.h"
#include "Struct/gpio_state.h"
#include "QSPI/quadspi.h"
#include "SPI_SlaveOfOBC/spi_slave.h"
extern uint32_t g_total_size;
extern uint16_t g_chunk_size;
/*************************************************
 *                Command Define                 *
 *************************************************/
static void CMD_ClearCLI(EmbeddedCli *cli, char *args, void *context);
static void CMD_RtcSet(EmbeddedCli *cli, char *args, void *context);
static void CMD_RtcGet(EmbeddedCli *cli, char *args, void *context);
static void CMD_CollectData(EmbeddedCli *cli, char *args, void *context);
static void CMD_QspiRead(EmbeddedCli *cli, char *args, void *context);
static void CMD_DumpQspi(EmbeddedCli *cli, char *args, void *context);
static void CMD_DumpChunk(EmbeddedCli *cli, char *args, void *context);
static void CMD_DumpPreSpi(EmbeddedCli *cli, char *args, void *context);
/*************************************************
 *                 Command  Array                *
 *************************************************/
// Guide: Command bindings are declared in the following order:
// { category, name, help, tokenizeArgs, context, binding }
// - category: Command group; set to NULL if grouping is not needed.
// - name: Command name (required)
// - help: Help string describing the command (required)
// - tokenizeArgs: Set to true to automatically split arguments when the command is called.
// - context: Pointer to a command-specific context; can be NULL.
// - binding: Callback function that handles the command.

static const CliCommandBinding cliStaticBindings_internal[] = {
    { "Ultis",		 	"help",        	"Print list of commands [Firmware: 1]",             	false,  NULL, CMD_Help,			 },
    { "Ultis",			"cls",         	"Clears the console",                               	false,  NULL, CMD_ClearCLI,  	 },
    { "Time", 			"rtc_set",     	"Set RTC time: rtc_set <h> <m> <s> <DD> <MM> <YY>",   	true,  	NULL, CMD_RtcSet,    	 },
    { "Time",         	"rtc_get",     	"Get RTC data. Usage: rtc_get <soft|work|all>",  		true,  	NULL, CMD_RtcGet,    	 },
    { "Data", 			"collect_data", "Collect data: collect_data <type> <sample>", 			true,   NULL, CMD_CollectData 	 },
    { "QSPI",           "read_qspi",   	"Read QSPI Flash: qspi_read <size> (1-100KB)",       	true,   NULL, CMD_QspiRead,   	 },
    { "QSPI",           "dump_qspi",   	"Dump QSPI data: dump_qspi <size> (1-100KB)",        	true,   NULL, CMD_DumpQspi,   	 },
	{ "QSPI", 			"dump_chunk", 	"Dump chunk data: dump_chunk <slot>", 					true, 	NULL, CMD_DumpChunk 	 },
	{ "QSPI", 			"dump_prespi", 	"Dump SPI RAM data: dump_prespi <size> (1-10KB)", 		true, 	NULL, CMD_DumpPreSpi 	 },
};
/*************************************************
 *                 External Declarations         *
 *************************************************/
void dump_buffer(EmbeddedCli *cli, const char *buffer, size_t size);

#define MAX_SIZE (100 * 1024) // 100KB
#define RAM_D2_200KB_START ((uint8_t*)&_scustom_data)
#define RAM_D2_200KB_SIZE  (200 * 1024) // 200KB

extern uint32_t _scustom_data;
extern uint32_t _ecustom_data;

#define SPI_RAM_START ((uint8_t*)&_schunk_data)
#define SPI_RAM_SIZE  (10 * 1024)

extern uint8_t _schunk_data[];
extern uint8_t _echunk_data[];

static uint16_t UpdateCRC16_XMODEM(uint16_t crc, uint8_t byte) {
    const uint16_t polynomial = 0x1021; // CRC16 XMODEM
    crc ^= (uint16_t)byte << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ polynomial;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

void dump_buffer(EmbeddedCli *cli, const char *buffer, size_t size) {
    char line[80];
    const uint32_t bytes_per_line = 16;

    for (size_t i = 0; i < size; i += bytes_per_line) {
        snprintf(line, sizeof(line), "%04X: ", (unsigned int)i);
        char *ptr = line + strlen(line);

        for (size_t j = 0; j < bytes_per_line && (i + j) < size; j++) {
            snprintf(ptr, sizeof(line) - (ptr - line), "%02X ", (unsigned char)buffer[i + j]);
            ptr += 3;
        }

        while (ptr < line + 3 * bytes_per_line + 6) {
            *ptr++ = ' ';
        }

        *ptr++ = '|';
        for (size_t j = 0; j < bytes_per_line && (i + j) < size; j++) {
            char c = buffer[i + j];
            *ptr++ = (c >= 32 && c <= 126) ? c : '.';
        }
        *ptr = '\0';

        embeddedCliPrint(cli, line);
        HAL_Delay(1);// Nhường CPU
    }
}
/*************************************************
 *             Command List Function             *
 *************************************************/
static void CMD_ClearCLI(EmbeddedCli *cli, char *args, void *context) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "\33[2J");
    embeddedCliPrint(cli, buffer);
}

static void CMD_RtcSet(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1); // hour
    const char *arg2 = embeddedCliGetToken(args, 2); // minute
    const char *arg3 = embeddedCliGetToken(args, 3); // second
    const char *arg4 = embeddedCliGetToken(args, 4); // day
    const char *arg5 = embeddedCliGetToken(args, 5); // month
    const char *arg6 = embeddedCliGetToken(args, 6); // year

    char buffer[100];
    if (arg1 == NULL || arg2 == NULL || arg3 == NULL ||
        arg4 == NULL || arg5 == NULL || arg6 == NULL) {
        snprintf(buffer, sizeof(buffer),
                 "Usage: rtc_set <hour> <minute> <second> <day> <month> <year>");
        embeddedCliPrint(cli, buffer);
        return;
    }

    int hour   = atoi(arg1);
    int minute = atoi(arg2);
    int second = atoi(arg3);
    int day    = atoi(arg4);
    int month  = atoi(arg5);
    int year   = atoi(arg6);

    if (hour < 0 || hour > 23) {
        embeddedCliPrint(cli, "Invalid hour (must be 0-23). Please enter again.");
        return;
    }
    if (minute < 0 || minute > 59) {
        embeddedCliPrint(cli, "Invalid minute (must be 0-59). Please enter again.");
        return;
    }
    if (second < 0 || second > 59) {
        embeddedCliPrint(cli, "Invalid second (must be 0-59). Please enter again.");
        return;
    }
    if (day < 1 || day > 31) {
        embeddedCliPrint(cli, "Invalid day (must be 1-31). Please enter again.");
        return;
    }
    if (month < 1 || month > 12) {
        embeddedCliPrint(cli, "Invalid month (must be 1-12). Please enter again.");
        return;
    }
    if (year < 0 || year > 99) {
        embeddedCliPrint(cli, "Invalid year (must be 2 digits, e.g., 25 for 2025). Please enter again.");
        return;
    }

    s_DateTime dt;
    dt.hour   = (uint8_t)hour;
    dt.minute = (uint8_t)minute;
    dt.second = (uint8_t)second;
    dt.day    = (uint8_t)day;
    dt.month  = (uint8_t)month;
    dt.year   = (uint8_t)year;

    Utils_SetRTC(&dt);

    snprintf(buffer, sizeof(buffer),
             "--> RTC set to %02d:%02d:%02d, %02d/%02d/20%02d",
             dt.hour, dt.minute, dt.second, dt.day, dt.month, dt.year);
    embeddedCliPrint(cli, buffer);
    embeddedCliPrint(cli, "");
}

static void CMD_RtcGet(EmbeddedCli *cli, char *args, void *context) {
    const char *mode = embeddedCliGetToken(args, 1);
    char buffer[100];

    if (mode == NULL) {
        snprintf(buffer, sizeof(buffer), "Usage: rtc_get <soft|work|all>");
        embeddedCliPrint(cli, buffer);
        return;
    }

    if (strcmp(mode, "soft") == 0) {
        s_DateTime rtc;
        Utils_GetRTC(&rtc);
        snprintf(buffer, sizeof(buffer),
                 "--> Soft RTC: Time: %02d:%02d:%02d, Date: %02d/%02d/20%02d",
                 rtc.hour, rtc.minute, rtc.second,
                 rtc.day, rtc.month, rtc.year);
        embeddedCliPrint(cli, buffer);
    } else if (strcmp(mode, "work") == 0) {
        uint32_t days = 0;
        uint8_t hours = 0, minutes = 0, seconds = 0;
        Utils_GetWorkingTime(&days, &hours, &minutes, &seconds);
        snprintf(buffer, sizeof(buffer),
                        "--> Working Uptime: Time: %02d:%02d:%02d, Days: %d",
                        hours, minutes, seconds, (uint8_t)days);
        embeddedCliPrint(cli, buffer);
    } else if (strcmp(mode, "all") == 0) {
        // Soft RTC
        s_DateTime rtc;
        Utils_GetRTC(&rtc);
        snprintf(buffer, sizeof(buffer),
                 "--> Soft RTC: Time: %02d:%02d:%02d, Date: %02d/%02d/20%02d",
                 rtc.hour, rtc.minute, rtc.second,
                 rtc.day, rtc.month, rtc.year);
        embeddedCliPrint(cli, buffer);
        // Working uptime:
        uint32_t days = 0;
        uint8_t hours = 0, minutes = 0, seconds = 0;
        Utils_GetWorkingTime(&days, &hours, &minutes, &seconds);
        snprintf(buffer, sizeof(buffer),
                        "--> Working Uptime: Time: %02d:%02d:%02d, Days: %d",
                        hours, minutes, seconds, (uint8_t)days);
        embeddedCliPrint(cli, buffer);
        // Epoch
        uint32_t epoch = Utils_GetEpoch();
        snprintf(buffer, sizeof(buffer), "--> Epoch: %lu", (unsigned long)epoch);
        embeddedCliPrint(cli, buffer);
    } else {
        snprintf(buffer, sizeof(buffer), "Unknown mode. Use: rtc_get <soft|work|all>");
        embeddedCliPrint(cli, buffer);
    }
    embeddedCliPrint(cli, "");
}

static void CMD_CollectData(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1); // type
    const char *arg2 = embeddedCliGetToken(args, 2); // sample
    char buffer[100];

    if (arg1 == NULL || arg2 == NULL) {
        embeddedCliPrint(cli, "Usage: collect_data <type> <sample>");
        return;
    }

    uint8_t type = (uint8_t)atoi(arg1);
    uint32_t sample = (uint32_t)strtoul(arg2, NULL, 0);

    Std_ReturnType ret = SPI_SlaveDevice_CollectData(type, sample, (uint32_t)RAM_D2_200KB_START);
    if (ret == E_OK) {
        DataProcessContext_t ctx;
        if (SPI_SlaveDevice_GetDataInfo(&ctx) == E_OK) {
            snprintf(buffer, sizeof(buffer), "Collected %lu samples (type %d), size: %lu bytes, CRC: 0x%04X",
                     (unsigned long)ctx.sample, ctx.type, (unsigned long)ctx.data_size, ctx.crc);
            embeddedCliPrint(cli, buffer);
        }
    } else if (ret == E_BUSY) {
        snprintf(buffer, sizeof(buffer), "Type %d not implemented yet.", type);
        embeddedCliPrint(cli, buffer);
    } else {
        snprintf(buffer, sizeof(buffer), "Failed to collect data. Error code: %d", ret);
        embeddedCliPrint(cli, buffer);
    }

    embeddedCliPrint(cli, "");
}

static void CMD_QspiRead(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1); // size
    char buffer[100];

    if (arg1 == NULL) {
        snprintf(buffer, sizeof(buffer), "Usage: qspi_read <size> (size: 1-100KB)");
        embeddedCliPrint(cli, buffer);
        return;
    }

    uint32_t size = (uint32_t)strtoul(arg1, NULL, 0);
    if (size < 1 || size > MAX_SIZE) {
        snprintf(buffer, sizeof(buffer), "Invalid size. Must be 1 to %lu bytes.", (unsigned long)MAX_SIZE);
        embeddedCliPrint(cli, buffer);
        return;
    }

    if (CSP_QSPI_Read(RAM_D2_200KB_START, 0, size) != HAL_OK) {
        snprintf(buffer, sizeof(buffer), "Error reading QSPI Flash.");
        embeddedCliPrint(cli, buffer);
        return;
    }

    uint16_t crc = 0x0000;
    for (uint32_t i = 0; i < size; i++) {
        crc = UpdateCRC16_XMODEM(crc, RAM_D2_200KB_START[i]);
    }

    HAL_Delay(1);
    snprintf(buffer, sizeof(buffer), "Read %lu bytes from QSPI Flash, CRC16-XMODEM: 0x%04X",
             (unsigned long)size, crc);
    embeddedCliPrint(cli, buffer);
    embeddedCliPrint(cli, "");
}

static void CMD_DumpQspi(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1); // size
    char buffer[100];

    if (arg1 == NULL) {
        snprintf(buffer, sizeof(buffer), "Usage: dump_qspi <size> (size: 1-100KB)");
        embeddedCliPrint(cli, buffer);
        return;
    }

    uint32_t size = (uint32_t)strtoul(arg1, NULL, 0);
    if (size < 1 || size > MAX_SIZE) {
        snprintf(buffer, sizeof(buffer), "Invalid size. Must be 1 to %lu bytes.", (unsigned long)MAX_SIZE);
        embeddedCliPrint(cli, buffer);
        return;
    }

    snprintf(buffer, sizeof(buffer), "Dumping %lu bytes from custom RAM:", (unsigned long)size);
    embeddedCliPrint(cli, buffer);
    dump_buffer(cli, (const char*)SPI_RAM_START, size);
    embeddedCliPrint(cli, "Dump complete.");
    embeddedCliPrint(cli, "");
}

static void CMD_DumpChunk(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1); // slot
    char buffer[100];

    if (arg1 == NULL) {
        snprintf(buffer, sizeof(buffer), "Usage: dump_chunk <slot>");
        embeddedCliPrint(cli, buffer);
        return;
    }

    uint32_t slot = (uint32_t)strtoul(arg1, NULL, 0);
    uint32_t num_chunks = (g_total_size + g_chunk_size - 1) / g_chunk_size;

    if (g_total_size == 0 || g_chunk_size == 0) {
        snprintf(buffer, sizeof(buffer), "No data or chunk size set.");
        embeddedCliPrint(cli, buffer);
        return;
    }

    if (slot >= num_chunks) {
        snprintf(buffer, sizeof(buffer), "Invalid slot (%lu >= %lu).", (unsigned long)slot, (unsigned long)num_chunks);
        embeddedCliPrint(cli, buffer);
        return;
    }

    uint32_t offset = slot * g_chunk_size;
    uint32_t size = (slot == num_chunks - 1) ? (g_total_size % g_chunk_size) : g_chunk_size;
    if (size == 0) size = g_chunk_size;

    // Tính CRC16
    uint16_t crc = 0x0000;
    for (uint32_t i = 0; i < size; i++) {
        crc = UpdateCRC16_XMODEM(crc, RAM_D2_200KB_START[offset + i]);
    }

    snprintf(buffer, sizeof(buffer), "Dumping chunk %lu (size: %lu bytes, CRC16: 0x%04X):",
             (unsigned long)slot, (unsigned long)size, crc);
    embeddedCliPrint(cli, buffer);
    dump_buffer(cli, (const char*)(RAM_D2_200KB_START + offset), size);
    embeddedCliPrint(cli, "Dump complete.");
    embeddedCliPrint(cli, "");
}

static void CMD_DumpPreSpi(EmbeddedCli *cli, char *args, void *context) {
    const char *arg1 = embeddedCliGetToken(args, 1); // size
    char buffer[100];

    if (arg1 == NULL) {
        snprintf(buffer, sizeof(buffer), "Usage: dump_prespi <size> (1-10KB)");
        embeddedCliPrint(cli, buffer);
        return;
    }

    uint32_t size = (uint32_t)strtoul(arg1, NULL, 0);
    if (size < 1 || size > SPI_RAM_SIZE) {
        snprintf(buffer, sizeof(buffer), "Invalid size. Must be 1 to %lu bytes.", (unsigned long)SPI_RAM_SIZE);
        embeddedCliPrint(cli, buffer);
        return;
    }

    snprintf(buffer, sizeof(buffer), "Dumping %lu bytes from SPI RAM:", (unsigned long)size);
    embeddedCliPrint(cli, buffer);
    dump_buffer(cli, (const char*)SPI_RAM_START, size);
    embeddedCliPrint(cli, "Dump complete.");
    embeddedCliPrint(cli, "");
}

/*************************************************
 *                  End CMD List                 *
 *************************************************/

/*************************************************
 *                Getter - Helper                *
 *************************************************/
const CliCommandBinding *getCliStaticBindings(void) {
    return cliStaticBindings_internal;
}

uint16_t getCliStaticBindingCount(void) {
    return sizeof(cliStaticBindings_internal) / sizeof(cliStaticBindings_internal[0]);
}


