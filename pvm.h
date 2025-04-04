#ifndef PVM_PVM_H
#define PVM_PVM_H

#include <stdint.h>
#include <stddef.h>

#define PVM_VERSION 1

#ifndef PVM_DATA_STACK_SIZE
#define PVM_DATA_STACK_SIZE 30
#endif
#ifndef PVM_CALL_STACK_SIZE
#define PVM_CALL_STACK_SIZE 10
#endif

#ifndef packed_struct
#if defined(__GNUC__) || defined(__clang__)
#define packed_struct struct __attribute__((__packed__, aligned(1)))
#elif defined(_MSC_VER)
#define packed_struct __declspec(align(1)) struct
#else
#define packed_struct struct
#warning "Unknown compiler - packed_struct will not enforce packing"
#endif
#endif

/// \brief This defines a type pvm_op_t which is an 8-bit unsigned integer.
///
/// \details Represents an operation code (opcode) in the PVM bytecode. Each instruction in the bytecode is encoded as a single byte, so an 8-bit type is sufficient.
///
/// \note This type will not be extended or sprinkled in other versions of virtual machine
typedef uint8_t pvm_op_t;

/// \brief This defines a type pvm_address_t which is a 16-bit unsigned integer.
///
/// \details Represents an address within the PVM executable. This is used to reference locations in the code section,
/// constants section, or other addressable entities within the executable.
///
/// \note This type limits maximum size of the coed section and also affects executable header size
///
/// \note This type may be extended or sprinkled in other versions of virtual machine if needed
typedef uint16_t pvm_address_t;

/// \brief This defines a default data type pvm_data_t which is a 32-bit signed integer.
///
/// \details Represents data values that are pushed onto and popped from the data stack. This type holds variables,
/// loaded constants, and intermediate results of computations.
///
/// \note This type may be extended or sprinkled in other versions of virtual machine if needed
typedef int32_t pvm_data_t;

/// \brief Sign Extension macro
///
/// \detailsThe PVM_DATA_SIGN macro (0x80000000ul) is used to handle sign extension when dealing with shorter stack types.
/// This ensures that negative values are correctly interpreted.
///
/// \note Change this walue when changing the size of the `pvm_data_t` type
///
/// \example When data type is 16-bit set to the following values: \code typedef int16_t pvm_data_t;
/// #define PVM_DATA_SIGN 0xFFFF8000ul
#define PVM_DATA_SIGN 0x80000000ul

/// \brief This defines a default constant type pvm_const_t which is a 32-bit signed integer.
///
/// \details Represents constant values within the PVM executable. Constants are stored in a separate section of the
/// executable and can be loaded onto the data stack using specific instructions.
///
/// \note This type may be extended or sprinkled in other versions of virtual machine if needed
///
/// \note No sense to extend this type more than `pvm_data_t`
typedef int32_t pvm_const_t;


/// \brief Sign Extension macro for Constants
///
/// \detailsThe PVM_DATA_SIGN macro (0x80000000ul) is used to handle sign extension when dealing with shorter stack types.
/// This ensures that negative values are correctly interpreted.
///
/// \note Change this wale when changing the size of the `pvm_data_t` type
///
/// \example When data type is 16-bit set to the following values: \code typedef int16_t pvm_const_t;
/// #define PVM_CONST_SIGN 0xFFFF8000ul
#define PVM_CONST_SIGN 0x80000000ul

/// \brief This defines a type pvm_data_stack_t which is an 8-bit unsigned integer.
///
/// \details Represents an index or position within the data stack. The data stack is used to store variables, constants,
/// and intermediate results of computations.
///
/// \note This type may be extended in other versions of virtual machine if needed to have a deeper data stack
typedef uint8_t pvm_data_stack_t;

/// \brief This defines a type pvm_call_stack_t which is an 8-bit unsigned integer.
///
/// \details Represents an index or position within the call stack. The call stack is used to manage function calls and
/// returns, storing return addresses and other context information.
///
/// \note This type may be extended in other versions of virtual machine if needed to have a deeper call stack
typedef uint8_t pvm_call_stack_t;

