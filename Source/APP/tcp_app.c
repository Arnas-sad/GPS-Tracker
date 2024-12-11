/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "debug_api.h"
#include "heap_api.h"
#include "tcp_api.h"
#include "tcp_app.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define MSG_COUNT 55
#define MSG_SIZE sizeof(sTcpJobMessage_t)
#define TCP_TASK_MSG_QUEUE_ATTR_NAME "TcpTaskMessage"
#define MSG_PRIORITY 0
#define MSG_QUEUE_PUT_TIMEOUT_MS 30
#define MSG_QUEUE_GET_TIMEOUT_MS 20
#define AVAILABLE_SOCKET_BUFFER_SIZE 70
#define SOCKET_ID_SIZE 5
#define TCP_JOB_HANDLE_TASK_ATTR_NAME "TcpJobHandleTask"
#define TCP_JOB_HANDLE_TASK_STACK_SIZE 512U
#define TCP_JOB_HANDLE_TASK_ARGS NULL
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
 
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
CREATE_MODULE_TAG(TCP_APP);
static const osMessageQueueAttr_t g_tcp_task_msg_queue_attr = {
    .name = TCP_TASK_MSG_QUEUE_ATTR_NAME
};
static const osThreadAttr_t g_tcp_job_handle_task_attr = {
    .name = TCP_JOB_HANDLE_TASK_ATTR_NAME,
    .stack_size = TCP_JOB_HANDLE_TASK_STACK_SIZE,
    .priority = 25
};
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
static sSocketProperties_t g_socket [eServerId_Last];
static osMessageQueueId_t g_tcp_task_msg_queue_id = NULL;
static osThreadId_t g_tcp_job_handle_task_id = NULL;
static sTcpConnectJob_t g_tcp_connect;
static sTcpSendJob_t g_tcp_send;
static sTcpDisconnectJob_t g_tcp_close;
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
void TCP_APP_JobHandler (void *args);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
void TCP_APP_JobHandler (void *args) {
	sTcpJobMessage_t tcp_job;

    while (1) {
        if (osMessageQueueGet(g_tcp_task_msg_queue_id, &tcp_job, MSG_PRIORITY, MSG_QUEUE_GET_TIMEOUT_MS) != osOK) {
            osThreadYield();
            continue;
        }

        char socket_str[AVAILABLE_SOCKET_BUFFER_SIZE] = {0};
        size_t count = 0;

        switch (tcp_job.type) {
            case eTcpJob_Connect: {
                g_tcp_connect = *((sTcpConnectJob_t *) tcp_job.data);
                Heap_API_Free(tcp_job.data);

                if (g_socket[g_tcp_connect.connect_id].is_socket_free == false) {
                    bool free_sockets = true;
                    TCP_ADD_ReturnFreeSockets(socket_str, count, free_sockets);
                    DEBUG_INFO("Connection with this socket is currently running. %s\r\n", socket_str);
                    continue;
                }

                if (TCP_API_Connect(g_tcp_connect.connect_id, g_tcp_connect.ip_address, g_tcp_connect.port) == eModemError_ATSuccess) {
                    g_socket[g_tcp_connect.connect_id].is_socket_free = false;
                }

                continue;
            }
            case eTcpJob_Send: {
                g_tcp_send = *((sTcpSendJob_t*) tcp_job.data);
                Heap_API_Free(tcp_job.data);

                if (g_socket[g_tcp_connect.connect_id].is_socket_free == true) {
                    Heap_API_Free(g_tcp_send.data_str);
                    bool busy_sockets = false;
                    TCP_ADD_ReturnFreeSockets(socket_str, count, busy_sockets);
                    DEBUG_INFO("No previous connection was initiated with socket id: %d. %s\r\n", g_tcp_send.connect_id, socket_str);
                    continue;
                }

                TCP_API_Send(g_tcp_send.connect_id, g_tcp_send.data_str, g_tcp_send.data_size);
                continue;
            }
            case eTcpJob_Disconnect: {
                g_tcp_close = *((sTcpDisconnectJob_t*) tcp_job.data);
                Heap_API_Free(tcp_job.data);

                if (g_socket[g_tcp_connect.connect_id].is_socket_free == true) {
                    bool busy_sockets = false;
                    TCP_ADD_ReturnFreeSockets(socket_str, count, busy_sockets);
                    DEBUG_INFO("No previous connection was initiated with socket id: %d. %s\r\n", g_tcp_send.connect_id, socket_str);
                    continue;
                }

                if (TCP_API_Disconnect(g_tcp_close.connect_id) == eModemError_ATSuccess) {
                    g_socket[g_tcp_close.connect_id].is_socket_free = true;
                    DEBUG_INFO("Device has disconnect from the server!\r\n");
                } else {
                    DEBUG_INFO("Failed to close the connection!\r\n");
                }

                continue;
            }
            default: {
                DEBUG_WARN("Uknown TCP job type!\r\n");
                continue;
            }
        }
    }
}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool TCP_APP_Init (void) {
    for (eServerId_t srv_id = eServerId_First; srv_id < eServerId_Last; srv_id++) {
        g_socket[srv_id].is_socket_free = true;
    }

    if (g_tcp_task_msg_queue_id == NULL) {
        g_tcp_task_msg_queue_id = osMessageQueueNew(MSG_COUNT, MSG_SIZE, &g_tcp_task_msg_queue_attr);
        if (g_tcp_task_msg_queue_id == NULL) {
            DEBUG_ERROR("Failed to create a message queue for TCP tasks!\r\n");
            return false;
        }
    }

    if (g_tcp_job_handle_task_id == NULL) {
        g_tcp_job_handle_task_id = osThreadNew(TCP_APP_JobHandler, TCP_JOB_HANDLE_TASK_ARGS, &g_tcp_job_handle_task_attr);
        if (g_tcp_job_handle_task_id == NULL) {
            DEBUG_ERROR("Failed to create a Thread for tcp job handling!\r\n");
            return false;
        }
    }

    return true;
}

