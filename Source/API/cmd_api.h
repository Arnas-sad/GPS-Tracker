#ifndef SOURCE_API_CMD_API_H_
#define SOURCE_API_CMD_API_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "message.h"
#include "buffer.h"
/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/
typedef struct sCommandHandlerArgs {
    sString_t cmd_args;
    sBuffer_t *response_buffer;
} sCommandHandlerArgs_t;

typedef struct sCommandDescription {
    bool (*command_function)(sCommandHandlerArgs_t *cmd_args);
    char *command_name;
    size_t command_name_size;
} sCommandDescription_t;

typedef struct sCommandLauncherArgs {
    const sCommandDescription_t *commands_table;
    size_t commands_table_size;
    sBuffer_t *response_buffer;
} sCommandLauncherArgs_t;
/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/
bool CMD_API_Launcher (sString_t user_input, sCommandLauncherArgs_t *launcher_params);
#endif /* SOURCE_API_CMD_API_H_ */
