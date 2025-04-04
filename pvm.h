#ifndef PVM_PVM_H
#define PVM_PVM_H

#include <stdint.h>
#include <stddef.h>

#define PVM_MIN_VERSION 1

#ifndef PVM_DATA_STACK_SIZE
#define PVM_DATA_STACK_SIZE 30
#endif
#ifndef PVM_CALL_STACK_SIZE
#define PVM_CALL_STACK_SIZE 10
#endif

#ifndef packed_struct
#define packed_struct struct __attribute__((__packed__, aligned(1)))
#endif

typedef uint8_t pvm_op_t;
typedef uint16_t pvm_address_t;
typedef int32_t pvm_data_t;
#define PVM_DATA_SIGN 0x80000000ul
// typedef uint16_t pvm_data_t;
// #define PVM_DATA_SIGN 0xFFFF8000ul
typedef int32_t pvm_const_t;
#define PVM_CONST_SIGN 0x80000000ul
// typedef uint16_t pvm_const_t;
// #define PVM_CONST_SIGN 0xFFFF8000ul
typedef uint8_t pvm_data_stack_t;
typedef uint8_t pvm_call_stack_t;
typedef uint8_t pvm_function_index_t;
#define PVM_INTEGRAL_OP_MASK 0x0F

/// \brief Represents a function in the PVM executable.
///
/// \details This structure defines the attributes of a function within the PVM executable.
/// It includes the function's address, argument size, variable size, return size, and flags indicating if the function is variadic or a system library function.
///
/// \note The structure is packed to ensure efficient memory usage.
typedef packed_struct pvm_function {
	pvm_address_t address;
	uint8_t args_size;
	uint8_t variables_size;
	uint8_t returns_size : 6;
	uint8_t variadic : 1;
	uint8_t sys_lib : 1;
} pvm_function_t;

/// \brief Represents the PVM executable.
///
/// \details This structure defines the layout of a PVM executable, including its size, minimum VM version, number of functions, number of constants, and the number of main variables.
/// It also includes an array of function definitions.
///
/// \note The structure is packed to ensure efficient memory usage.
typedef packed_struct pvm_exe {
	pvm_address_t size;
	uint8_t min_vm_version;
	uint8_t functions_size;
	uint8_t constants_size;
	uint8_t main_variables_count;
	pvm_function_t functions[];
} pvm_exe_t;

/// \brief Represents the PVM instance.
///
/// \details This structure defines the state of a PVM instance, including the timer, timeout, data stack, call stack, program counter, stack tops, and persistent data.
/// The persistent data section contains the binding and a pointer to the executable.
///
/// \note The structure is packed to ensure efficient memory usage.
typedef packed_struct pvm {
	uint32_t timer;
	uint32_t timeout;
	pvm_data_t data_stack[PVM_DATA_STACK_SIZE];
	packed_struct pvm_call_stack {
		pvm_address_t return_address;
		pvm_data_stack_t variables_start;
		pvm_data_stack_t args_size;
		pvm_function_index_t function_index;
	} call_stack[PVM_CALL_STACK_SIZE];
	pvm_address_t pc;
	pvm_data_stack_t data_top;
	pvm_call_stack_t call_top;
	// data that persists over reset
	packed_struct {
		uint8_t binding;
		const pvm_exe_t *exe;
	} persist;
} pvm_t;

