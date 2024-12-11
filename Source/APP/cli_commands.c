/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "cmsis_os2.h"
#include "debug_api.h"
#include "heap_api.h"
#include "cmd_api.h"
#include "cli_app.h"
#include "cli_commands.h"
#include "led_api.h"
#include "led_app.h"
#include "tcp_app.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define ARGUMENTS_SEPERATOR " ,"
#define REPLY_INCORRECT_ARG_MESSAGE "Incorrect command arguments!\r\n"
#define COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE 100
#define CONVERTED_BACK_NUMBER_STRING_SIZE 10
#define MAX_SIZE_OF_IPV4_ADDRESS 15
#define MAX_PORT 65536
#define MIN_PORT 0
#define DELIMITER "\r\n"
#define COMMAND_END_SYMBOL '\032'
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
typedef enum eLedState {
    eLedState_First = 0,
    eLedState_On = eLedState_First,
    eLedState_Off,
    eLedState_Last
} eLedState_t;
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
CREATE_MODULE_TAG(CLI_APP_COMMANDS);
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
static char g_converted_back_number[CONVERTED_BACK_NUMBER_STRING_SIZE] = {0};
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
static bool MODEM_CMD_GetArgInt (int *arg_value, char **save_ptr);
static bool MODEM_CMD_IsStringNumber (char *string, size_t string_size);
bool CLI_CMD_ExecuteLedCommand (sCommandHandlerArgs_t *handler_args, eLedState_t pin_status);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
static bool MODEM_CMD_IsStringNumber (char *string, size_t string_size) {
    if (memcmp(string, itoa(atoi(string), g_converted_back_number, 10), strnlen(string, string_size)) == 0) {
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

bool CLI_CMD_ExecuteLedCommand (sCommandHandlerArgs_t *handler_args, eLedState_t pin_status) {
    if (handler_args->cmd_args.str == NULL) {
        DEBUG_INFO("Function argument is not valid, the argument is a NULL!\r\n");
        return false;
    }

    if ((handler_args->cmd_args.str == NULL) || (handler_args->cmd_args.size == 0)) {
        DEBUG_INFO("No command arguments entered, please enter valid command arguments!\r\n");
        return false;
    }

    if (handler_args->cmd_args.str[handler_args->cmd_args.size] != '\0') {
        DEBUG_ERROR("Command message doesn't have a null terminator!\r\n");
    }

    int led_int;
    if (MODEM_CMD_GetArgInt(&led_int, &handler_args->cmd_args.str) == false) {
        handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str, 
                                                        COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1, 
                                                        REPLY_INCORRECT_ARG_MESSAGE);
        return false;
    }
 
    eLed_t *led = (eLed_t *) Heap_API_Calloc(1, sizeof(eLed_t));
    *led = led_int;

    if (*led == 0) {
        DEBUG_INFO("Number only, please enter valid pin number!\r\n");
        Heap_API_Free(led);
        return false;
    }

    bool function_status = false;

    switch (*led) {
        case eLed_Status: {
            function_status = (pin_status == eLedState_On ? LED_APP_AddTask(eLedAction_On, led)
                                                          : LED_APP_AddTask(eLedAction_Off, led));
        } break;
        case eLed_GpsFix: {
            function_status = (pin_status == eLedState_On ? LED_APP_AddTask(eLedAction_On, led)
                                                          : LED_APP_AddTask(eLedAction_Off, led));
        } break;
        default: {
            handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str,
                                                            COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1,
                                                            "Incorrect led state!\r\n");
            Heap_API_Free(led);
            return false;
        }
    }

    if (function_status == true) {
        handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str,
                                                        COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1,
                                                        pin_status == eLedState_On ?
                                                        "Led state was successfully set!\r\n" :
                                                        "Led state was successfully reset!\r\n");
    } else {
        handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str,
                                                        COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1,
                                                        "Command execution failed!\r\n");
    }

    return true;
}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool CLI_CMD_SetLed (sCommandHandlerArgs_t *handler_args) {
    return CLI_CMD_ExecuteLedCommand(handler_args, eLedState_On);
}

bool CLI_CMD_ResetLed (sCommandHandlerArgs_t *handler_args) {
    return CLI_CMD_ExecuteLedCommand(handler_args, eLedState_Off);
}

