#ifndef SOURCE_API_MODEM_API_COMMANDS_H_
#define SOURCE_API_MODEM_API_COMMANDS_H_
/**********************************************************************************************************************
* Includes
*********************************************************************************************************************/
#include <stdbool.h>
#include "cmd_api.h"
/**********************************************************************************************************************
* Exported definitions and macros
*********************************************************************************************************************/

/**********************************************************************************************************************
* Exported types
*********************************************************************************************************************/

/**********************************************************************************************************************
* Exported variables
*********************************************************************************************************************/

/**********************************************************************************************************************
* Prototypes of exported functions
*********************************************************************************************************************/
bool Modem_API_CMD_OK (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_DisableEcho (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_Error (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_GetError (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_NetworkRegStatus (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_AddressPDP (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_QIOPEN (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_ReadyToSend (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_CMD_SendOk (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_SendFail (sCommandHandlerArgs_t *modem_handler_args);
bool Modem_API_CMD_QIURC (sCommandHandlerArgs_t *modem_handler_args);
#endif /* SOURCE_API_MODEM_API_COMMANDS_H_ */
