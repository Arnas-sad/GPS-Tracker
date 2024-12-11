/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "FreeRTOSConfig.h"
#include "cmsis_os2.h"
#include "debug_api.h"
#include "heap_api.h"
#include "led_api.h"
#include "led_app.h"
#include "cli_commands.h"
#include "cli_app.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define COMMAND_HANDLER_TASK_NAME "HandlerTask"
#define COMMAND_HANDLER_MESSAGE_QUEUE_NAME "FunctionRequestQueue"
#define LED_MESSAGE_QUEUE_NAME "LedMessageQueue"
#define COMMAND_HANDLER_TASK_STACK_SIZE 512U
#define NONE_THREAD_ARGUMENTS NULL
#define MSG_COUNT 32
#define MSG_SIZE sizeof(sLedFunctionRequest_t *)
#define MSG_QUEUE_PRIORITY 0
#define MSG_QUEUE_TIMEOUT_MS 10
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
typedef struct sLedFunctionRequest {
    void *func_args;
    eLedAction_t func_request_type;
} sLedFunctionRequest_t;

typedef enum eTaskHandlerState {
    eTaskHandlerState_First = 0,
    eTaskHandlerState_Collect = eTaskHandlerState_First,
    eTaskHandlerState_Put,
    eTaskHandlerState_Execute,
    eTaskHandlerState_Last
} eTaskHandlerState_t;

typedef struct sTaskRuntimeData {
    osMessageQueueId_t msg_queue_id;
    bool is_led_ready;
} sTaskRuntimeData_t;
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
CREATE_MODULE_TAG (LED_APP);
static const osThreadAttr_t g_command_handler_task_attr = {
    .name = COMMAND_HANDLER_TASK_NAME,
    .stack_size = COMMAND_HANDLER_TASK_STACK_SIZE,
    .priority = 25
};
static const osMessageQueueAttr_t g_job_collector_msg_queue_attr = {
    .name = COMMAND_HANDLER_MESSAGE_QUEUE_NAME
};
static const osMessageQueueAttr_t g_led_msg_queue_attr = {
    .name = LED_MESSAGE_QUEUE_NAME
};
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
static osThreadId_t g_command_handler_task_id = NULL;
static osMessageQueueId_t g_main_msg_queue_id = NULL;
static eTaskHandlerState_t g_curr_state = eTaskHandlerState_Collect;
static sTaskRuntimeData_t g_run_data[eLed_Last] = {0};
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
static void LED_APP_RequestHandler (void *args);
eLed_t LED_APP_GetLed (sLedFunctionRequest_t *function_request);
bool LED_APP_ExecuteLedRequest (sLedFunctionRequest_t *function_request);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
static void LED_APP_RequestHandler (void *args) {
    sLedFunctionRequest_t *function_request;
    eLed_t led;

    while (1) {
        switch (g_curr_state) {
            case eTaskHandlerState_Collect: {
                for (led = eLed_First; led < eLed_Last; led++) {
                    if ((g_run_data[led].is_led_ready == true) && 
                        (osMessageQueueGet(g_run_data[led].msg_queue_id, &function_request, 
                                           MSG_QUEUE_PRIORITY, 0) == osOK)) {
                        g_curr_state = eTaskHandlerState_Execute;
                        break;
                    }
                }

                if (osMessageQueueGet(g_main_msg_queue_id, &function_request, 
                                      MSG_QUEUE_PRIORITY, 10) != osOK) {
                    continue;
                }

                if ((function_request->func_args == NULL) || (function_request->func_request_type < eLedAction_First) || 
                    (function_request->func_request_type >= eLedAction_Last)) {
                    DEBUG_ERROR("Function parameter or type is invalid!\r\n");
                    continue;
                }
                
                led = LED_APP_GetLed(function_request);
                if (g_run_data[led].is_led_ready == true) {
                    g_curr_state = eTaskHandlerState_Execute;
                    continue;
                }

                g_curr_state = eTaskHandlerState_Put;
            }
            case eTaskHandlerState_Put: {
                if ((led >= eLed_First) && (led < eLed_Last)) {
                    if (osMessageQueuePut(g_run_data[led].msg_queue_id, &function_request, 
                                          MSG_QUEUE_PRIORITY, MSG_QUEUE_TIMEOUT_MS) != osOK) {
                        DEBUG_ERROR("Failed to put the LED function into a queue!\r\n");
                    }
                }    
                else {
                    DEBUG_ERROR("Failed to get the LED's number!");
                    break;
                }

                g_curr_state = eTaskHandlerState_Execute;
            }
            case eTaskHandlerState_Execute: {
                if (g_run_data[led].is_led_ready == true) {
                    if (LED_APP_ExecuteLedRequest(function_request) == true) {
                        Heap_API_Free(function_request->func_args);
                        Heap_API_Free(function_request);
                    }
                }

                g_curr_state = eTaskHandlerState_Collect;
                continue;
            }
            default: {
                g_curr_state = eTaskHandlerState_Collect;
                break;
            }
        }
    }
}

