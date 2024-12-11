/**********************************************************************************************************************
* Includes
*********************************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "cmsis_os2.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "message.h"
#include "string_util.h"
#include "uart_api.h"
#include "debug_api.h"
#include "heap_api.h"
#include "cmd_api.h"
#include "modem_api.h"
#include "modem_api_commands.h"
/**********************************************************************************************************************
* Private definitions and macros
*********************************************************************************************************************/
#define APN_NAME "internet.tele2.lt"
#define MODEM_BAUDRATE 115200
#define MODEM_UART eUartApiDevice_Modem
#define CMD_RECEPTION_TIMEOUT_MS 400
#define SOCKET_CLOSE_TIMEOUT_MS 10000
#define MODEM_API_SET_UP_MODEM_TASK_ATTR_NAME "SetUpModem"
#define MODEM_API_RECEIVE_TASK_ATTR_NAME "ReceiveTask"
#define MODEM_API_SET_UP_MODEM_TASK_STACK_SIZE 1024U
#define MODEM_API_RECEIVE_TASK_STACK_SIZE 1024U
#define NONE_THREAD_ARGUMENTS NULL
#define UART_MODEM_COMMAND_HANDLE_MUTEX_ATTR_NAME "ComamndHandleMutex"
#define MODEM_CONTROL_EVENT_FLAG_NAME "ModemControlFlag"
#define FAILED_TO_CLEAR_FLAG "Failed to clear flag!\r\n"
#define CMD(COMMAND) .command_name = #COMMAND, .command_name_size = sizeof(#COMMAND) - 1
#define MODEM_SETUP_COMMAND(COMMAND) .str = #COMMAND, .size = sizeof(#COMMAND) - 1
#define MODEM_AT_TABLE_SIZE 10
#define NUMBER_OF_MODEM_SET_UP_COMMANDS (sizeof(g_modem_setup_commands) / sizeof(g_modem_setup_commands[0]))
#define AT_COMMAND_BUFFER_SIZE 80
#define AT_COMMAND_PARAMETERS_BUFFER_SIZE 60
#define CLI_RESPONSE_BUFFER_SIZE 200
#define MODEM_LOCK_TIMEOUT_MS 450
/**********************************************************************************************************************
* Private typedef
*********************************************************************************************************************/
typedef struct eModemResponseSpecs {
    sString_t AT_command;
    eModemCommands_t command_id; 
} eATCommandsSpecs_t;
/**********************************************************************************************************************
* Private constants
*********************************************************************************************************************/
CREATE_MODULE_TAG (MODEM_API);
static const osThreadAttr_t g_modem_api_setup_task_attr = {
    .name = MODEM_API_SET_UP_MODEM_TASK_ATTR_NAME,
    .stack_size = MODEM_API_SET_UP_MODEM_TASK_STACK_SIZE,
    .priority = 25
};
static const osThreadAttr_t g_modem_api_receive_task_attr = {
    .name = MODEM_API_RECEIVE_TASK_ATTR_NAME,
    .stack_size = MODEM_API_RECEIVE_TASK_STACK_SIZE,
    .priority = 25
};
static const osMutexAttr_t g_uart_modem_command_handle_attr = {
    .name = UART_MODEM_COMMAND_HANDLE_MUTEX_ATTR_NAME
};
static const osEventFlagsAttr_t g_state_flag_attr = {
    .name = MODEM_CONTROL_EVENT_FLAG_NAME
};

static const sCommandDescription_t g_modem_callback_function_lut[] = {
    {.command_function = &Modem_API_CMD_OK, CMD(OK)},
    {.command_function = &Modem_API_CMD_DisableEcho, CMD(ATE0)},
    {.command_function = &Modem_API_CMD_Error, CMD(ERROR)},
    {.command_function = &Modem_API_CMD_GetError, CMD(+QIGETERROR:)},
    {.command_function = &Modem_API_CMD_NetworkRegStatus, CMD(+CEREG:)},
    {.command_function = &Modem_API_CMD_AddressPDP, CMD(+CGPADDR:)},
    {.command_function = &Modem_API_CMD_QIOPEN, CMD(+QIOPEN:)},
    {.command_function = &Modem_API_CMD_ReadyToSend, CMD(>)},
    {.command_function = &Modem_CMD_SendOk, CMD(SEND OK)},
    {.command_function = &Modem_API_CMD_SendFail, CMD(SEND FAIL)},
    {.command_function = &Modem_API_CMD_QIURC, CMD(+QIURC:)}
};

