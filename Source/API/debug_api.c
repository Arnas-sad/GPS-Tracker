/**********************************************************************************************************************
* Includes
*********************************************************************************************************************/
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "uart_driver.h"
#include "uart_api.h"
#include "debug_api.h"
#include "string_util.h"
/**********************************************************************************************************************
* Private definitions and macros
*********************************************************************************************************************/
#define DEBUG_MUTEX_ACQUIRE_TIMEOUT 100
#define DEBUG_UART_SPEED 115200
#define DEBUG_BUFFER_SIZE 1024
#define CFG_DEBUG_API_SEND_MUTEX_NAME "DebugApiMutex"
#define UART_DEBUGGING eUartApiDevice_Debug
/**********************************************************************************************************************
* Private typedef
*********************************************************************************************************************/

/**********************************************************************************************************************
* Private constants
*********************************************************************************************************************/
static const osMutexAttr_t g_send_mutex_attr = {.name = CFG_DEBUG_API_SEND_MUTEX_NAME};
/**********************************************************************************************************************
* Private variables
*********************************************************************************************************************/
static char g_debug_message_buffer[DEBUG_BUFFER_SIZE] = {0};
static osMutexId_t g_print_mutex = NULL;
/**********************************************************************************************************************
* Exported variables and references
*********************************************************************************************************************/

/**********************************************************************************************************************
* Prototypes of private functions
*********************************************************************************************************************/

/**********************************************************************************************************************
* Definitions of private functions
*********************************************************************************************************************/

/**********************************************************************************************************************
* Definitions of exported functions
*********************************************************************************************************************/
bool Debug_API_Init(void) {
    sString_t delimiter = (sString_t)DEFINE_STRING("\r\n");

    if (UART_API_Init(UART_DEBUGGING, DEBUG_UART_SPEED, delimiter) == false) {
        return false;        
    }  

    g_print_mutex = osMutexNew(&g_send_mutex_attr);

    if (g_print_mutex == NULL) {
        return false;       
    }

    return true;
}

bool Debug_API_PrintMessage(const char *module_tag, const char *file, int line, eDebugLevel_t debug_level, const char *format, ...) {
    
    if (g_print_mutex == NULL) {
        return false;
    }

    if ((format == NULL) || (module_tag == NULL)) {
         return false;
    }

    if (osMutexAcquire(g_print_mutex, DEBUG_MUTEX_ACQUIRE_TIMEOUT) != osOK) {
        return false;
    }

    bool return_val = true;

    int bytes_written = 0;
    switch (debug_level) {
        case eDebugLevel_Info: {
            bytes_written = snprintf(g_debug_message_buffer, DEBUG_BUFFER_SIZE, "[%s.INF]: \t", module_tag);
        } break;
        case eDebugLevel_Warning: {
            bytes_written = snprintf(g_debug_message_buffer, DEBUG_BUFFER_SIZE, "[%s.WRN] (%s @%u line): \t", module_tag, file, line);
        } break;
        case eDebugLevel_Error: {
            bytes_written = snprintf(g_debug_message_buffer, DEBUG_BUFFER_SIZE, "[%s.ERR] (%s @%u line): \t", module_tag, file, line);
        } break;
        default: {
            break;
        }
    }

    if ((bytes_written < DEBUG_BUFFER_SIZE) && (bytes_written > 0)) {
        va_list args;
        va_start(args, format);
        bytes_written += vsnprintf(&g_debug_message_buffer[bytes_written], DEBUG_BUFFER_SIZE, format, args);   
        va_end(args);

        if (bytes_written >= DEBUG_BUFFER_SIZE) {
            return_val = false;
        }  
        
        sString_t full_message;
        full_message.str = g_debug_message_buffer;
        full_message.size = bytes_written;

        if (UART_API_SendMessage(UART_DEBUGGING, full_message) == false) {    
            return_val = false; 
        }
    }

    if (osMutexRelease(g_print_mutex) != osOK) {
        return_val = false;
    }

    return return_val;
}