eLed_t LED_APP_GetLed (sLedFunctionRequest_t *function_request) {
    if (function_request == NULL) {
        DEBUG_ERROR("Function pointer is not valid: %p", function_request);
        return 0;
    }

    switch (function_request->func_request_type) {
        case eLedAction_On:
        case eLedAction_Off: {
            eLed_t led = *((eLed_t *) function_request->func_args);
            return led;
        } break;
        case eLedAction_Blink: {
            sLedActionArgs_t blink_func_args;
            blink_func_args = *((sLedActionArgs_t *) function_request->func_args);
            return blink_func_args.led;
        } break;
        default: {
            DEBUG_ERROR("Function request does no contain any of the possible LED functions!");
            return 0;
        }
    }
}

bool LED_APP_ExecuteLedRequest (sLedFunctionRequest_t *function_request) {
    switch (function_request->func_request_type) {
        case eLedAction_On: {
            eLed_t led = *((eLed_t *) function_request->func_args);
            if (LED_API_LedOn(led) == false) {
                DEBUG_INFO("Failed to set the LED!\r\n");
                return false;
            }
        } break;
        case eLedAction_Off: {
            eLed_t led = *((eLed_t *) function_request->func_args);
            if (LED_API_LedOff(led) == false) {
                DEBUG_INFO("Failed to reset the LED!\r\n");
                return false;
            }
        } break;
        case eLedAction_Blink: {
            sLedActionArgs_t blink_func_args;
            blink_func_args = *((sLedActionArgs_t *) function_request->func_args);
            if (LED_API_LedBlink(blink_func_args.led, blink_func_args.blink_freq,
                                 blink_func_args.blinks_num, &LED_APP_BlinkCompletedCallback) == false) {
                DEBUG_INFO("Failed to blink the LED!\r\n");
                return false;
            }
            else {
                g_run_data[blink_func_args.led].is_led_ready = false;
            }
        } break;
        default: {
            DEBUG_WARN("Failed to retrieve function task!\r\n");
            Heap_API_Free(function_request->func_args);
            Heap_API_Free(function_request);
            return false;
        }
    }

    return true;
}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool LED_APP_Init (void) {
    if (g_command_handler_task_id == NULL) {
        g_command_handler_task_id = osThreadNew(&LED_APP_RequestHandler, NONE_THREAD_ARGUMENTS, 
        &g_command_handler_task_attr);
        if (g_command_handler_task_id == NULL) {
            DEBUG_ERROR("Request Handler Thread creation failed!\r\n");
            return false;
        }
    }

    if (g_main_msg_queue_id == NULL) {
        g_main_msg_queue_id = osMessageQueueNew(MSG_COUNT, MSG_SIZE, &g_job_collector_msg_queue_attr);
        if (g_main_msg_queue_id == NULL) {
            DEBUG_ERROR("Failed to create message queue!\r\n");
            return false;
        }
    }

    for (eLed_t led = eLed_First; led < eLed_Last; led++) {
        if (g_run_data[led].msg_queue_id == NULL) {
            g_run_data[led].msg_queue_id = osMessageQueueNew(MSG_COUNT, MSG_SIZE, &g_led_msg_queue_attr);
            if (g_run_data[led].msg_queue_id == NULL) {
                DEBUG_ERROR("Failed to create message queue for LEDs!\r\n");
                return false;
            }
        }

        g_run_data[led].is_led_ready = true;
    }

    g_curr_state = eTaskHandlerState_Collect;

    return true;
}

bool LED_APP_AddTask (eLedAction_t led_action_type, void *led_action_args) {
    if ((led_action_type < eLedAction_First) || (led_action_type >= eLedAction_Last) || (led_action_args == NULL)) {
        DEBUG_ERROR("Incorrect LED function type selected: %d or invalid function arguments are passed: %p!\r\n",
                    led_action_type, led_action_args);
        return false;
    }

    sLedFunctionRequest_t *func_req_ptr = (sLedFunctionRequest_t *) Heap_API_Calloc(1, sizeof(sLedFunctionRequest_t));

    if (func_req_ptr == NULL) {
        DEBUG_ERROR("Memory allocation failed for the LED request function!\r\n");
        return false;
    }

    func_req_ptr->func_args = led_action_args;
    func_req_ptr->func_request_type = led_action_type;

    if (osMessageQueuePut(g_main_msg_queue_id, &func_req_ptr, MSG_QUEUE_PRIORITY, MSG_QUEUE_TIMEOUT_MS) != osOK) {
        DEBUG_ERROR("Failed to put the LED function into a queue!\r\n");
        Heap_API_Free(func_req_ptr);
        return false;
    }

    DEBUG_INFO("GET_ADD_TASK!\r\n");

    return true;
}

void LED_APP_BlinkCompletedCallback (eLed_t led) {
    if ((led < eLed_First) || (led >= eLed_Last)) {
        DEBUG_ERROR("Incorrect LED number!");
        return;
    }

    g_run_data[led].is_led_ready = true;
}
