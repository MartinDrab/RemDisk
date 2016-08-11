
#ifndef __PREPROCESSOR_H_
#define __PREPROCESSOR_H_

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


#if defined (DBG)



#define AT_FUNCTION __FILE__ ":" __FUNCTION__
#define AT_LINE AT_FUNCTION ":" TOSTRING(__LINE__)

/*
 * Prints the source file and function name. Determined for non-parametric
 * functions.
 */
#define DEBUG_ENTER_FUNCTION_NO_ARGS() \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_FUNCTION "()\n"); \

/*
 * Prints the source file, function name and parameters.
 */
#define DEBUG_ENTER_FUNCTION(paramsFormat,...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_FUNCTION "(" paramsFormat ")\n", __VA_ARGS__); \

/*
 * Prints the source file, function name and the return value.
 */
#define DEBUG_EXIT_FUNCTION(returnValueFormat,...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_FUNCTION "(-):" returnValueFormat "\n", __VA_ARGS__); \

/*
 * Prints the source file and function name. Determined for ending a function
 * without a return value.
 */
#define DEBUG_EXIT_FUNCTION_VOID() \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_FUNCTION "(-):void\n"); \

/*
 * Prints the source file, function name and the number of the line.
 */
#define DEBUG_PRINT_LOCATION_VOID() \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_LINE "\n")

/*
 * Prints the source file, function name and the number of the line.
 */
#define DEBUG_PRINT_LOCATION(format,...) \
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_LINE format "\n", __VA_ARGS__)

#define DEBUG_IRQL_LESS_OR_EQUAL(theIrql) \
	if (KeGetCurrentIrql() > theIrql) { \
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_TRACE_LEVEL, AT_LINE "Current IRQL is %d, this is greater than %d\n", KeGetCurrentIrql(), theIrql); \
		__debugbreak(); \
	} \

#else // if defined (_DEBUG) && defined (KERNEL_BUILD)

#define DEBUG_ENTER_FUNCTION_NO_ARGS() { }
#define DEBUG_ENTER_FUNCTION(paramsFormat,...) { }
#define DEBUG_EXIT_FUNCTION(returnValueFormat,...) { }
#define DEBUG_EXIT_FUNCTION_VOID() { }
#define DEBUG_PRINT_LOCATION_VOID() { }
#define DEBUG_PRINT_LOCATION(format,...) { }
#define DEBUG_IRQL_LESS_OR_EQUAL(theIrql) { }

#endif // if defined (_DEBUG) && defined (KERNEL_BUILD)

/*
 * Macro for reporting error conditions.
 */
#define DEBUG_ERROR(format,...) \
   DEBUG_PRINT_LOCATION(" ERROR: " format, __VA_ARGS__)



#endif
