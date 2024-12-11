/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <string.h>
#include "cmsis_os2.h"
#include "string_util.h"
#include "debug_api.h"
#include "modem_api.h"
#include "cmd_api.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define INCORRECT_COMMAND_ARGUMENTS "Incorrect command arguments!\r\n"
#define FAILED_TO_SEPERATE_ARGUMENTS "Failed to seperate arguments!\r\n"
#define FLAG_SET_FAILED "Failed to set the flag!\r\n"
#define ERROR_TYPE_IDENTIFICATION_FAIL "Failed to identify the error!\r\n"
#define OPERATION_SUCCESSFUL 0
#define ERROR(ERR) {.str = #ERR, .size = sizeof(#ERR) - 1}
#define TABLE_SIZE 25
#define ERROR_TYPE_MESSAGE_BUFFER 50
#define GET_LAST_ERROR_COMMAND_BUFFER_SIZE 20
#define ARGUMENTS_SEPERATOR " ,\""
#define RECEIVED_COMMAND_ARGUMENTS_SEPERATOR " \r\n"
#define ERROR_STRING_SIZE 35
#define REG_TO_HOME_NET 1
#define REG_ROAMING 5
#define MODEM_LOCK_TIMEOUT_MS 450
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
typedef enum eErrorId {
    eErrorId_First = 0,
    eErrorId_UnknownError = eErrorId_First,
    eErrorId_OperationBlocked,
    eErrorId_InvalidParameters,
    eErrorId_MemoryNotEnough,
    eErrorId_CreateSocketFailed,
    eErrorId_OperationNotSupported,
    eErrorId_SocketBindFailed,
    eErrorId_SocketListenFailed,
    eErrorId_SocketWriteFailed,
    eErrorId_SocketReadFailed,
    eErrorId_SocketAcceptFailed,
    eErrorId_OpenPDPContextFailed,
    eErrorId_ClosePDPContextFailed,
    eErrorId_SocketIdentityUsed,
    eErrorId_DNSBusy,
    eErrorId_DNSParseFailed,
    eErrorId_SocketConnectFailed,
    eErrorId_SocketClosed,
    eErrorId_OperationBusy,
    eErrorId_OperationTimeout,
    eErrorId_PDPContextBroken,
    eErrorId_CancelSend,
    eErrorId_OperationNotAllowed,
    eErrorId_APNNotConfigured,
    eErrorId_PortBusy,
    eErrorId_Last
} eErrorId_t;

typedef struct sErrorSpecs {
    sString_t name;
    size_t id;
} sErrorSpecs_t;
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
CREATE_MODULE_TAG(MODEM_API_COMMANDS);

static const sErrorSpecs_t error_response [TABLE_SIZE] = {
    [eErrorId_UnknownError]          = {.name = ERROR(unknown error occurred!\r\n),              .id = 550},
    [eErrorId_OperationBlocked]      = {.name = ERROR(operation is currently blocked!\r\n),      .id = 551},
    [eErrorId_InvalidParameters]     = {.name = ERROR(invalid parameters provided!\r\n),         .id = 552},
    [eErrorId_MemoryNotEnough]       = {.name = ERROR(not enough memory available!\r\n),         .id = 553},
    [eErrorId_CreateSocketFailed]    = {.name = ERROR(failed to create socket!\r\n),             .id = 554},
    [eErrorId_OperationNotSupported] = {.name = ERROR(operation is not supported!\r\n),          .id = 555},
    [eErrorId_SocketBindFailed]      = {.name = ERROR(failed to bind socket!\r\n),               .id = 556},
    [eErrorId_SocketListenFailed]    = {.name = ERROR(failed to start listening on socket!\r\n), .id = 557},
    [eErrorId_SocketWriteFailed]     = {.name = ERROR(failed to write data to socket!\r\n),      .id = 558},
    [eErrorId_SocketReadFailed]      = {.name = ERROR(failed to read data from socket!\r\n),     .id = 559},    
    [eErrorId_SocketAcceptFailed]    = {.name = ERROR(failed to accept socket connection!\r\n),  .id = 560},
    [eErrorId_OpenPDPContextFailed]  = {.name = ERROR(failed to open PDP context!\r\n),          .id = 561},
    [eErrorId_ClosePDPContextFailed] = {.name = ERROR(failed to close PDP context!\r\n),         .id = 562},
    [eErrorId_SocketIdentityUsed]    = {.name = ERROR(socket identity is already in use!\r\n),   .id = 563},
    [eErrorId_DNSBusy]               = {.name = ERROR(DNS service is busy!\r\n),                 .id = 564},
    [eErrorId_DNSParseFailed]        = {.name = ERROR(failed to parse DNS response!\r\n),        .id = 565},
    [eErrorId_SocketConnectFailed]   = {.name = ERROR(failed to connect socket!\r\n),            .id = 566},
    [eErrorId_SocketClosed]          = {.name = ERROR(socket has been closed!\r\n),              .id = 567},
    [eErrorId_OperationBusy]         = {.name = ERROR(operation is currently busy!\r\n),         .id = 568},
    [eErrorId_OperationTimeout]      = {.name = ERROR(operation timed out!\r\n),                 .id = 569},
    [eErrorId_PDPContextBroken]      = {.name = ERROR(PDP context has been broken!\r\n),         .id = 570},
    [eErrorId_CancelSend]            = {.name = ERROR(send operation was canceled!\r\n),         .id = 571},
    [eErrorId_OperationNotAllowed]   = {.name = ERROR(operation is not allowed!\r\n),            .id = 572},
    [eErrorId_APNNotConfigured]      = {.name = ERROR(APN is not configured!\r\n),               .id = 573},
    [eErrorId_PortBusy]              = {.name = ERROR(port is currently busy!\r\n),              .id = 574}
};
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
static char g_converted_number[10] = {0};
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
static bool MODEM_CMD_IsStringNumber (char *string, size_t string_size);
static bool MODEM_CMD_GetArgInt (int *arg_value, char **save_ptr);
static bool MODEM_CMD_IdentifyError (uint32_t error_id, sBuffer_t *error_msg);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
static bool MODEM_CMD_IsStringNumber (char *string, size_t string_size) {
    if (memcmp(string, itoa(atoi(string), g_converted_number, 10), strnlen(string, string_size)) == 0) {
        return true;
    }

    return false;
}

