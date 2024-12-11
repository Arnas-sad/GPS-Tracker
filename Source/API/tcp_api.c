/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "message.h"
#include "uart_api.h"
#include "debug_api.h"
#include "modem_api.h"
#include "heap_api.h"
#include "tcp_api.h"
#include "tcp_app.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define COMMAND_PARAMETERS_BUFFER_SIZE 50
#define MODEM_LOCK_TIMEOUT_MS 450
#define MAX_PORT 65536
#define MIN_PORT 0
#define MODEM_UART eUartApiDevice_Modem
#define FAILED_TO_LOCK_MODEM "Failed to lock the modem!\r\n"
#define FAILED_TO_UNLOCK_MODEM "Failed to unlock the modem!\r\n"
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
 
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
CREATE_MODULE_TAG(TCP_API);
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
size_t cmd_params_size = 0;
static char cmd_params_str[COMMAND_PARAMETERS_BUFFER_SIZE] = {0};
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
 
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
 
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
eModemError_t TCP_API_Connect (eServerId_t connect_id, char *ip_address, size_t port) {
    if ((ip_address == NULL) || (port < MIN_PORT) || (port > MAX_PORT) ||
        (connect_id < eServerId_First) || (connect_id >= eServerId_Last)) {
        DEBUG_INFO("Invalid IP address, port or socket ID!\r\n");
        return eModemError_InvalidParameters;
    }

    if (Modem_API_GetState() != eModemState_Initialized) {
        DEBUG_ERROR("Modem is not initialized yet, wait for modem to be initialized!\r\n");
        return eModemError_InvalidState;
    }

    if (Modem_API_LockModem(MODEM_LOCK_TIMEOUT_MS) == false) {
        return eModemError_ResourceBusy;
    }

    size_t cmd_params_size = 0;
    cmd_params_size = snprintf(cmd_params_str, COMMAND_PARAMETERS_BUFFER_SIZE, 
                               "1,%d,\"TCP\",\"%s\",%u,0,1", connect_id, ip_address, port);
    if (Modem_API_SendCommand(eModemCommands_QIOPEN, eModemFlags_ServerOpen, cmd_params_str, cmd_params_size) != eModemError_ATSuccess) {
        if (Modem_API_UnlockModem() == false) {
            return eModemError_Unknown;
        }
        return eModemError_SendFail;
    }

    if (Modem_API_UnlockModem() == false) {
        return eModemError_Unknown;
    }

    return eModemError_ATSuccess;
}

eModemError_t TCP_API_Send (eServerId_t connect_id, char *server_data_str, size_t server_data_size) {
    if ((connect_id < eServerId_First) || (connect_id >= eServerId_Last) || 
        (server_data_str == NULL) || (server_data_size == 0)) {
        DEBUG_INFO("Incorrect socket ID or data!\r\n");
        Heap_API_Free(server_data_str);
        return eModemError_InvalidParameters;
    }

    if (Modem_API_LockModem(MODEM_LOCK_TIMEOUT_MS) == false) {
        Heap_API_Free(server_data_str);
        return eModemError_ResourceBusy;
    }

    sString_t data_to_server = {.size = server_data_size, .str = server_data_str};

    size_t cmd_params_size = 0;
    cmd_params_size = snprintf(cmd_params_str, COMMAND_PARAMETERS_BUFFER_SIZE, "%d", connect_id);
    if (Modem_API_SendCommand(eModemCommands_QISEND, eModemFlags_SendOK, cmd_params_str, cmd_params_size) != eModemError_ATSuccess) {
        if (Modem_API_UnlockModem() == false) {
            Heap_API_Free(server_data_str);
            return eModemError_Unknown;
        }
        Heap_API_Free(server_data_str);
        return eModemError_SendFail;
    }

    osDelay(10);

    if (UART_API_SendMessage(MODEM_UART, data_to_server) == false) {
        DEBUG_ERROR("Failed to send %s command!\r\n", data_to_server);
        Heap_API_Free(server_data_str);
        return eModemError_SendFail;
    }

    Heap_API_Free(data_to_server.str);

    if (Modem_API_UnlockModem() == false) {
        return eModemError_Unknown;
    }
    
    return eModemError_ATSuccess;
}

eModemError_t TCP_API_Disconnect (eServerId_t connect_id) {
    if ((connect_id < eServerId_First) || (connect_id >= eServerId_Last)) {
        DEBUG_INFO("Incorrect socket ID!\r\n");
        return eModemError_InvalidParameters;
    }

    if (Modem_API_LockModem(MODEM_LOCK_TIMEOUT_MS) == false) {
        return eModemError_ResourceBusy;
    }

    size_t cmd_params_size = 0;
    cmd_params_size = snprintf(cmd_params_str, COMMAND_PARAMETERS_BUFFER_SIZE, "%d", connect_id);
    if (Modem_API_SendCommand(eModemCommands_QICLOSE, eModemFlags_ResponseOK, cmd_params_str, cmd_params_size) != eModemError_ATSuccess) {
        if (Modem_API_UnlockModem() == false) {
            return eModemError_Unknown;
        }
        return eModemError_SendFail;
    }

    if (Modem_API_UnlockModem() == false) {
        return eModemError_Unknown;
    }

	return eModemError_ATSuccess;
}
