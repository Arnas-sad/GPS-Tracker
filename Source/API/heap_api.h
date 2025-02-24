#ifndef SOURCE_API_HEAP_API_H_
#define SOURCE_API_HEAP_API_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
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
bool Heap_API_Init (void);
void *Heap_API_Malloc (size_t element_size);
void *Heap_API_Calloc (size_t num_elements, size_t element_size);
void Heap_API_Free (void *mem_ptr);
#endif /* SOURCE_API_HEAP_API_H_ */
