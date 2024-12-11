/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
#include <cmsis_os2.h>
#include <string.h>
#include "message.h"
#include "cmd_api.h"
#include "debug_api.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
 
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
static bool CMD_API_FindAndRunCommand (sCommandLauncherArgs_t *launcher_params, sString_t user_input);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
static bool CMD_API_FindAndRunCommand (sCommandLauncherArgs_t *launcher_params, sString_t user_input) {
    sCommandHandlerArgs_t handler_args = {
        .cmd_args = {0},
        .response_buffer = launcher_params->response_buffer
    };

    for (size_t i = 0; i < launcher_params->commands_table_size; i++) {
        if (strncmp(user_input.str, launcher_params->commands_table[i].command_name, 
                    launcher_params->commands_table[i].command_name_size) == 0) {

            size_t cmd_size = launcher_params->commands_table[i].command_name_size;
            handler_args.cmd_args.str = user_input.str + cmd_size;
            handler_args.cmd_args.size = user_input.size - cmd_size;

            launcher_params->commands_table[i].command_function(&handler_args);

            return true;
        }
    }
    return false;
}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool CMD_API_Launcher(sString_t user_input, sCommandLauncherArgs_t *launcher_params) {
    if ((user_input.str == NULL) || (user_input.size == 0)) {
        launcher_params->response_buffer->count = snprintf(launcher_params->response_buffer->str,
                                                           launcher_params->response_buffer->size + 1,
                                                           "Empty command was received!\r\n");

        return false;
    }

    if ((launcher_params->commands_table == NULL) || (launcher_params->commands_table_size == 0)) {
        launcher_params->response_buffer->count = snprintf(launcher_params->response_buffer->str,
                                                           launcher_params->response_buffer->size + 1,
                                                           "Command launcher parameters are not set correctly!\r\n");

        return false;
    }

    if (CMD_API_FindAndRunCommand(launcher_params, user_input) == false) {
        launcher_params->response_buffer->count = snprintf(launcher_params->response_buffer->str,
                                                           launcher_params->response_buffer->size + 1,
                                                           "Command does not exist!\r\n");

        return false;
    }

    return true;
}