static bool MODEM_CMD_GetArgInt (int *arg_value, char **save_ptr) {
    char *arg_token = strtok_r(NULL, ARGUMENTS_SEPERATOR, save_ptr);

    if (arg_token == NULL) {
        return false;
    }

    if (MODEM_CMD_IsStringNumber(arg_token, strlen(arg_token)) == false) {
        return false;
    }

    *arg_value = atoi(arg_token);
    return true;
}

static bool MODEM_CMD_IdentifyError (uint32_t error_id, sBuffer_t *error_msg) {
    for (size_t i = eErrorId_First; i < eErrorId_Last; i++) {
        if (error_id == error_response[i].id) {
            snprintf(&error_msg->str[error_msg->count], error_msg->size - error_msg->count, "%s\r\n",
                     error_response[i].name.str);
            return true;
        }
        continue;
    }

    snprintf(&error_msg->str[error_msg->count], error_msg->size - error_msg->count,
             "error id:%lu does not exist!\r\n", error_id);

    return false;
}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool Modem_API_CMD_OK (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    if (Modem_API_SetFlag(eModemFlags_ResponseOK) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FLAG_SET_FAILED);
        return false;
    }

    modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                          modem_handler_args->response_buffer->size, 
                                                          "Response from modem: OK!\r\n");

    return true;
}

bool Modem_API_CMD_DisableEcho (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }
    
    if (Modem_API_SetFlag(eModemFlags_EchoDisabled) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FLAG_SET_FAILED);
        return false;
    }

    modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                          modem_handler_args->response_buffer->size, 
                                                          "Echo Mode Disabled!\r\n");

    return true;
}

bool Modem_API_CMD_Error (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    if (Modem_API_SetFlag(eModemFlags_Error) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FLAG_SET_FAILED);
        return false;
    }

    osDelay(5);

    if (Modem_API_LockModem(MODEM_LOCK_TIMEOUT_MS) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              "Failed to lock the modem!\r\n");
        return false;
    }

    size_t latest_error_size = 0;
    char latest_error_str[GET_LAST_ERROR_COMMAND_BUFFER_SIZE] = {0};
    latest_error_size = snprintf(latest_error_str, GET_LAST_ERROR_COMMAND_BUFFER_SIZE, "ERROR") + 1;
    if (Modem_API_SendCommand(eModemCommands_QIGETERROR, eModemFlags_Error, latest_error_str, latest_error_size) != eModemError_ATSuccess) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              "Failed to get the latest encountered error!\r\n");
    }

    if (Modem_API_UnlockModem() == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              "Failed to unlock the modem!\r\n");
        return false;
    }

    return true;
}

bool Modem_API_CMD_GetError (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    int error_id;
    if (MODEM_CMD_GetArgInt(&error_id, &modem_handler_args->cmd_args.str) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                        modem_handler_args->response_buffer->size,
                                                        FAILED_TO_SEPERATE_ARGUMENTS);
        return false;
    }

    if (error_id == OPERATION_SUCCESSFUL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              "%s!\r\n", modem_handler_args->cmd_args.str);

        return true;
    } else {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              "Error encountered: %s\r\n", modem_handler_args->cmd_args.str);
    }

    return true;
}

