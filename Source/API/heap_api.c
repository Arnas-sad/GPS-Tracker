/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/
#include <stdbool.h>
#include "cmsis_os2.h"
#include "heap_api.h"
/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/
#define CFG_HEAP_API_MUTEX_NAME "HeapApiMutex"
#define DEBUG_MUTEX_ACQUIRE_TIMEOUT 100
/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
 
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/
static const osMutexAttr_t g_heap_mutex_attr = {.name = CFG_HEAP_API_MUTEX_NAME};
/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/
static osMutexId_t g_heap_mutex_id = NULL;
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
bool Heap_API_Init (void) {

    if (g_heap_mutex_id != NULL) {
        return true;
    }

    g_heap_mutex_id = osMutexNew(&g_heap_mutex_attr);

    if (g_heap_mutex_id == NULL) {
        return false;
    }

    return true;
}

void *Heap_API_Malloc (size_t element_size) {

    if (g_heap_mutex_id == NULL) {
        return NULL;
    }

    if (osMutexAcquire(g_heap_mutex_id, DEBUG_MUTEX_ACQUIRE_TIMEOUT) != osOK) {
        return NULL;
    }

    void *mem_ptr = malloc(element_size);

    if (osMutexRelease(g_heap_mutex_id) != osOK) {
        free(mem_ptr);
        return NULL;
    }

    return mem_ptr;
}

void *Heap_API_Calloc (size_t num_elements, size_t element_size) {

    if (g_heap_mutex_id == NULL) {
        return NULL;
    }

    if (osMutexAcquire(g_heap_mutex_id, DEBUG_MUTEX_ACQUIRE_TIMEOUT) != osOK) {
        return NULL;
    }

    void *mem_ptr = calloc(num_elements, element_size);

    if (osMutexRelease(g_heap_mutex_id) != osOK) {
        free(mem_ptr);
        return NULL;
    }

    return mem_ptr;
}

void Heap_API_Free (void *mem_ptr) {

    if (g_heap_mutex_id == NULL) {
        return;
    }

    if (osMutexAcquire(g_heap_mutex_id, DEBUG_MUTEX_ACQUIRE_TIMEOUT) != osOK) {
        return;
    }

    free(mem_ptr);

    if (osMutexRelease(g_heap_mutex_id) != osOK) {
        return;
    }

    return;
}
