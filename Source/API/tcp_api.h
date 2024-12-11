#ifndef SOURCE_API_TCP_API_H_
#define SOURCE_API_TCP_API_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include "message.h"
#include "error_codes.h"
#include "tcp_app.h"
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
eModemError_t TCP_API_Connect (eServerId_t connect_id, char *ip_address, size_t port);
eModemError_t TCP_API_Send (eServerId_t connect_id, char *server_data_str, size_t server_data_size);
eModemError_t TCP_API_Disconnect (eServerId_t connect_id);
#endif /* SOURCE_API_TCP_API_H_ */