bool Modem_API_CMD_NetworkRegStatus (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    int urc_control;
    if (MODEM_CMD_GetArgInt(&urc_control, &modem_handler_args->cmd_args.str) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              FAILED_TO_SEPERATE_ARGUMENTS);
        return false;
    }

    int reg_status;
    if (MODEM_CMD_GetArgInt(&reg_status, &modem_handler_args->cmd_args.str) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              FAILED_TO_SEPERATE_ARGUMENTS);
        return false;
    }

    if ((reg_status != REG_TO_HOME_NET) && (reg_status != REG_ROAMING)) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size + 1,
                                                              "Device is not registered or registration denied!\r\n");
        return false;
    }

    if (Modem_API_SetFlag(eModemFlags_Registered) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FLAG_SET_FAILED);
        return false;
    }

    return true;
}

bool Modem_API_CMD_AddressPDP (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    int context_id;
    if (MODEM_CMD_GetArgInt(&context_id, &modem_handler_args->cmd_args.str) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FAILED_TO_SEPERATE_ARGUMENTS);
        return false;
    }

    char *pdp_address = strtok_r(NULL, ARGUMENTS_SEPERATOR, &modem_handler_args->cmd_args.str);

    if (pdp_address == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FAILED_TO_SEPERATE_ARGUMENTS);
        return false;
    }

    if (Modem_API_SetFlag(eModemFlags_ValidPDPAddress) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FLAG_SET_FAILED);
        return false;
    }

    modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                          modem_handler_args->response_buffer->size, 
                                                          "Assigned IP address: %s, of context: %d!\r\n", 
                                                          pdp_address, context_id);

    return true;
}

bool Modem_API_CMD_QIOPEN (sCommandHandlerArgs_t *modem_handler_args) {
    if ((modem_handler_args->cmd_args.str == NULL) || (modem_handler_args->cmd_args.size == 0)) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    int socket_id;
    if (MODEM_CMD_GetArgInt(&socket_id, &modem_handler_args->cmd_args.str) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str,
                                                              modem_handler_args->response_buffer->size, 
                                                              FAILED_TO_SEPERATE_ARGUMENTS);
        return false;
    }

    int error_id;
    if (MODEM_CMD_GetArgInt(&error_id, &modem_handler_args->cmd_args.str) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FAILED_TO_SEPERATE_ARGUMENTS);
        return false;
    }

    if (error_id == 0) {
        if (Modem_API_SetFlag(eModemFlags_ServerOpen) == false) {
            modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                                modem_handler_args->response_buffer->size, 
                                                                FLAG_SET_FAILED);
            return false;
        }

        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              "Link with a server has been established!\r\n");
    } else {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              "Error occured while opening the link with the server: ");

        MODEM_CMD_IdentifyError(error_id, modem_handler_args->response_buffer);
    }    

    return true;
}

bool Modem_API_CMD_ReadyToSend (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        DEBUG_INFO(INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                          modem_handler_args->response_buffer->size, 
                                                          "Modem is ready to send data!\r\n");

    return true;
}

bool Modem_CMD_SendOk (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    if (Modem_API_SetFlag(eModemFlags_SendOK) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FLAG_SET_FAILED);
        return false;
    }

    modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                          modem_handler_args->response_buffer->size, 
                                                          "Data successfully sent to the server!\r\n");

    return true;
}

bool Modem_API_CMD_SendFail (sCommandHandlerArgs_t *modem_handler_args) {
    if (modem_handler_args->cmd_args.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size,
                                                              INCORRECT_COMMAND_ARGUMENTS);
        return false;
    }

    if (Modem_API_SetFlag(eModemFlags_SendFail) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FLAG_SET_FAILED);
        return false;
    }

    modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                          modem_handler_args->response_buffer->size, 
                                                          "Failed to send data to the server!\r\n");

    return true;
}

bool Modem_API_CMD_QIURC (sCommandHandlerArgs_t *modem_handler_args) {
    sString_t received_data;
    received_data.str = strtok_r(NULL, RECEIVED_COMMAND_ARGUMENTS_SEPERATOR, &modem_handler_args->cmd_args.str);
    if (received_data.str == NULL) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                        modem_handler_args->response_buffer->size, 
                                                        "Invalid data received from the server!\r\n");
        return false;
    }

    if (Modem_API_SetFlag(eModemFlags_DataReceived) == false) {
        modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                              modem_handler_args->response_buffer->size, 
                                                              FLAG_SET_FAILED);
        return false;
    }

    modem_handler_args->response_buffer->count = snprintf(modem_handler_args->response_buffer->str, 
                                                          modem_handler_args->response_buffer->size, 
                                                          "Data from server: %s\r\n", received_data.str);

    return true;
}
