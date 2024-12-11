#ifndef SOURCE_APP_CLI_COMMANDS_H_
#define SOURCE_APP_CLI_COMMANDS_H_
/**********************************************************************************************************************
* Includes
*********************************************************************************************************************/
#include <stdbool.h>
#include "cmd_api.h"
#include "led_api.h"
/**********************************************************************************************************************
* Exported definitions and macros
*********************************************************************************************************************/

/**********************************************************************************************************************
* Exported types
*********************************************************************************************************************/
typedef struct sLedActionArgs {
    eLed_t led;
    size_t blink_freq;
    size_t blinks_num;
    void (*callback_func)(eLed_t led);
} sLedActionArgs_t;

typedef enum eLedAction {
    eLedAction_First = 0,
    eLedAction_On = eLedAction_First,
    eLedAction_Off,
    eLedAction_Toggle,
    eLedAction_Blink,
    eLedAction_Last
} eLedAction_t;
/**********************************************************************************************************************
* Exported variables
*********************************************************************************************************************/

/**********************************************************************************************************************
* Prototypes of exported functions
*********************************************************************************************************************/
bool CLI_CMD_SetLed (sCommandHandlerArgs_t *handler_args);
bool CLI_CMD_ResetLed (sCommandHandlerArgs_t *handler_args);
bool CLI_CMD_BlinkLed (sCommandHandlerArgs_t *handler_args);
bool CLI_CMD_TcpOpen (sCommandHandlerArgs_t *handler_args);
bool CLI_CMD_TcpSend (sCommandHandlerArgs_t *handler_args);
bool CLI_CMD_TcpClose (sCommandHandlerArgs_t *handler_args);
#endif /* SOURCE_APP_CLI_COMMANDS_H_ */
