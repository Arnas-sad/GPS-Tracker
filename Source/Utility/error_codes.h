#ifndef SOURCE_UTILITY_ERROR_CODES_H_
#define SOURCE_UTILITY_ERROR_CODES_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stddef.h>
/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/
typedef enum eModemError {
    eModemError_First,
    eModemError_ATSuccess = eModemError_First,
    eModemError_NoResponse,
    eModemError_InvalidResponse,
    eModemError_InvalidParameters,
    eModemError_InvalidState,
    eModemError_ReceptionFail,
    eModemError_SendFail,
    eModemError_MemoryAllocationFail,
    eModemError_CallbackFail,
    eModemError_ClearFlagFail,
    eModemError_WaitFlagFail,
    eModemError_FlagNotSet,
    eModemError_ResourceBusy,
    eModemError_Unknown,
    eModemError_Last
} eModemError_t;
/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

#endif /* SOURCE_UTILITY_ERROR_CODES_H_ */