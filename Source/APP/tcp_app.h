#ifndef SOURCE_API_TCP_APP_H_
#define SOURCE_API_TCP_APP_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdint.h>
#include "message.h"
/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/
typedef enum eServerId {
    eServerId_First = 0,
    eServerId_Second,
    eServerId_Third,
    eServerId_Fourth,
    eServerId_Fifth,
    eServerId_Sixth,
    eServerId_Seventh,
    eServerId_Eighth,
    eServerId_Nineth,
    eServerId_Tenth,
    eServerId_Eleventh,
    eServerId_Last
} eServerId_t;

typedef struct sTcpConnectJob {
    eServerId_t connect_id;
    char ip_address[16];
    size_t port;
} sTcpConnectJob_t;

typedef struct sSocketProperties {
    bool is_socket_free;
} sSocketProperties_t;

typedef struct sTcpSendJob {
    eServerId_t connect_id;
    char *data_str;
    size_t data_size;
} sTcpSendJob_t;

typedef struct sTcpDisconnectJob {
    eServerId_t connect_id;
} sTcpDisconnectJob_t;

typedef enum eTcpJob {
    eTcpJob_First = 0,
    eTcpJob_Connect = eTcpJob_First,
    eTcpJob_Send,
    eTcpJob_Disconnect,
    eTcpJob_Last
} eTcpJob_t;

typedef struct sTcpJobMessage {
    eTcpJob_t type;
    void *data;
} sTcpJobMessage_t;
/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/
bool TCP_APP_Init (void);
bool TCP_APP_AddTask (sTcpJobMessage_t *tcp_job_msg);
void TCP_ADD_ReturnFreeSockets (char *sockets, size_t count, bool socket_state);
#endif /* SOURCE_API_TCP_APP_H_ */