static char g_command_reply_buffer[CLI_RESPONSE_BUFFER_SIZE] = {0};

static sBuffer_t g_response_buffer = {.str = g_command_reply_buffer, .size = CLI_RESPONSE_BUFFER_SIZE, .count = 0};

static const sCommandLauncherArgs_t g_modem_cmd_launcher_params = {
    .commands_table = g_modem_callback_function_lut,
    .commands_table_size = sizeof(g_modem_callback_function_lut) / sizeof(g_modem_callback_function_lut[0]),
    .response_buffer = &g_response_buffer
};
static const sString_t g_modem_setup_commands[] = {
    {MODEM_SETUP_COMMAND(RDY)},
    {MODEM_SETUP_COMMAND(+CFUN: 1)},
    {MODEM_SETUP_COMMAND(+CPIN: READY)},
    {MODEM_SETUP_COMMAND(+QUSIM: 1)},
    {MODEM_SETUP_COMMAND(+QIND: SMS DONE)},
    {MODEM_SETUP_COMMAND(+QIND: PB DONE)}
};
static const sString_t g_modem_AT_commands[eModemCommands_Last] = {
    [eModemCommands_ATE0]       = {MODEM_SETUP_COMMAND(E)},
    [eModemCommands_QICSGP]     = {MODEM_SETUP_COMMAND(+QICSGP=)},
    [eModemCommands_QIACT]      = {MODEM_SETUP_COMMAND(+QIACT=)},
    [eModemCommands_CEREG]      = {MODEM_SETUP_COMMAND(+CEREG)},
    [eModemCommands_CGPADDR]    = {MODEM_SETUP_COMMAND(+CGPADDR=)},
    [eModemCommands_QIGETERROR] = {MODEM_SETUP_COMMAND(+QIGET)}, 
    [eModemCommands_QIOPEN]     = {MODEM_SETUP_COMMAND(+QIOPEN=)},
    [eModemCommands_QISEND]     = {MODEM_SETUP_COMMAND(+QISEND=)},
    [eModemCommands_QIURC]      = {MODEM_SETUP_COMMAND(+QIRD=)},
    [eModemCommands_QICLOSE]    = {MODEM_SETUP_COMMAND(+QICLOSE=)}
};
static uint32_t g_modem_flags[eModemFlag_Last] = {
    [eModemFlags_Ready]           = 0x01,          
    [eModemFlags_ResponseOK]      = 0x02,     
    [eModemFlags_EchoDisabled]    = 0x04,   
    [eModemFlags_Error]           = 0x08,          
    [eModemFlags_Registered]      = 0x10,     
    [eModemFlags_ValidPDPAddress] = 0x20,
    [eModemFlags_ServerOpen]      = 0x40,   
    [eModemFlags_SendOK]          = 0x80,        
    [eModemFlags_SendFail]        = 0x100,      
    [eModemFlags_DataReceived]    = 0x200,  
};
/**********************************************************************************************************************
* Private variables
*********************************************************************************************************************/
static osThreadId_t g_modem_api_setup_task_id = NULL;
static osThreadId_t g_modem_api_receive_task_id = NULL;
static osMutexId_t g_uart_modem_command_handle_id = NULL;
static osEventFlagsId_t g_status_flag_id = NULL;
static sString_t modem_command;
static eModemState_t g_modem_state;
static sString_t g_modem_message;
static bool set_up_cmd_received = false;
static uint32_t flag = 0;
/**********************************************************************************************************************
* Exported variables and references
*********************************************************************************************************************/

