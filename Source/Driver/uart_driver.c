/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f413xx.h"
#include "uart_driver.h"
#include "ring_buffer.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define USART1_RING_BUFFER_SIZE 1024
#define USART2_RING_BUFFER_SIZE 1024
#define USART1_IRQ_PRIORITY 0
#define USART2_IRQ_PRIORITY 0
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
typedef struct {
    USART_TypeDef *usart_port;
    uint32_t datawidth;
    uint32_t stopbits;
    uint32_t parity;
    uint32_t transferdirection;
    uint32_t hardwareflowcontrol;
    uint32_t oversampling;
    uint32_t clock;
    void (*clock_func)(uint32_t periph);
    bool enable_rxne;
    size_t ring_buffer_size;
    IRQn_Type irq_type;
    uint32_t irq_priority;
} sUart_Params_t;
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
const static sUart_Params_t g_static_usart_lut[eUartDriver_Last] = {
    [eUartDriver_1] = {.usart_port = USART1,                          .datawidth = LL_USART_DATAWIDTH_8B,             
                       .stopbits = LL_USART_STOPBITS_1,               .parity = LL_USART_PARITY_NONE,                 
                       .transferdirection = LL_USART_DIRECTION_TX_RX, .hardwareflowcontrol = LL_USART_HWCONTROL_NONE, 
                       .oversampling = LL_USART_OVERSAMPLING_16,      .clock = LL_APB2_GRP1_PERIPH_USART1,
                       .clock_func = LL_APB2_GRP1_EnableClock,        .enable_rxne = true,
                       .ring_buffer_size = USART1_RING_BUFFER_SIZE,   .irq_type = USART1_IRQn,
                       .irq_priority = USART1_IRQ_PRIORITY},

    [eUartDriver_2] = {.usart_port = USART2,                          .datawidth = LL_USART_DATAWIDTH_8B,             
                       .stopbits = LL_USART_STOPBITS_1,               .parity = LL_USART_PARITY_NONE,                 
                       .transferdirection = LL_USART_DIRECTION_TX_RX, .hardwareflowcontrol = LL_USART_HWCONTROL_NONE, 
                       .oversampling = LL_USART_OVERSAMPLING_16,      .clock = LL_APB1_GRP1_PERIPH_USART2,
                       .clock_func = LL_APB1_GRP1_EnableClock,        .enable_rxne = true,
                       .ring_buffer_size = USART2_RING_BUFFER_SIZE,   .irq_type = USART2_IRQn,
                       .irq_priority = USART2_IRQ_PRIORITY}
};                                                           
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
static RingBufferHandle_t g_ring_buffer[eUartDriver_Last] = {0};
/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/
void UART_Driver_IRQHandler (eUartDriver_t uart);
/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/
void UART_Driver_IRQHandler (eUartDriver_t uart) {
    if ((LL_USART_IsEnabledIT_RXNE(g_static_usart_lut[uart].usart_port)) && (LL_USART_IsActiveFlag_RXNE(g_static_usart_lut[uart].usart_port))) {
        uint8_t data = LL_USART_ReceiveData8(g_static_usart_lut[uart].usart_port);
        RingBuffer_Put(g_ring_buffer[uart], data);
    }
}

void USART1_IRQHandler (void) {
    UART_Driver_IRQHandler(eUartDriver_1);
}

void USART2_IRQHandler (void) {
    UART_Driver_IRQHandler(eUartDriver_2);
}
/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/
bool UART_Driver_Init (eUartDriver_t uart, uint32_t baudrate) {
    if ((uart >= eUartDriver_Last) || (baudrate == 0)) {
        return false;
    }
    
    g_static_usart_lut[uart].clock_func(g_static_usart_lut[uart].clock);

    LL_USART_InitTypeDef uart_init_struct = {0};

    uart_init_struct.BaudRate = baudrate;
    uart_init_struct.DataWidth = g_static_usart_lut[uart].datawidth;
    uart_init_struct.StopBits = g_static_usart_lut[uart].stopbits;
    uart_init_struct.Parity = g_static_usart_lut[uart].parity;
    uart_init_struct.TransferDirection = g_static_usart_lut[uart].transferdirection;
    uart_init_struct.HardwareFlowControl = g_static_usart_lut[uart].hardwareflowcontrol;
    uart_init_struct.OverSampling = g_static_usart_lut[uart].oversampling;

    if (LL_USART_Init(g_static_usart_lut[uart].usart_port, &uart_init_struct) == ERROR) {
        return false;
    }  

    LL_USART_ConfigAsyncMode(g_static_usart_lut[uart].usart_port);  

    if (g_static_usart_lut[uart].enable_rxne) {
        g_ring_buffer[uart] = RingBuffer_Init(g_static_usart_lut[uart].ring_buffer_size);

        if (g_ring_buffer[uart] == NULL) {
            return false;
        }

        NVIC_SetPriority(g_static_usart_lut[uart].irq_type, g_static_usart_lut[uart].irq_priority);
        NVIC_EnableIRQ(g_static_usart_lut[uart].irq_type);
        LL_USART_EnableIT_RXNE(g_static_usart_lut[uart].usart_port);
    }

    LL_USART_Enable(g_static_usart_lut[uart].usart_port);

    return true;
}

bool UART_Driver_SendByte (eUartDriver_t uart, uint8_t data) {
    if (uart >= eUartDriver_Last) {
        return false;
    }

    while (LL_USART_IsActiveFlag_TXE(g_static_usart_lut[uart].usart_port) == 0) {
    }

    LL_USART_TransmitData8(g_static_usart_lut[uart].usart_port, data);

    return true;
}

bool UART_Driver_SendBytes (eUartDriver_t uart, uint8_t *data, size_t length) {
    if ((uart >= eUartDriver_Last) || (data == NULL) || (length == 0)) {
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        if (!UART_Driver_SendByte(uart, data[i])) {
            return false;
        }
    }
    
    return true;
}

bool UART_Driver_GetByte (eUartDriver_t uart, uint8_t *data) {
    if ((uart >= eUartDriver_Last) || (data == NULL)) {
        return false;
    }

    bool return_val = true;

    LL_USART_DisableIT_RXNE(g_static_usart_lut[uart].usart_port);

    if (RingBuffer_Get(g_ring_buffer[uart], data) == false) {
        return_val = false;
    }

    LL_USART_EnableIT_RXNE(g_static_usart_lut[uart].usart_port);

    return return_val;
}
