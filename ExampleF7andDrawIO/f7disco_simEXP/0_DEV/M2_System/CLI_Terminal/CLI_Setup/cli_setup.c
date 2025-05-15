/*
 * cli_setup.c
 *
 *  Created on: Feb 27, 2025
 *      Author: CAO HIEU
 */

#include "cli_setup.h"
#include "CLI_Terminal/CLI_Command/cli_command.h"
#include "main.h"

extern UART_HandleTypeDef huart1;

// CLI buffer
/*************************************************
 *           CLI Static Buffer Define            *
 *************************************************/

#define CLI_BUFFER_SIZE 2048
static  CLI_UINT cliStaticBuffer[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];

/*************************************************
 *             ----------------------            *
 *************************************************/

static EmbeddedCli *cli_native;

// Bool to disable the interrupts, if CLI is not yet ready.
static _Bool cliIsReady = false;

/*************************************************
 *          Tx Transmit CLI Byte Buffer          *
 *************************************************/

static void writeCharToCli(EmbeddedCli *embeddedCli, char c) {
	HAL_UART_Transmit(&huart1, (uint8_t *) &c, 1, 0xFFFF);
}

//Call before FREERTOS be initialized
//Call After UART Driver Init (or Peripheral use CLI)

Std_ReturnType SystemCLI_Init() {
    // Initialize the CLI configuration settings
    // Initialize UART CM4 CLI
    EmbeddedCliConfig *native_config = embeddedCliDefaultConfig();
    native_config->cliBuffer = cliStaticBuffer;
    native_config->cliBufferSize = CLI_BUFFER_SIZE;
    native_config->rxBufferSize = CLI_RX_BUFFER_SIZE;
    native_config->cmdBufferSize = CLI_CMD_BUFFER_SIZE;
    native_config->historyBufferSize = CLI_HISTORY_SIZE;
    native_config->maxBindingCount = CLI_MAX_BINDING_COUNT;
    native_config->enableAutoComplete = CLI_AUTO_COMPLETE;
    native_config->invitation = CLI_INITATION;
    native_config->staticBindings = getCliStaticBindings();
    native_config->staticBindingCount = getCliStaticBindingCount();

    cli_native = embeddedCliNew(native_config);
    if (cli_native == NULL) {
        return E_ERROR;
    }
    cli_native->writeChar = writeCharToCli;

    // Init the CLI with blank screen
//    onClearCLI(cli, NULL, NULL);

    // CLI has now been initialized, set bool to true to enable interrupts.
    cliIsReady = true;

    return E_OK;
}


/*************************************************
 *             Get CLI Pointers                  *
 *************************************************/
EmbeddedCli *getCliPointer() {
    return cli_native;
}