/**********************************************************************************************************************
* Prototypes of private functions
*********************************************************************************************************************/
static void Modem_API_SetUpModem (void *args);
static void Modem_API_ReceiveTask (void *args);
static bool Modem_API_ClearFlagByCommand (eModemFlags_t command_flag);
/**********************************************************************************************************************
* Definitions of private functions
*********************************************************************************************************************/
static void Modem_API_SetUpModem (void *args) {
    uint8_t cmd_count = 0;
    char cmd_params_str[AT_COMMAND_PARAMETERS_BUFFER_SIZE] = {0};
    size_t cmd_params_size = 0;

    while (1) {
        switch (g_modem_state) {
            case eModemState_TurnedOff: {
                if (((GPIO_Driver_Write(eGPIODriver_ModemPowerOffPin, eGPIO_PinState_Low)) ||
                    (GPIO_Driver_Write(eGPIODriver_ModemOnPin, eGPIO_PinState_High)) ||
                    (GPIO_Driver_Write(eGPIODriver_Reset_NPin, eGPIO_PinState_High))) == false) {
                    DEBUG_WARN("Failed to set the write pins in the second boot cycle!\r\n");
                    break;
                }

                osDelay(10);

                if (((GPIO_Driver_Write(eGPIODriver_ModemPowerOffPin, eGPIO_PinState_High)) ||
                    (GPIO_Driver_Write(eGPIODriver_ModemOnPin, eGPIO_PinState_Low)) ||
                    (GPIO_Driver_Write(eGPIODriver_Reset_NPin, eGPIO_PinState_Low))) == false) {
                    DEBUG_WARN("Failed to set the write pins in the third boot cycle!\r\n");
                    break;
                }

                osDelay(10);

                if (GPIO_Driver_Write(eGPIODriver_ModemOnPin, eGPIO_PinState_High) == false) {
                    DEBUG_WARN("Failed to set the write pin!\r\n");
                    break;
                }

                osDelay(510);

                if (GPIO_Driver_Write(eGPIODriver_ModemOnPin, eGPIO_PinState_Low) == false) {
                    DEBUG_WARN("Failed to set the write pin!\r\n");
                    break;
                }

                osDelay(13000);

                if (((GPIO_Driver_Write(eGPIODriver_ModemUartDtrPin, eGPIO_PinState_Low)) || 
                    (GPIO_Driver_Write(eGPIODriver_GnssOnPin, eGPIO_PinState_High))) == false) {
                    DEBUG_WARN("Failed to set the write pins!\r\n");
                    break;
                }

                g_modem_state = eModemState_TurnedOn;
            }
            case eModemState_TurnedOn: {
                while (1) {
                    if (UART_API_GetMessage(MODEM_UART, &modem_command, CMD_RECEPTION_TIMEOUT_MS) == false) {
                        DEBUG_ERROR("Failed to receive commands after modem start up!\r\n");
                        continue;
                    }

                    if (modem_command.size < 2) {
                        Heap_API_Free(modem_command.str);
                        continue;
                    }
                    
                    for (uint8_t i = 0; i < NUMBER_OF_MODEM_SET_UP_COMMANDS; i++) {
                        if (strncmp(modem_command.str, g_modem_setup_commands[i].str, g_modem_setup_commands[i].size) == 0) {
                            set_up_cmd_received = true;
                            cmd_count++;
                            Heap_API_Free(modem_command.str);
                            break;
                        }
                    }

                    if ((set_up_cmd_received == true) && (cmd_count != NUMBER_OF_MODEM_SET_UP_COMMANDS)) {
                        set_up_cmd_received = false;
                        continue;
                    }
                    else if (cmd_count == NUMBER_OF_MODEM_SET_UP_COMMANDS) {
                        flag = osEventFlagsSet(g_status_flag_id, g_modem_flags[eModemFlags_Ready]);
                        if (flag >= osFlagsError) {
                            DEBUG_ERROR("Failed to set the flag, error occured!\r\n");
                        }
                        g_modem_state = eModemState_Ready;
                        break;
                    }
                    else {
                        DEBUG_ERROR("Received incorrect command: %s, modem is not ready for a connection!\r\n", modem_command.str);
                        g_modem_state = eModemState_TurnedOn;
                        Heap_API_Free(modem_command.str);
                        break;
                    }
                }
            }    
            case eModemState_Ready: {

                if (Modem_API_LockModem(MODEM_LOCK_TIMEOUT_MS) == false) {
                    DEBUG_ERROR("Failed to lock the modem!\r\n");
                    break;
                }
                //ATE0: ECHO Disable
                cmd_params_size = snprintf(cmd_params_str, AT_COMMAND_PARAMETERS_BUFFER_SIZE, "0") + 1;
                while (Modem_API_SendCommand(eModemCommands_ATE0, eModemFlags_EchoDisabled, cmd_params_str, cmd_params_size) != eModemError_ATSuccess) {
                    osDelay(1000);
                    DEBUG_ERROR("Failed to disable ECHO mode, retrying!...\r\n");
                }

                //QICSGP: Define PDP context
                cmd_params_size = snprintf(cmd_params_str, AT_COMMAND_PARAMETERS_BUFFER_SIZE,
                                           "1,1,\"%s\",\"\",\"\",0", APN_NAME) + 1;
                while (Modem_API_SendCommand(eModemCommands_QICSGP, eModemFlags_ResponseOK, cmd_params_str, cmd_params_size) != eModemError_ATSuccess) {
                    osDelay(1000);
                    DEBUG_INFO("Defining PDP context... !!!\r\n");
                }

                //QIACT: Activate PDP context
                cmd_params_size = snprintf(cmd_params_str, AT_COMMAND_PARAMETERS_BUFFER_SIZE, "1") + 1;
                while (Modem_API_SendCommand(eModemCommands_QIACT, eModemFlags_ResponseOK, cmd_params_str, cmd_params_size) != eModemError_ATSuccess) {
                    osDelay(1000);
                    DEBUG_INFO("Retrying PDP activation...!!\r\n");
                }

                osDelay(100);

                //CEREG: Check Network registration status
                cmd_params_size = snprintf(cmd_params_str, AT_COMMAND_PARAMETERS_BUFFER_SIZE, "?") + 1;
                while (Modem_API_SendCommand(eModemCommands_CEREG, eModemFlags_Registered, cmd_params_str, cmd_params_size) != eModemError_ATSuccess) {
                    osDelay(1000);
                    DEBUG_INFO("Retrying network registration status identification procedure...!!\r\n");
                }

                //CGPADDR: Check PDP address
                cmd_params_size = snprintf(cmd_params_str, AT_COMMAND_PARAMETERS_BUFFER_SIZE, "1") + 1;
                while (Modem_API_SendCommand(eModemCommands_CGPADDR, eModemFlags_ValidPDPAddress, cmd_params_str, cmd_params_size) != eModemError_ATSuccess) {
                    osDelay(1000);
                    DEBUG_INFO("Retrying PDP address checking procedure...!!\r\n");
                }

                if (Modem_API_UnlockModem() == false) {
                    DEBUG_ERROR("Failed to Unlock the modem!\r\n");
                    break;
                }

                g_modem_state = eModemState_Initialized;
                break;
            }
            default: {
                DEBUG_ERROR("Unexpected modem state!\r\n");
                g_modem_state = eModemState_TurnedOff;
                break;
            }
        }

        if (g_modem_state == eModemState_Initialized) {
            DEBUG_INFO("Modem is ready for connection!\r\n");
            break;
        }
        else {
            DEBUG_INFO("Modem is not setup correctly, retrying the process! ...\r\n");
            osDelay(5000);
            continue;
        }
    }

    osThreadExit();
}