/// \brief This defines a type pvm_function_index_t which is an 8-bit unsigned integer.
///
/// \details Represents an index into the function table within the PVM executable. Each function in the executable is
/// identified by a unique index.
///
/// \note This type may be extended in other versions of virtual machine if needed to have more built-in functions
typedef uint8_t pvm_function_index_t;
/// \brief Represents a function in the PVM executable.
///
/// \details This structure defines the attributes of a function within the PVM executable.
/// It includes the function's address, argument size, variable size, return size, and flags indicating if the function is variadic or a system library function.
///
/// \note The structure is packed to ensure efficient memory usage.
typedef packed_struct pvm_function {
	/// \brief The address of the function in the code section.
	///
	/// \details This address points to the starting bytecode of the function.
	pvm_address_t address;
	/// \brief The number of the arguments passed to the function.
	///
	/// \details This value indicates the number of bytes required to store the arguments on the data stack.
	uint8_t arguments_count;
	/// \brief The number of the local variables used by the function.
	///
	/// \details This value indicates the number of bytes required to store the local variables on the data stack.
	uint8_t variables_count;
	/// \brief The number of the return values from the function.
	///
	/// \details This value indicates the number of bytes required to store the return values on the data stack.
	uint8_t returns_count : 6;
	/// \brief A flag indicating if the function accepts a variable number of arguments.
	///
	/// \details If set, the function can accept additional arguments beyond the specified `args_size`.
	uint8_t is_variadic : 1;
	/// \brief A flag indicating if the function is a system library function.
	///
	/// \details System library functions are built-in functions provided by the PVM and are not part of the user-defined code.
	uint8_t is_built_in : 1;
} pvm_function_t;

/// \brief Represents the PVM executable.
///
/// \details This structure defines the layout of a PVM executable, including its size, minimum VM version, number of
/// functions, number of constants, and the number of main variables. It also includes an array of function descriptors,
/// constants and executable bytecode itself.
///
/// \note The structure is packed to ensure efficient memory usage.
typedef packed_struct pvm_exe {
	/// \brief The version code of the PVM required running this executable.
	///
	/// \details This field indicates the minimum version of the PVM required to execute this binary. It ensures that the executable is
	//// compatible with the version of the PVM running on the MCU. If the PVM version is lower than the specified minimum
	//// version, the executable will not be loaded.
	uint8_t vm_version;
	/// \brief The total size of the executable in bytes.
	///
	/// \details This field specifies the total size of all variable fields in the executable in bytes excluding fixed size fields. It
	/// includes the size of functions description table, constants, and code sections. This value is also used to verify the
	/// integrity of the executable during loading.
	pvm_address_t size;
	/// \brief The number of functions defined in the executable.
	///
	/// \details This field specifies the number of functions defined in the executable. Each function is described by a structure that
	/// includes its address, argument size, variable size, return size, and flags indicating if the function is variadic or a
	/// system library function. The compiler sorts them by usage and removes unused user and system built-in functions from
	/// this table.
	uint8_t functions_count;
	/// \brief The number of constants defined in the executable.
	///
	/// \details This field specifies the number of constants defined in the executable. Constants are used to store fixed values that do
	/// not change during the execution of the program. The compiler determines them walking through the compiled module then
	/// sorts by frequency of use to access efficiently. They are stored in an array following the functions section.
	uint8_t constants_count;
	/// \brief The number of main variables used by the executable.
	///
	/// \details This field specifies the number of main function variables used by the executable. Main variables are not global
	/// variables and not accessible throughout 'global' keyword. They are initialized at the start of the program with 0 and
	/// persist until the program terminates.
	uint8_t main_variables_count;
	/// \brief This variable-sized field is an array of function definitions.
	/// Each function is described by a function descriptor structure 'pvm_function'
	///
	/// \details see the description of 'pvm_function' structure
	pvm_function_t functions[];
	/// \brief The array of constants used by the executable `constants`.
	///
	/// \details This variable-sized field is an array of constants used by the executable. Constants are fixed values that do not
	/// change during the execution of the program. They are stored in an array following the functions section and are
	/// accessed using the `LDC` instruction.
	// pvm_data_t constants[];
	/// \brief The bytecode of the executable `code`.
	///
	/// \details This variable-sized field contains the bytecode of the executable. The bytecode consists of a series of 8-bit
	/// instructions that are executed by the PVM. The instructions are designed to be compact and efficient, with a focus on
	/// minimizing resource usage. The code section follows the constants section in the executable.
	// uint8_t code[];
} pvm_exe_t;