/// \brief Enumerates the possible error codes returned by PVM functions.
///
/// \details This enumeration defines various error codes that can be returned by PVM functions to indicate different types of errors.
/// The error codes cover issues related to stack underflow/overflow, invalid function indices, program counter overrun, and other runtime errors.
///
/// \note The error codes are designed to be used for error handling and debugging in the PVM.
typedef enum pvm_errno {
	PVM_NO_ERROR = 0,
	PVM_MAIN_RETURN,
	PVM_CALL_STACK_UNDERFLOW = PVM_MAIN_RETURN,
	PVM_CALL_STACK_OVERFLOW,
	PVM_DATA_STACK_UNDERFLOW,
	PVM_DATA_STACK_OVERFLOW,
	PVM_ARG_OUT_OF_STACK,
	PVM_VAR_OUT_OF_STACK,
	PVM_RETURN_OUT_OF_STACK,
	PVM_DATA_STACK_SMASHED,
	PVM_PC_OVERRUN,
	PVM_EXE_NO_FUNCTION,
	PVM_BUILTIN_NO_FUNCTION,
	PVM_NO_VARIABLE,
	PVM_NO_CONSTANT,
	PVM_VARIADIC_SIZE
} pvm_errno_t;

/// \brief Defines the signature for built-in functions in the PVM.
///
/// \param[in,out] vm The PVM instance.
/// \param[in,out] arguments An array of arguments passed to the built-in function.
/// \param[in] args_size The number of arguments in the array.
///
/// \details This type definition specifies the prototype for built-in functions in the PVM.
/// Built-in functions are implemented as C functions and can be called directly by the PVM.
///
/// \note Built-in functions are used to extend the functionality of the PVM with custom operations.
typedef void(pvm_builtin_f(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size));

/// \brief Array of built-in functions for the PVM.
///
/// \details This array contains pointers to built-in functions that extend the functionality of the PVM.
/// Each entry in the array corresponds to a built-in function, which is implemented as a C function.
extern const packed_struct pvm_builtins {
	pvm_builtin_f *func;
} pvm_builtins[];

/// \brief The number of built-in functions in the PVM.
///
/// \details This constant defines the total number of built-in functions available in the PVM.
/// It is used to determine the size of the `pvm_builtins` array.
///
/// \note This value is used for bounds checking and to ensure that built-in function indices are valid.
/// Typically assign in builtins c module as<br>\code (sizeof(pvm_builtins) / sizeof(pvm_builtins[0]))
extern const size_t pvm_builtins_size;

/// \brief A necessary to implement function that is used for SLP instruction functionality.
/// It returns the current time in milliseconds since an unspecified starting point, which is not affected by system time changes.
///
/// \return The current time span in milliseconds.
///
/// \note The returned value is not the actual time of day, but rather the time elapsed since an unspecified starting point.
extern uint32_t now_ms(void);

/// \brief Checks the validity of a PVM executable.
///
/// \param[in] exe The PVM executable to check.
/// \param[in] size The size of the executable in bytes.
///
/// \return PVM_EXE_SIZE if the size does not match, PVM_EXE_VERSION if the minimum VM version does not match, otherwise PVM_EXE_OK.
///
/// \details This function verifies the size and minimum VM version of the given PVM executable.
/// It returns an error if the size of the executable does not match the expected size or if the minimum VM version does not match the current version.
///
/// \note The size parameter should include the size of the executable header.
enum pvm_exe_check_result {
	PVM_EXE_OK = 0,
	PVM_EXE_SIZE,
	PVM_EXE_VERSION
} pvm_exe_check(const pvm_exe_t *exe, size_t size);

/// \brief Resets the PVM instance to its initial state.
///
/// \param[in,out] vm The PVM instance to reset.
///
/// \details This function clears all the runtime data of the PVM instance, except for the persistent data.
/// After resetting, the data stack top is set to the number of main variables defined in the executable.
///
/// \note This function does not modify the executable or the persistent data.
void pvm_reset(pvm_t *vm);

/// \brief Executes the next instruction in the PVM.
///
/// \param[in,out] vm The PVM instance.
///
/// \return PVM_NO_ERROR if the instruction was executed successfully, otherwise an error code.
///
/// \details This function fetches and executes the next instruction from the PVM's program counter.
/// It handles various operations including arithmetic, logical, stack, and control flow instructions.
/// The function also manages the PVM's data stack and call stack.
pvm_errno_t pvm_op(pvm_t *vm);

#endif