static void Modem_API_ReceiveTask (void *args) {

    flag = osEventFlagsWait(g_status_flag_id, g_modem_flags[eModemFlags_Ready], osFlagsNoClear, osWaitForever);
    if (flag >= osFlagsError) {
        DEBUG_ERROR("Error occured while waiting for modem entering the ready state!\r\n");
        osThreadExit();
    }

    while (1) {
        if (UART_API_GetMessage(MODEM_UART, &g_modem_message, CMD_RECEPTION_TIMEOUT_MS) == false) {
            continue;
        }

        if (g_modem_message.size < 2) {
            Heap_API_Free(g_modem_message.str);
            continue;
        }

        if (CMD_API_Launcher(g_modem_message, &g_modem_cmd_launcher_params) == false) {
            DEBUG_WARN("%s", g_response_buffer);
        } else {
            DEBUG_INFO("%s", g_response_buffer);
        }

        Heap_API_Free(g_modem_message.str);
    }
}

static bool Modem_API_ClearFlagByCommand (eModemFlags_t command_flag) {
    bool is_flag_cleared = true;
    is_flag_cleared = Modem_API_ClearFlag(command_flag) && Modem_API_ClearFlag(eModemFlags_Error) &&
                      Modem_API_ClearFlag(eModemFlags_ResponseOK);

    return is_flag_cleared;
}
/**********************************************************************************************************************
* Definitions of exported functions
*********************************************************************************************************************/
bool Modem_API_Init (void) {
    g_modem_state = eModemState_TurnedOff;

    sString_t delimiter = (sString_t)DEFINE_STRING("\r\n");
    if (UART_API_Init(MODEM_UART, MODEM_BAUDRATE, delimiter) == false) {
        DEBUG_ERROR("Failed to initialize modem uart api function!\r\n");
        return false;
    }

    if (g_uart_modem_command_handle_id == NULL) {
        g_uart_modem_command_handle_id = osMutexNew(&g_uart_modem_command_handle_attr);
        if (g_uart_modem_command_handle_id == NULL) {
            DEBUG_ERROR("Failed to create mutex for modem command handling!\r\n");
            return false;
        }
    }

    if (g_status_flag_id == NULL) {
        g_status_flag_id = osEventFlagsNew(&g_state_flag_attr);
        if (g_status_flag_id == NULL) {
            DEBUG_ERROR("Failed to create an event flag for modem state indication!\r\n");
            return false;
        }
    }

    if (g_modem_api_setup_task_id == NULL) {
        g_modem_api_setup_task_id = osThreadNew(&Modem_API_SetUpModem, 
                                                NONE_THREAD_ARGUMENTS,
                                                &g_modem_api_setup_task_attr);
        if (g_modem_api_setup_task_id == NULL) {
            DEBUG_ERROR("Failed to create thread for setting up the modem!\r\n");
            return false;
        }
    }

    if (g_modem_api_receive_task_id == NULL) {
        g_modem_api_receive_task_id = osThreadNew(&Modem_API_ReceiveTask,
                                                  NONE_THREAD_ARGUMENTS,
                                                  &g_modem_api_receive_task_attr);
        if (g_modem_api_receive_task_id == NULL) {
            DEBUG_ERROR("Failed to create thread for AT command processing!\r\n");
            return false;
        }
    }

    return true;
}

