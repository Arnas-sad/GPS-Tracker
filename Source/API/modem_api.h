#ifndef SOURCE_API_MODEM_API_H_
#define SOURCE_API_MODEM_API_H_
/**********************************************************************************************************************
* Includes
*********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "error_codes.h"
/**********************************************************************************************************************
* Exported definitions and macros
*********************************************************************************************************************/
typedef enum eModemCommands {
   eModemCommands_First = 0,
   eModemCommands_ATE0 = eModemCommands_First,
   eModemCommands_QICSGP,
   eModemCommands_QIACT,
   eModemCommands_CEREG,
   eModemCommands_CGPADDR,
   eModemCommands_QIGETERROR, 
   eModemCommands_QIOPEN,
   eModemCommands_QISEND,
   eModemCommands_QIURC,
   eModemCommands_QICLOSE,
   eModemCommands_Last
} eModemCommands_t;

typedef enum eModemState {
   eModemState_First = 0,
   eModemState_TurnedOff = eModemState_First,
   eModemState_TurnedOn,
   eModemState_Ready,
   eModemState_Uninitialized,
   eModemState_Initialized,
   eModemState_Last
} eModemState_t;

typedef enum eModemFlags {
   eModemFlags_First = 0,
   eModemFlags_Ready = eModemFlags_First,
   eModemFlags_ResponseOK,
   eModemFlags_EchoDisabled,
   eModemFlags_Error,
   eModemFlags_Registered,
   eModemFlags_ValidPDPAddress,
   eModemFlags_ServerOpen,
   // eModemFlags_ReadyToSend,
   eModemFlags_SendOK,
   eModemFlags_SendFail,
   eModemFlags_DataReceived,
   eModemFlag_Last
} eModemFlags_t;
/**********************************************************************************************************************
* Exported types
*********************************************************************************************************************/

/**********************************************************************************************************************
* Exported variables
*********************************************************************************************************************/

/**********************************************************************************************************************
* Prototypes of exported functions
*********************************************************************************************************************/
bool Modem_API_Init (void);
bool Modem_API_SetFlag (eModemFlags_t flag_to_set);
bool Modem_API_IsFlagSet (eModemFlags_t flag_to_check);
bool Modem_API_ClearFlag (eModemFlags_t flag_to_clear);
eModemError_t Modem_API_SendCommand (eModemCommands_t g_modem_AT_commands, eModemFlags_t flag, char *cmd_params_string, size_t cmd_params_size);
eModemState_t Modem_API_GetState(void);
bool Modem_API_LockModem (uint32_t timeout);
bool Modem_API_UnlockModem (void);
#endif /* SOURCE_API_MODEM_API_H_ */