/// \brief Represents the PVM instance.
///
/// \details This structure defines the state of a PVM instance, including the timer, timeout, data stack, call stack, program counter, stack tops, and persistent data.
/// The persistent data section contains the binding and a pointer to the executable.
///
/// \note The structure is packed to ensure efficient memory usage.
typedef packed_struct pvm {
	/// \brief Timer
	///
	/// \details This field holds the current time in milliseconds. It is used to track the elapsed time for sleep instructions. The
	/// timer is set using the `now_ms` function, which returns the current time.
	uint32_t timer;
	/// \brief Timeout
	///
	/// \details This field specifies the duration for which the PVM should sleep. When a sleep instruction (`SLP`) is executed, the
	/// timeout value is set, and the PVM enters a sleep state until the specified duration has elapsed. The combination of the
	/// timer and timeout fields allows the PVM to handle delays accurately.
	uint32_t timeout;
	/// \brief Data Stack
	///
	/// \details The data stack is a crucial component of the PVM instance, used for storing temporary data during the execution of
	/// instructions. It is implemented as an array of `pvm_data_t` with a fixed size defined by `PVM_DATA_STACK_SIZE` during
	/// compile time.
	pvm_data_t data_stack[PVM_DATA_STACK_SIZE];
	/// \brief Call Stack
	///
	/// \details The call stack is used to manage function calls and returns. It is implemented as an array of structures, each
	/// containing the return address, the start of the variables in the data stack, the size of the arguments, and the
	/// function index. The call stack size is defined by `PVM_CALL_STACK_SIZE` during compile time.
	///
	/// \details The call stack allows the PVM to keep track of the execution context for each function call, enabling nested
	/// function calls and proper return handling. The stack top pointer (`call_top`) keeps track of the current position
	/// in the call stack.
	packed_struct pvm_call_stack {
		/// \brief Return Address
		///
		/// \details This field stores the address to which the program counter (PC) should return after the current
		/// function call completes. It is crucial for managing the control flow and ensuring that the execution resumes
		/// correctly after a function returns.
		pvm_address_t return_address;
		/// \brief Variables Start
		///
		/// \details This field indicates the starting index of the variables for the current function in the data stack.
		/// It helps in managing the local variables of the function and ensures that they are correctly accessed and
		/// modified during the function's execution.
		pvm_data_stack_t variables_start;
		/// \brief Actual number of arguments passed
		///
		/// \details This field specifies the actual number of the arguments passed to the current function (especially
		/// variadic ones). It is used to manage the argument stack and ensure that the correct number of arguments is
		/// pushed onto the stack before a function call and popped off the stack after the function returns.
		pvm_data_stack_t arguments_count;
		/// \brief Function Index
		///
		/// This field stores the index of the current function in the executable's function table. It is used to identify
		/// the function being executed. This index is crucial for managing the data call stack using function descriptor.
		pvm_function_index_t function_index;
	} call_stack[PVM_CALL_STACK_SIZE];
	/// \brief Program Counter (PC)
	///
	/// \details The program counter (PC) is a register that points to the address of the next instruction to be executed. It is
	/// incremented automatically as instructions are fetched and executed.
	pvm_address_t pc;
	/// \brief Data Top
	///
	/// \details This field is a pointer that keeps track of the current position in the data stack. It is incremented when data is
	/// pushed onto the stack and decremented when data is popped from the stack.
	pvm_data_stack_t data_top;
	/// \brief Call Top
	///
	/// \details This field is a pointer that keeps track of the current position in the call stack. It is incremented when a function
	/// is called and decremented when a function returns.
	pvm_call_stack_t call_top;
	/// \brief Data that persists over reset
	///
	/// \details The persistent data section ensures that the PVM instance can be reset without losing the executable and binding
	/// information, allowing for consistent execution across resets.
	packed_struct {
		/// \brief Binding
		///
		/// \details This field is used to store binding-specific data that persists across resets of the PVM instance. It is a user-defined
		/// field that can be used to store context-specific information like id of a dedicated output of the MCU this virtual
		/// machine is tied to.
		uint8_t binding;
		/// \brief Executable Pointer
		///
		//// \details This field is a pointer to the PVM executable structure described above.
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
	/// \brief Indicates that no error has occurred. This is the default success code.
	PVM_NO_ERROR = 0,
	/// \brief Indicates that the main function has returned. This is typically used to signal the end of the main program
	/// execution.
	PVM_MAIN_RETURN,
	/// \brief Indicates that the call stack has underflowed. This means that an attempt was made to pop from an empty call stack.
	/// This error code is aliased to `PVM_MAIN_RETURN`.
	PVM_CALL_STACK_UNDERFLOW = PVM_MAIN_RETURN,
	/// \brief Indicates that the call stack has overflowed. This means that an attempt was made to push more frames onto the call
	/// stack than it can hold.
	PVM_CALL_STACK_OVERFLOW,
	/// \brief Indicates that the data stack has underflowed. This means that an attempt was made to pop from an empty data stack.
	PVM_DATA_STACK_UNDERFLOW,
	/// \brief Indicates that the data stack has overflowed. This means that an attempt was made to push more values onto the data
	/// stack than it can hold.
	PVM_DATA_STACK_OVERFLOW,
	/// \brief Indicates that there are not enough stack to hold arguments for a function call. This means that an attempt was made
	/// to call a function and push more arguments into the stack than it can hold.
	PVM_ARG_OUT_OF_STACK,
	/// \brief Indicates that there are not enough stack to hold variables for a function call. This means that an attempt was made
	/// to call a function and push more variables into the stack than it can hold.
	PVM_VAR_OUT_OF_STACK,
	/// \brief Indicates that there are not enough stack to hold return values for a function call. This means that an attempt was made
	/// to call a function and push more return values into the stack than it can hold.
	PVM_RETURN_OUT_OF_STACK,
	/// \brief Indicates that the data stack has been corrupted. This means that the stack has been overwritten or otherwise
	/// tampered with, leading to an inconsistent state.
	PVM_DATA_STACK_SMASHED,
	/// \brief Indicates that the program counter (PC) has overrun. This means that the PC has exceeded the bounds of the
	/// executable code, typically due to an invalid jump or call instruction.
	PVM_PC_OVERRUN,
	/// \brief Indicates that a function index is out of bounds. This means that an attempt was made to call a function that does
	/// not exist in the executable's function table.
	PVM_EXE_NO_FUNCTION,
	/// \brief Indicates that a built-in function index is out of bounds. This means that an attempt was made to call a built-in
	/// function that does not exist in the built-in function table.
	PVM_BUILTIN_NO_FUNCTION,
	/// \brief Indicates that a variable index is out of bounds. This means that an attempt was made to access a variable that does
	/// not exist in the current scope.
	PVM_NO_VARIABLE,
	/// \brief Indicates that a constant index is out of bounds. This means that an attempt was made to access a constant that does
	/// not exist in the executable's constant table.
	PVM_NO_CONSTANT,
	/// \brief Indicates that the size of variadic arguments is incorrect. This means that the number of variadic arguments passed
	/// to a function does not match the expected number.
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
