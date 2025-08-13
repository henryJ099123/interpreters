#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/***** FLAGS FOR DEBUGGING/FEATURES *****/
#undef  NAN_BOXING

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

// stress mode. GC runs as often as possible
#undef  DEBUG_STRESS_GC
#define DEBUG_LOG_GC
/***** END FLAGS *****/

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