bool CLI_CMD_BlinkLed (sCommandHandlerArgs_t *handler_args) {
    if ((handler_args->cmd_args.str == NULL) || (handler_args->cmd_args.size == 0)) {
        DEBUG_INFO("No command arguments entered, please enter valid command arguments!\r\n");
        return false;
    }

    if (handler_args->cmd_args.str[handler_args->cmd_args.size] != '\0') {
        DEBUG_ERROR("Command message doesn't have a null terminator!\r\n");
        return false;
    }

    int led_int;
    if (MODEM_CMD_GetArgInt(&led_int, &handler_args->cmd_args.str) == false) {
        handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str, 
                                                        COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1, 
                                                        REPLY_INCORRECT_ARG_MESSAGE);
        return false;
    }

    int frequency;
    if (MODEM_CMD_GetArgInt(&frequency, &handler_args->cmd_args.str) == false) {
        handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str, 
                                                        COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1, 
                                                        REPLY_INCORRECT_ARG_MESSAGE);
        return false;
    }

    int period_s;
    if (MODEM_CMD_GetArgInt(&period_s, &handler_args->cmd_args.str) == false) {
        handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str, 
                                                        COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1, 
                                                        REPLY_INCORRECT_ARG_MESSAGE);
        return false;
    }

    sLedActionArgs_t *led_action_args = (sLedActionArgs_t *) Heap_API_Calloc(1, sizeof(sLedActionArgs_t));

    if (led_action_args == NULL) {
        DEBUG_ERROR("Failed to allocate space for LED function arguments!\r\n");
        return false;
    }

    led_action_args->led = led_int;
    led_action_args->blinks_num = period_s;
    led_action_args->blink_freq = frequency;

   if ((led_action_args->led == 0) || (led_action_args->blinks_num == 0) || (led_action_args->blink_freq == 0)) {
       DEBUG_INFO("One or more LED blinking function arguments are 0's!\r\n");
       Heap_API_Free(led_action_args);
       return false;
   }

    bool function_status = false;

    switch (led_action_args->led) {
        case eLed_Status: {
            function_status = LED_APP_AddTask(eLedAction_Blink, led_action_args);
        } break;
        case eLed_GpsFix: {
            function_status = LED_APP_AddTask(eLedAction_Blink, led_action_args);
        } break;
        default: {
            handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str,
                                                           COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1,
                                                           "Incorrect led state!\r\n");
            Heap_API_Free(led_action_args);
            return false;
        }
    }

    if (function_status == true) {
        handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str, 
                                                        COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1, 
                                                        "Led blinked successfully!\r\n");
    } else {
        handler_args->response_buffer->count = snprintf(handler_args->response_buffer->str, 
                                                        COMMAND_EXECUTION_RESPONSE_BUFFER_SIZE + 1, 
                                                        "Command execution failed!\r\n");
    }

    return true;
}

bool CLI_CMD_TcpOpen (sCommandHandlerArgs_t *handler_args) {
    if ((handler_args->cmd_args.str == NULL) || (handler_args->cmd_args.size == 0)) {
        DEBUG_INFO("No command arguments entered, please enter valid command arguments!\r\n");
        return false;
    }

    if (handler_args->cmd_args.str[handler_args->cmd_args.size] != '\0') {
        DEBUG_ERROR("Command message doesn't have a null terminator!\r\n");
        return false;
    }

    int socket_id;
    if (MODEM_CMD_GetArgInt(&socket_id, &handler_args->cmd_args.str) == false) {
        DEBUG_INFO("%s", REPLY_INCORRECT_ARG_MESSAGE);
        return false;
    }

    if ((socket_id < eServerId_First) || (socket_id >= eServerId_Last)) {
        DEBUG_INFO("Scoket ID is out of range, the range: 0 to 10!\r\n");
        return false;
    }

    char *ip_address = strtok_r(NULL, ARGUMENTS_SEPERATOR, &handler_args->cmd_args.str);

    if (ip_address == NULL) {
        DEBUG_ERROR("Failed to retrieve server IP address!\r\n");
        return false;
    }

    int port;
    if (MODEM_CMD_GetArgInt(&port, &handler_args->cmd_args.str) == false) {
        DEBUG_INFO("%s", REPLY_INCORRECT_ARG_MESSAGE);
        return false;
    }

    if ((port < MIN_PORT) || (port > MAX_PORT)) {
        DEBUG_ERROR("Incorrect server and port number, available port are from 0 to 65536!\r\n");
        return false;
    } 

    sTcpConnectJob_t *connect_params = (sTcpConnectJob_t *) Heap_API_Calloc(1, sizeof(sTcpConnectJob_t));

    if (connect_params == NULL) {
        DEBUG_ERROR("Failed to allocate space for connection to server function arguments!\r\n");
        return false;
    }

    connect_params->connect_id = (eServerId_t) socket_id;
    connect_params->port = port;
    snprintf(connect_params->ip_address, sizeof(connect_params->ip_address), "%s", ip_address);

    if (strlen(connect_params->ip_address) > MAX_SIZE_OF_IPV4_ADDRESS) {
        DEBUG_ERROR("Failed to pass server IP address!");
        Heap_API_Free(connect_params);
        return false;
    }

    sTcpJobMessage_t tcp_job = {.type = eTcpJob_Connect, .data = connect_params};

    if (TCP_APP_AddTask(&tcp_job) == false) {
        DEBUG_INFO("Failed to connect to the server!\r\n");
        Heap_API_Free(connect_params);
        return false;
    }

    return true;
}