eModemError_t Modem_API_SendCommand (eModemCommands_t AT_command, eModemFlags_t command_flag, char *cmd_params_string, size_t cmd_params_size) {
    eModemError_t error_type = eModemError_ATSuccess;

    if ((AT_command < eModemCommands_First) || (AT_command >= eModemCommands_Last) || 
        (cmd_params_string == NULL) || (cmd_params_size == 0)) {
        DEBUG_ERROR("Invalid AT command or its parameters!\r\n");
        return eModemError_InvalidParameters;
    }

    if (Modem_API_ClearFlagByCommand(command_flag) == false) {
        return eModemError_ClearFlagFail;
    }

    sString_t formatted_AT_command;
    formatted_AT_command.str = (char *) Heap_API_Calloc(AT_COMMAND_BUFFER_SIZE, sizeof(char));

    if (formatted_AT_command.str == NULL) {
        DEBUG_ERROR("Failed to allocate space for AT command!\r\n");
        return eModemError_MemoryAllocationFail;
    }

    formatted_AT_command.size = snprintf(formatted_AT_command.str, AT_COMMAND_BUFFER_SIZE, "AT%s%s\r\n", 
                                         g_modem_AT_commands[AT_command].str, cmd_params_string) + 1;

    if ((formatted_AT_command.size == 0) || (formatted_AT_command.str == NULL)) {
        error_type = eModemError_InvalidParameters;
    }

    if (UART_API_SendMessage(MODEM_UART, formatted_AT_command) == false) {
        DEBUG_ERROR("Failed to send %s command!\r\n", formatted_AT_command.str);
        error_type = eModemError_SendFail;
    }

    if (AT_command != eModemCommands_QISEND) {
        uint32_t timeout_ms = ((AT_command == eModemCommands_QICLOSE) || (AT_command == eModemCommands_QIGETERROR)) ? 
                               SOCKET_CLOSE_TIMEOUT_MS : CMD_RECEPTION_TIMEOUT_MS;
        uint32_t flags = osEventFlagsWait(g_status_flag_id, g_modem_flags[eModemFlags_Error] | g_modem_flags[command_flag], 
                                          osFlagsWaitAny, timeout_ms);
        if (flags >= osFlagsError) {
            DEBUG_INFO("Wait flag event failed!\r\n");
            error_type = eModemError_WaitFlagFail;
        }
    }

    Heap_API_Free(formatted_AT_command.str);

    return error_type;
}

