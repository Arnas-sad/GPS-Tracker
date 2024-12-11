/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "uart_driver.h"
#include "uart_api.h"
#include "message.h"    
#include "string_util.h"
#include "heap_api.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define MSG_COUNT 10
#define MESSAGE_QUEUE_PUT_MESSAGE_TIMEOUT_MS 10
#define MUTEX_TIMEOUT_MS 10
#define MODEM_MAX_MESSAGE_SIZE 1024
#define DEBUG_MAX_MESSAGE_SIZE 128
#define UART_API_COLLECTOR_TASK_NAME "UartApiTask"
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
typedef struct sUartAPIConfig_t {
    eUartDriver_t linked_periph;
    size_t max_msg_size;
} sUartAPIConfig_t; 

typedef enum {
    eState_First = 0,
    eState_Setup = eState_First,
    eState_Collect,
    eState_Flush,
    eState_Last
} eState_t;

typedef struct {
    eState_t curr_state;
    bool is_initialized;
    osMutexId_t mutex_id;
    osMessageQueueId_t msg_queue;
    sString_t rx_message;
    sString_t delimiter;
} sRuntime_t;
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
const static sUartAPIConfig_t g_config_lut[eUartApiDevice_Last] = {   
    [eUartApiDevice_Modem] = {
        .linked_periph = eUartDriver_2,
        .max_msg_size = MODEM_MAX_MESSAGE_SIZE
    },
    [eUartApiDevice_Debug] = {
        .linked_periph = eUartDriver_1,
        .max_msg_size = DEBUG_MAX_MESSAGE_SIZE
    }
};
const static osThreadAttr_t g_uart_api_collector_task_attr = {              
    .name = UART_API_COLLECTOR_TASK_NAME, 
    .stack_size = 1024U,
    .priority = 25
};
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
static sRuntime_t g_runtime_data[eUartApiDevice_Last] = {0};
static osThreadId_t g_message_collector_task_id = NULL;
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
static void UART_API_Thread (void *arg);
static inline bool UART_API_IsDelimiterFound (sString_t delim, sString_t msg);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
static void UART_API_Thread (void *arg) {
    Heap_API_Init();

    while (1) {
        for (eUartApiDevice_t uart = eUartApiDevice_First; uart < eUartApiDevice_Last; uart++) {
            if (g_runtime_data[uart].is_initialized == false) {
                continue;
            }

            switch (g_runtime_data[uart].curr_state) {
                case eState_Setup: {
                    g_runtime_data[uart].rx_message.size = 0;
                    g_runtime_data[uart].rx_message.str = (char *) Heap_API_Calloc(g_config_lut[uart].max_msg_size + 1, sizeof(char));
        
                    if (g_runtime_data[uart].rx_message.str == NULL) {
                        continue;
                    }
        
                    g_runtime_data[uart].curr_state = eState_Collect;
                }
                case eState_Collect: {
                    char byte;

                    while (UART_Driver_GetByte(g_config_lut[uart].linked_periph, (uint8_t *)&byte)) {
                        g_runtime_data[uart].rx_message.str[g_runtime_data[uart].rx_message.size++] = byte;

                        if (UART_API_IsDelimiterFound(g_runtime_data[uart].delimiter, g_runtime_data[uart].rx_message)) {

                            size_t delim_length = g_runtime_data[uart].delimiter.size;
                            g_runtime_data[uart].rx_message.str[g_runtime_data[uart].rx_message.size - delim_length] = '\0';
                            g_runtime_data[uart].rx_message.size -= delim_length;

                            if ((g_runtime_data[uart].rx_message.size == 0) || 
                                (g_runtime_data[uart].rx_message.str[0] == '\0')) {
                                break; 
                            }

                            g_runtime_data[uart].curr_state = eState_Flush;
                        }
                        else if (g_runtime_data[uart].rx_message.size >= g_config_lut[uart].max_msg_size) {
                            g_runtime_data[uart].curr_state = eState_Flush;
                        }
                    }

                    if (g_runtime_data[uart].curr_state != eState_Flush) {
                        continue;
                    }
                }
                case eState_Flush: {
                    if (osMessageQueuePut(g_runtime_data[uart].msg_queue, &g_runtime_data[uart].rx_message, 0,
                        MESSAGE_QUEUE_PUT_MESSAGE_TIMEOUT_MS) != osOK) {
                        continue;
                    }

                    g_runtime_data[uart].curr_state = eState_Setup;

                    continue;
                }
                default: {
                    break;
                }
            }
        }

        osThreadYield();
    }
}

static inline bool UART_API_IsDelimiterFound (sString_t delim, sString_t msg) {
    if ((delim.str == NULL) || (delim.size == 0) || (msg.str == NULL) || (msg.size == 0)) {
        return false;
    }

    if (delim.str[delim.size - 1] != msg.str[msg.size - 1]) {
        return false;
    }

    for (size_t i = 0; i < delim.size; i++) {
        if (delim.str[delim.size - 1 - i] != msg.str[msg.size - 1 - i]) {
            return false;   
        }
    }

    return true;                            
}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool UART_API_Init (eUartApiDevice_t uart, uint32_t baudrate, sString_t delim) {
    if ((uart >= eUartApiDevice_Last) || (baudrate == 0) || (delim.str == NULL) || (delim.size == 0)) {
        return false;
    } 

    osKernelState_t kernel_state = osKernelGetState();
    if ((kernel_state == osKernelError) || (kernel_state == osKernelInactive)) {
        return false;
    }

    if (UART_Driver_Init(g_config_lut[uart].linked_periph, baudrate) == false) {
        return false;
    }

    g_runtime_data[uart].mutex_id = osMutexNew(NULL);

    if (g_runtime_data[uart].mutex_id == NULL) {
        return false;
    }

    g_runtime_data[uart].msg_queue = osMessageQueueNew(MSG_COUNT, sizeof(sString_t), NULL); 

    if (g_runtime_data[uart].msg_queue == NULL) {
        return false;
    }

    if (g_message_collector_task_id == NULL) {
        g_message_collector_task_id = osThreadNew(UART_API_Thread, NULL, &g_uart_api_collector_task_attr);
        if (g_message_collector_task_id == NULL) {
            return false;
        }
    }

    g_runtime_data[uart].delimiter = delim;
    g_runtime_data[uart].curr_state = eState_Setup;
    g_runtime_data[uart].is_initialized = true;

    return true;
}

bool UART_API_SendMessage (eUartApiDevice_t uart, sString_t msg) {
    if ((uart >= eUartApiDevice_Last) || (msg.str == NULL) || (msg.size == 0)) {   
        return false;
    }

    if (g_runtime_data[uart].is_initialized == false) {
        return false;
    }

    if (osMutexAcquire(g_runtime_data[uart].mutex_id, MUTEX_TIMEOUT_MS) != osOK) {
        return false;
    }

    bool return_val = true;

    if (UART_Driver_SendBytes(g_config_lut[uart].linked_periph, (uint8_t *) msg.str, msg.size) == false) {
        return_val = false;
    }

    osMutexRelease(g_runtime_data[uart].mutex_id);

    return return_val;
}

bool UART_API_GetMessage (eUartApiDevice_t uart, sString_t *msg, uint32_t timeout) {
    if ((uart >= eUartApiDevice_Last) || (msg == NULL)) {
        return false;
    }

    if (g_runtime_data[uart].is_initialized == false) {
        return false;
    }

    bool return_val = true;

    if (osMessageQueueGet(g_runtime_data[uart].msg_queue, msg, 0, timeout) != osOK) {
        return_val = false;
    }

    return return_val;
}
