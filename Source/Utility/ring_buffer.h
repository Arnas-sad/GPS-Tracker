#ifndef SOURCE_UTILITY_RING_BUFFER_H_
#define SOURCE_UTILITY_RING_BUFFER_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/
typedef struct sRingBuffer_t *RingBufferHandle_t;
/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/
RingBufferHandle_t RingBuffer_Init (size_t max_length);
bool RingBuffer_Put (RingBufferHandle_t rb, uint8_t byte);
bool RingBuffer_Get (RingBufferHandle_t rb, uint8_t *byte);
bool RingBuffer_Free (RingBufferHandle_t rb);
#endif /* SOURCE_UTILITY_RING_BUFFER_H_ */