bool Modem_API_SetFlag (eModemFlags_t flag_to_set) {
    if ((flag_to_set < eModemFlags_First) || (flag_to_set >= eModemFlag_Last)) {
        DEBUG_ERROR("Invalid flag is passed!\r\n");
        return false;
    }

    uint32_t flag_check = osEventFlagsSet(g_status_flag_id, g_modem_flags[flag_to_set]);
    if (flag_check >= osFlagsError) {
        DEBUG_ERROR("Fail occured in setting the flag!\r\n");
        return false;
    }

    return true;
}

bool Modem_API_IsFlagSet (eModemFlags_t flag_to_check) {
    if ((flag_to_check < eModemFlags_First) || (flag_to_check >= eModemFlag_Last)) {
        DEBUG_ERROR("Invalid flag is passed!\r\n");
        return false;
    }

    if ((osEventFlagsGet(g_status_flag_id) & g_modem_flags[flag_to_check]) > 0) {
        return true;
    }
    else {
        return false;
    }
}

bool Modem_API_ClearFlag (eModemFlags_t flag_to_clear) {
    if ((flag_to_clear < eModemFlags_First) || (flag_to_clear >= eModemFlag_Last)) {
        DEBUG_ERROR("Invalid flag is passed!\r\n");
        return false;
    }

    if (osEventFlagsClear(g_status_flag_id, g_modem_flags[flag_to_clear]) >= osFlagsError) {
        DEBUG_ERROR("Failed to clear the flag!\r\n");
        return false;
    }

    return true;
}

bool Modem_API_LockModem (uint32_t timeout) {
    if (timeout < 0) {
        DEBUG_ERROR("Invalid timeout value!\r\n");
        return false;
    }

    if (osMutexAcquire(g_uart_modem_command_handle_id, timeout) != osOK) {
        DEBUG_ERROR("Failed to acquire command handling mutex!\r\n");
        return false;
    }

    return true;
}

bool Modem_API_UnlockModem (void) {
    if (osMutexRelease(g_uart_modem_command_handle_id) != osOK) {
        DEBUG_ERROR("Failed to release command handling mutex!\r\n");
        return false;
    }

    return true;
}

eModemState_t Modem_API_GetState (void) {
    return g_modem_state;
}