void TCP_ADD_ReturnFreeSockets(char *sockets, size_t count, bool socket_state) { 
    int socket_count = 0;
    if (socket_state) {
        count += snprintf(sockets, AVAILABLE_SOCKET_BUFFER_SIZE, "These are the available socket(s): ");
    } else {
        count += snprintf(sockets, AVAILABLE_SOCKET_BUFFER_SIZE, "These are in-use socket(s): ");
    }

    for (eServerId_t srv_id = eServerId_First; srv_id < eServerId_Last; srv_id++) {
        if (g_socket[srv_id].is_socket_free == socket_state) {
            count += snprintf(&sockets[count], AVAILABLE_SOCKET_BUFFER_SIZE, "%d, ", srv_id);
        } else {
            socket_count++;
        }
    }

    if (count > AVAILABLE_SOCKET_BUFFER_SIZE) {
        DEBUG_ERROR("Buffer overflow occured, too many characters were written to the socket buffer size!\r\n");
        return;
    }

    if (socket_count == eServerId_Last) {
        if (socket_state) {
            snprintf(sockets, AVAILABLE_SOCKET_BUFFER_SIZE, "All sockets are in use!");
        } else {
            snprintf(sockets, AVAILABLE_SOCKET_BUFFER_SIZE, "No sockets are currently in use!");
        }
    } else {
        sockets[count - 2] = '!';
    }

    return;
}

bool TCP_APP_AddTask (sTcpJobMessage_t *tcp_job) {
    if (tcp_job == NULL) {
        DEBUG_ERROR("TCP task arguments are invalid!\r\n");
        return false;
    }

    if (osMessageQueuePut(g_tcp_task_msg_queue_id, tcp_job, MSG_PRIORITY, MSG_QUEUE_PUT_TIMEOUT_MS) != osOK) {
        DEBUG_ERROR("Failed to queue the TCP task request!\r\n");
        return false;
    }

    return true;
}
