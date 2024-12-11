/**********************************************************************************************************************
* Includes
*********************************************************************************************************************/
#include <stdbool.h>
#include <cmsis_os2.h>
#include <string.h>
#include "string_util.h"
#include "uart_api.h"
#include "debug_api.h"
#include "heap_api.h"
#include "cli_commands.h"
/**********************************************************************************************************************
* Private definitions and macros
*********************************************************************************************************************/
#define CLI_UART_BAUDRATE 115200
#define CLI_APP_COMMAND_PARSE_MUTEX_ATTR_NAME "CLIAppMutex"
#define CLI_APP_COMMAND_PARSE_TASK_ATTR_NAME "CLIAppTask"
#define CLI_APP_TASK_STACK_SIZE 1024
#define RECEIVE_COMMAND_TIMEOUT_MS osWaitForever
#define CLI_RESPONSE_BUFFER_SIZE 160
#define DEFINE_DELIM() ((sString_t) DEFINE_STRING("\r\n"))
#define CMD(name) .command_name = name, .command_name_size = sizeof(name) - 1
#define TABLE_SIZE 6
#define NONE_THREAD_ARGUMENTS NULL
#define UART eUartApiDevice_Debug
/**********************************************************************************************************************
* Private typedef
*********************************************************************************************************************/

/**********************************************************************************************************************
* Private constants
*********************************************************************************************************************/
CREATE_MODULE_TAG (CLI_APP);
const static osThreadAttr_t g_cli_app_task_attr = {
    .name = CLI_APP_COMMAND_PARSE_TASK_ATTR_NAME,
    .priority = 25,
    .stack_size = CLI_APP_TASK_STACK_SIZE
};
static const sCommandDescription_t g_callback_function_command_lut[TABLE_SIZE] = {
    {.command_function = &CLI_CMD_SetLed, CMD("set:")},
    {.command_function = &CLI_CMD_ResetLed, CMD("reset:")},
    {.command_function = &CLI_CMD_BlinkLed, CMD("blink:")},
    {.command_function = &CLI_CMD_TcpOpen, CMD("connect:")},
    {.command_function = &CLI_CMD_TcpSend, CMD("send:")},
    {.command_function = &CLI_CMD_TcpClose, CMD("disconnect:")}
};
/**********************************************************************************************************************
* Private variables
*********************************************************************************************************************/
static char g_command_reply_buffer[CLI_RESPONSE_BUFFER_SIZE] = {0};
static osThreadId_t g_cli_app_task_id = NULL;
static sBuffer_t g_response_buffer = {.str = g_command_reply_buffer, .size = CLI_RESPONSE_BUFFER_SIZE, .count = 0};
static const sCommandLauncherArgs_t g_cmd_launcher_params = {
    .commands_table = g_callback_function_command_lut,
    .commands_table_size = TABLE_SIZE,
    .response_buffer = &g_response_buffer
};
/**********************************************************************************************************************
* Exported variables and references
*********************************************************************************************************************/

/**********************************************************************************************************************
* Prototypes of private functions
*********************************************************************************************************************/
static void CLI_CommandHandlerTask (void *arg);
/**********************************************************************************************************************
* Definitions of private functions
*********************************************************************************************************************/
static void CLI_CommandHandlerTask (void *arg) {
    sString_t user_input;

    while (1) {  
        if (UART_API_GetMessage(UART, &user_input, RECEIVE_COMMAND_TIMEOUT_MS) == false) {
            DEBUG_ERROR("Message not received, internal failure!\r\n");
        }

        if (CMD_API_Launcher(user_input, &g_cmd_launcher_params) == false) {
            DEBUG_WARN("%s\r\n", g_response_buffer);
        }
        else {
            DEBUG_INFO("%s\r\n", g_response_buffer);
        }

        Heap_API_Free(user_input.str);
    }
}
/**********************************************************************************************************************
* Definitions of exported functions
*********************************************************************************************************************/
bool CLI_APP_Init (void) {
    if (g_cli_app_task_id == NULL) {
        g_cli_app_task_id = osThreadNew(CLI_CommandHandlerTask, NONE_THREAD_ARGUMENTS, &g_cli_app_task_attr);

        if (g_cli_app_task_id == NULL) {
            return false;
        }
    }

    return true;
}
