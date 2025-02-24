#ifndef __UART_DRIVER__H__
#define __UART_DRIVER__H__
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/
typedef enum {
    eUartDriver_First = 0,
    eUartDriver_1 = eUartDriver_First,
    eUartDriver_2,
    eUartDriver_Last
} eUartDriver_t;
/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/
bool UART_Driver_Init (eUartDriver_t uart, uint32_t baudrate); 
bool UART_Driver_SendByte (eUartDriver_t uart, uint8_t data);  
bool UART_Driver_SendBytes (eUartDriver_t uart, uint8_t *data, size_t length);
bool UART_Driver_GetByte (eUartDriver_t uart, uint8_t *data);
#endif /* __UART_DRIVER__H__ */
