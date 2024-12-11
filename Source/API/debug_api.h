#ifndef SOURCE_API_DEBUG_API_H_
#define SOURCE_API_DEBUG_API_H_
/**********************************************************************************************************************
* Includes
*********************************************************************************************************************/
#include <stdbool.h>
/**********************************************************************************************************************
* Exported definitions and macros
*********************************************************************************************************************/
#define CREATE_MODULE_TAG(tag) static const char *module_tag = #tag
#define STRINGIZE(TAG) #TAG
#define DEBUG_INFO(format, ...) \
   Debug_API_PrintMessage(module_tag, NULL, 0, eDebugLevel_Info, format, ##__VA_ARGS__)
#define DEBUG_WARN(format, ...) \
   Debug_API_PrintMessage(module_tag, __FILE__, __LINE__, eDebugLevel_Warning, format, ##__VA_ARGS__)
#define DEBUG_ERROR(format, ...) \
   Debug_API_PrintMessage(module_tag, __FILE__, __LINE__, eDebugLevel_Error, format, ##__VA_ARGS__)
/**********************************************************************************************************************
* Exported types
*********************************************************************************************************************/
typedef enum {
   eDebugLevel_First = 0,
   eDebugLevel_Info = eDebugLevel_First,
   eDebugLevel_Warning,
   eDebugLevel_Error,
   eDebugLevel_Last
} eDebugLevel_t;
/**********************************************************************************************************************
* Exported variables
*********************************************************************************************************************/

/**********************************************************************************************************************
* Prototypes of exported functions
*********************************************************************************************************************/
bool Debug_API_Init(void);
bool Debug_API_PrintMessage(const char *module_tag, const char *file, int line, eDebugLevel_t debug_level, const char *format, ...);
#endif /* SOURCE_API_DEBUG_API_H_ */