bool CLI_CMD_TcpSend (sCommandHandlerArgs_t *handler_args) {
    if ((handler_args->cmd_args.str == NULL) || (handler_args->cmd_args.size == 0)) {
        DEBUG_INFO("No command arguments entered, please enter valid command arguments!\r\n");
        return false;
    }

    if (handler_args->cmd_args.str[handler_args->cmd_args.size] != '\0') {
        DEBUG_INFO("Command message does not contain a NULL terminator!\r\n");
        return false;
    }

    int socket_id;
    if (MODEM_CMD_GetArgInt(&socket_id, &handler_args->cmd_args.str) == false) {
        DEBUG_INFO("%s", REPLY_INCORRECT_ARG_MESSAGE);
        return false;
    }

    if ((socket_id < eServerId_First) || (socket_id >= eServerId_Last)) {
        DEBUG_INFO("Scoket ID is out of range, the range: 0 to 10!\r\n");
        return false;
    }

    char *data = strtok_r(NULL, ARGUMENTS_SEPERATOR, &handler_args->cmd_args.str);

    if (data == NULL) {
        DEBUG_INFO("Failed to tokenize server data!\r\n");
        return false;
    }

    sTcpSendJob_t *send_params = (sTcpSendJob_t *) Heap_API_Calloc(1, sizeof(sTcpSendJob_t));

    if (send_params == NULL) {
        DEBUG_ERROR("Failed to allocate memory for the data to be sent to the server.\r\n");
        return false;
    }

    send_params->connect_id = socket_id;
    send_params->data_size = strlen(data) + 4;  // 4 additional characters: '\r', '\n', '\032', '\0'
    send_params->data_str = Heap_API_Calloc(1, send_params->data_size);

    snprintf(send_params->data_str, send_params->data_size, "%s%s%c", data, DELIMITER, COMMAND_END_SYMBOL);

    sTcpJobMessage_t tcp_job = {.type = eTcpJob_Send, .data = send_params};

    if (TCP_APP_AddTask(&tcp_job) == false) {
        DEBUG_INFO("Failed to execute TCP task!\r\n");
        Heap_API_Free(send_params);
        return false;
    }

    return true;
}

bool CLI_CMD_TcpClose (sCommandHandlerArgs_t *handler_args) {
    if ((handler_args->cmd_args.str == NULL) || (handler_args->cmd_args.size == 0)) {
        DEBUG_INFO("No command arguments entered, please enter valid command arguments!\r\n");
        return false;
    }

    if (handler_args->cmd_args.str[handler_args->cmd_args.size] != '\0') {
        DEBUG_INFO("Command message does not contain a NULL terminator!\r\n");
        return false;
    }

    int socket_id = atoi(handler_args->cmd_args.str);
    if ((socket_id < eServerId_First) || (socket_id >= eServerId_Last)) {
        DEBUG_INFO("Scoket ID is out of range, the range: 0 to 10!\r\n");
        return false;
    }

    sTcpDisconnectJob_t *disconnect_params = (sTcpDisconnectJob_t *) Heap_API_Calloc(1, sizeof(sTcpDisconnectJob_t));

    if (disconnect_params == NULL) {
        DEBUG_INFO("Failed to allocate space for TCP disconnect parametrs!\r\n");
        return false;
    }

    disconnect_params->connect_id = socket_id;
    sTcpJobMessage_t tcp_job = {.type = eTcpJob_Disconnect, .data = disconnect_params};

    if (TCP_APP_AddTask(&tcp_job) == false) {
        DEBUG_INFO("Failed to disconnect from the servers!\r\n");
        Heap_API_Free(tcp_job.data);
        return false;
    }
    
    return true;
}
