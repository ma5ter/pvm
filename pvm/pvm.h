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

typedef packed_struct pvm_function {
	pvm_address_t address;
	uint8_t args_size;
	uint8_t variables_size;
	uint8_t returns_size : 6;
	uint8_t variadic : 1;
	uint8_t sys_lib : 1;
} pvm_function_t;

typedef packed_struct pvm_exe {
	pvm_address_t size;
	uint8_t min_vm_version;
	uint8_t functions_size;
	uint8_t constants_size;
	uint8_t main_variables_count;
	pvm_function_t functions[];
} pvm_exe_t;


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

typedef void(pvm_builtin_f(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size));

extern const packed_struct pvm_builtins {
	pvm_builtin_f *func;
} pvm_builtins[];

extern const size_t pvm_builtins_size;

enum pvm_exe_check_result {
	PVM_EXE_OK = 0,
	PVM_EXE_SIZE,
	PVM_EXE_VERSION
} pvm_exe_check(const pvm_exe_t *exe, size_t size);

void pvm_reset(pvm_t *vm);

pvm_errno_t pvm_op(pvm_t *vm);

extern uint32_t now_ms(void);

#endif
