#include "pvm.h"

// ReSharper disable CppRedundantInlineSpecifier

#ifdef PVM_DEBUG
// if PVM_DEBUG is defined then user provides a header file with static functions that output opcodes debug
#include PVM_DEBUG
#else
// otherwise define empty stubs
#pragma ide diagnostic ignored "EmptyDeclOrStmt"
#define p(fmt, ...)
#define p_s(str)
#define p_begin(vm)
#define p_end(vm)
#define p_pc(pc)
#define p_pop(count)
#define p_cal(fun, args_size)
#define p_ret(pc, fun, args_size)
#define p_psh(value)
#define p_ld(op, var, value)
#define p_stv(var, value)
#define p_slp(value)
#endif

#define PVM_INTEGRAL_OP_MASK 0x0F

#ifndef section_pvm_core
#if defined(__GNUC__) || defined(__clang__)
#define section_pvm_core __attribute__((section(".pvm_core")))
#else
#define section_pvm_core
#endif
#endif

/// \brief Validates the given function index against the size of executable's function table.
///
/// \param[in] vm The PVM instance.
/// \param[in] index The function index to validate.
///
/// \return PVM_EXE_NO_FUNCTION if the index is out of bounds, otherwise PVM_NO_ERROR.
///
/// \details This function checks if the provided function index is within the valid range of the executable's functions table.
/// It returns an error if the index is negative or greater than or equal to the number of functions in the executable.
static inline section_pvm_core pvm_errno_t pvm_validate_function_index(const pvm_t *vm, const int32_t index) {
	if (index < 0 || index >= vm->persist.exe->functions_count) return PVM_EXE_NO_FUNCTION;
	return PVM_NO_ERROR;
}

/// \brief Retrieves the pointer to the constants section of the PVM executable.
///
/// \param[in] exe The PVM executable.
///
/// \return A pointer to the constants section of the executable.
///
/// \details This function calculates and returns the address of the constants section in the PVM executable.
/// The constants section follows the functions section in the executable.
static inline pvm_const_t section_pvm_core *pvm_constants(const pvm_exe_t *exe) {
	return (pvm_const_t *)&exe->functions[exe->functions_count];
}

/// \brief Retrieves the pointer to the code section of the PVM executable.
///
/// \param[in] exe The PVM executable.
///
/// \return A pointer to the code section of the executable.
///
/// \details This function calculates and returns the address of the code section in the PVM executable.
/// The code section follows the constants section in the executable.
static inline pvm_op_t section_pvm_core *pvm_code(const pvm_exe_t *exe) {
	return (pvm_op_t *)&pvm_constants(exe)[exe->constants_count];
}

/// \brief Retrieves the size of the code section in the PVM executable.
///
/// \param[in] exe The PVM executable.
///
/// \return The size of the code section in bytes.
///
/// \details This function calculates and returns the size of the code section in the PVM executable.
/// The code section size is determined by subtracting the size of the constants section from the total size of the executable.
static inline size_t section_pvm_core pvm_code_size(const pvm_exe_t *exe) {
	return exe->size - ((uint8_t *)&pvm_constants(exe)[exe->constants_count] - (uint8_t *)&exe->functions[0]);
}

/// \brief Retrieves the index of the currently executing function in the PVM.
///
/// \param[in] vm The PVM instance.
///
/// \return The index of the currently executing function, or -1 for main().
///
/// \details This function returns the index of the function that is currently being executed by the PVM.
/// If the call stack is empty, it indicates that the main() is executing, and the function returns -1.
static int section_pvm_core pvm_current_function(const pvm_t *vm) {
	// main variables start from the beginning of the data stack
	pvm_call_stack_t top;
	if (((top = vm->call_top)) && top <= PVM_CALL_STACK_SIZE) {
		return vm->call_stack[top - 1].function_index;
	}
	return -1;
}

/// \brief Retrieves the starting index of the current function's variables in the PVM data stack.
///
/// \param[in] vm The PVM instance.
///
/// \return The starting index of the current function's variables in the data stack.
///
/// \details This function returns the starting index of the variables for the currently executing function in the PVM data stack.
/// If the call stack is empty, it indicates that the main program is executing, and the function returns 0.
static pvm_data_stack_t section_pvm_core pvm_current_variables_start(const pvm_t *vm) {
	// main variables start from the beginning of the data stack
	pvm_data_stack_t offset = 0;
	pvm_call_stack_t top;
	if ((top = vm->call_top)) {
		offset = vm->call_stack[top - 1].variables_start;
	}
	return offset;
}

static section_pvm_core pvm_errno_t pvm_data_stack_push(pvm_t *vm, const pvm_data_t data) {
	if (vm->data_top >= PVM_DATA_STACK_SIZE) return PVM_DATA_STACK_OVERFLOW;
	vm->data_stack[vm->data_top++] = data;
	return PVM_NO_ERROR;
}

static section_pvm_core pvm_errno_t pvm_data_stack_pop(pvm_t *vm, int32_t *data) {
	if (vm->data_top == 0) return PVM_DATA_STACK_UNDERFLOW;
	int32_t value = vm->data_stack[--vm->data_top];
	// expand sign for shorter stack types
	#if PVM_DATA_SIGN > 0x80000000
	if (value & PVM_DATA_SIGN) {
		value |= (int32_t)PVM_DATA_SIGN;
	}
	#endif
	*data = value;
	return PVM_NO_ERROR;
}

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
enum pvm_exe_check_result section_pvm_core pvm_exe_check(const pvm_exe_t *exe, const size_t size) {
	if (exe->size != size - sizeof(exe->vm_version) - sizeof(exe->size) - sizeof(exe->functions_count) - sizeof(exe->constants_count) - sizeof(exe->main_variables_count)) return PVM_EXE_SIZE;
	if (exe->vm_version != PVM_VERSION) return PVM_EXE_VERSION;
	return PVM_EXE_OK;
}

/// \brief Resets the PVM instance to its initial state.
///
/// \param[in,out] vm The PVM instance to reset.
///
/// \details This function clears all the runtime data of the PVM instance, except for the persistent data.
/// After resetting, the data stack top is set to the number of main variables defined in the executable.
///
/// \note This function does not modify the executable or the persistent data.
void section_pvm_core pvm_reset(pvm_t *vm) {
	for (int i = 0; i < sizeof(pvm_t) - sizeof(vm->persist); ++i) {
		((uint8_t *)vm)[i] = 0;
	}
	vm->data_top = vm->persist.exe->main_variables_count;
}

/// \brief Executes the next instruction in the PVM.
///
/// \param[in,out] vm The PVM instance.
///
/// \return PVM_NO_ERROR if the instruction was executed successfully, otherwise an error code.
///
/// \details This function fetches and executes the next instruction from the PVM's program counter.
/// It handles various operations including arithmetic, logical, stack, and control flow instructions.
/// The function also manages the PVM's data stack and call stack.
pvm_errno_t section_pvm_core pvm_op(pvm_t *vm) {
	register pvm_errno_t errno;
	int32_t value;

	// check SLP timeout
	if (vm->timer) {
		const uint32_t d = now_ms() - vm->timer;
		if (d < vm->timeout) return PVM_NO_ERROR;
		vm->timer = 0;
	}

	// check pc
	if (vm->pc >= pvm_code_size(vm->persist.exe)) return PVM_PC_OVERRUN;

	p_begin(vm);

	// fetch next instruction
	const pvm_op_t op = pvm_code(vm->persist.exe)[vm->pc++];

	// process instruction
	if (op & 0x80) {
		int32_t param;
		if (op & 0x40) {
			param = op & PVM_INTEGRAL_OP_MASK;
			// check for parameter overflow
			if (param == PVM_INTEGRAL_OP_MASK) {
				// get parameter from stack when overflowed
				if ((errno = pvm_data_stack_pop(vm, &param))) return errno;
				// complete positive values
				if (param > 0) {
					param += PVM_INTEGRAL_OP_MASK;
				}
			}
			if (op & 0x20) {
				uint_fast8_t stack_size;
				const int function = pvm_current_function(vm);
				if (function < 0) {
					stack_size = vm->persist.exe->main_variables_count;
				}
				else {
					if ((errno = pvm_validate_function_index(vm, function))) return errno;
					const pvm_function_t *const pvm_function = &vm->persist.exe->functions[function];
					stack_size = pvm_function->arguments_count + pvm_function->variables_count;
				}
				if (param < 0 || param >= stack_size) return PVM_NO_VARIABLE;
				if ((param += pvm_current_variables_start(vm)) >= PVM_DATA_STACK_SIZE) return PVM_VAR_OUT_OF_STACK;
				if (op & 0x10) {
					// STV	1	1	1	1
					if ((errno = pvm_data_stack_pop(vm, &value))) return errno;
					p_stv(param, value);
					vm->data_stack[param] = value;
				}
				else {
					// LDV	1	1	1	0
					p_ld("LDV", param, vm->data_stack[param]);
					if ((errno = pvm_data_stack_push(vm, vm->data_stack[param]))) return errno;
				}
			}
			else {
				if (op & 0x10) {
					// CAL	1	1	0	1
					if ((errno = pvm_validate_function_index(vm, param))) return errno;
					if (vm->call_top >= PVM_CALL_STACK_SIZE) return PVM_CALL_STACK_OVERFLOW;
					const pvm_function_t *const fun = &vm->persist.exe->functions[param];
					// get function arguments size
					size_t args_size = fun->arguments_count;
					// for variadic functions, get number of variadic arguments from the stack
					if (fun->is_variadic) {
						pvm_data_t variadic_size;
						if ((errno = pvm_data_stack_pop(vm, &variadic_size))) return errno;
						if (variadic_size < 0 || (args_size += variadic_size) > 0xFF) return PVM_VARIADIC_SIZE;
					}
					p_cal(fun, args_size);
					// check if all arguments are in the stack
					if (vm->data_top < args_size) return PVM_ARG_OUT_OF_STACK;
					// arguments are already pushed into the stack, check for stack overflow upon function call
					const pvm_data_stack_t stack_rest = PVM_DATA_STACK_SIZE - vm->data_top;
					if (stack_rest < fun->variables_count) return PVM_VAR_OUT_OF_STACK;
					if (stack_rest < fun->returns_count) return PVM_RETURN_OUT_OF_STACK;
					// calculate function stack start
					const pvm_data_stack_t call_stack_start = vm->data_top - args_size;
					// call the function
					const pvm_address_t address = fun->address;
					if (fun->is_built_in) {
						if (address >= pvm_builtins_size) return PVM_BUILTIN_NO_FUNCTION;
						// for built-in functions, parameters and return values occupy common space
						pvm_builtins[address].func(vm, vm->data_stack + call_stack_start, args_size);
						// as no RET instruction was executed, emulate it setting the stack pointer to the number of returns
						vm->data_top = call_stack_start + fun->returns_count;
					}
					else {
						struct pvm_call_stack *call = &vm->call_stack[vm->call_top++];
						call->function_index = param;
						call->variables_start = call_stack_start;
						call->arguments_count = args_size;
						// initialize local variables with zeros and set proper stack top at once
						for (int i = 0; i < fun->variables_count; ++i) {
							if ((errno = pvm_data_stack_push(vm, 0))) return errno;
						}
						call->return_address = vm->pc;
						vm->pc = address;
					}
				}
				else {
					p_s("JMP");
					// JMP	1	1	0	0
					jump:
					if (param < 0) param -= 2;
					vm->pc += param + 1;
					p_pc(vm->pc);
				}
			}
		}
		else {
			if (op & 0x20) {
				if (op & 0x10) {
					if (op & 0x08) {
						// NEG, INV, INC, DEC, POP
						if (op & 0x04) {
							p_pop(op & 3);
							// POP
							for (int i = (op & 3) + 1; i; i--) {
								if ((errno = pvm_data_stack_pop(vm, &value))) return errno;
							}
						}
						else {
							// NEG, INV, INC, DEC
							if ((errno = pvm_data_stack_pop(vm, &value))) return errno;
							if (op & 0x02) {
								if (op & 0x01) {
									p_s("DEC");
									// DEC
									--value;
								}
								else {
									p_s("INC");
									// INC
									++value;
								}
							}
							else {
								if (op & 0x01) {
									p_s("INV");
									// INV
									value = ~value;
								}
								else {
									p_s("NEG");
									// NEG
									value = -value;
								}
							}
							goto push_value;
						}
					}
					else {
						// SKZ, SNZ, SKN, SNN, SLP, RET, LDC, JMB
						if (op & 0x04) {
							// SLP, RET, LDC, JMB
							if (op & 0x02) {
								//  LDC, JMB
								if ((errno = pvm_data_stack_pop(vm, &value))) return errno;
								if (op & 0x01) {
									p_s("JMB");
									// JMB is the same as NEG followed JMP
									param = -value;
									goto jump;
								}
								// LDC
								if (value < 0 || value >= vm->persist.exe->constants_count) return PVM_NO_CONSTANT;
								p_ld("LDC", value, pvm_constants(vm->persist.exe)[value]);
								value = pvm_constants(vm->persist.exe)[value];
								// expand sign for shorter stack types
								#if PVM_CONST_SIGN > 0x80000000
								if (value & PVM_CONST_SIGN) {
									value |= (int32_t)PVM_CONST_SIGN;
								}
								#endif
								goto push_value;
							}
							// SLP, RET
							if (op & 0x01) {
								// RET
								p_s("RET");
								int function = pvm_current_function(vm);
								if (pvm_validate_function_index(vm, function)) return PVM_MAIN_RETURN;
								// cleanup stack
								pvm_data_stack_t stack_start = pvm_current_variables_start(vm);
								const pvm_function_t *const fun = &vm->persist.exe->functions[function];
								uint8_t returns_size = fun->returns_count;
								pvm_data_stack_t returns_start = vm->data_top - returns_size;
								// no need to check vm->call_top < 0 as pvm_current_function() already checked it
								struct pvm_call_stack *const call = &vm->call_stack[--vm->call_top];
								// check for smashed stack
								if (stack_start + call->arguments_count + fun->variables_count != returns_start) return PVM_DATA_STACK_SMASHED;
								// move return values to the beginning of the function stack
								while (returns_size--) {
									vm->data_stack[stack_start++] = vm->data_stack[returns_start++];
								}
								vm->data_top = stack_start;
								// stack is guaranteed not to be empty by the 'function < 0' check
								vm->pc = call->return_address;
								p_ret(vm->pc, fun, call->arguments_count);
							}
							else {
								// SLP
								// pseudo function with one parameter
								if ((errno = pvm_data_stack_pop(vm, &value))) return errno;
								vm->timer = now_ms();
								vm->timeout = value;
								p_slp(value);
							}
						}
						else {
							// SKZ, SNZ, SKN, SNN
						}
					}
				}
				else {
					int32_t second;
					uint_fast8_t branch = 0;
					if ((errno = pvm_data_stack_pop(vm, &value))) return errno;
					if ((errno = pvm_data_stack_pop(vm, &second))) return errno;
					if (op & 0x08) {
						// ADD, SUB, MUL, DIV, PWR, AND, IOR, XOR
						if (op & 0x04) {
							// PWR, AND, IOR, XOR
							if (op & 0x02) {
								// IOR, XOR
								if (op & 0x01) {
									p_s("XOR");
									// XOR
									value ^= second;
								}
								else {
									p_s("IOR");
									// IOR
									value |= second;
								}
							}
							else {
								// PWR, AND
								if (op & 0x01) {
									p_s("AND");
									// AND
									value &= second;
								}
								else {
									p_s("PWR");
									// PWR
									if (second <= 0) {
										value = 1;
									}
									else {
										const int32_t v = value;
										while (--second) {
											value *= v;
										}
									}
								}
							}
						}
						else {
							// ADD, SUB, MUL, DIV
							if (op & 0x02) {
								// MUL, DIV
								if (op & 0x01) {
									p_s("DIV");
									// DIV
									value /= second;
								}
								else {
									p_s("MUL");
									// MUL
									value *= second;
								}
							}
							else {
								// ADD, SUB
								if (op & 0x01) {
									p_s("SUB");
									// SUB
									value -= second;
								}
								else {
									p_s("ADD");
									// ADD
									value += second;
								}
							}
						}
						goto push_value;
					}
					// BZE, BNZ, BEQ, BNE, BGT, BLT, BGE, BLE
					if ((op & 7) > 1) {
						// BEQ, BNE, BGT, BLT, BGE, BLE
						int32_t third;
						if ((errno = pvm_data_stack_pop(vm, &third))) return errno;
						second -= third;
					}
					if (op & 0x04) {
						// BGT, BLT, BGE, BLE
						if (op & 0x02) {
							// BGE, BLE
							if (op & 0x01) {
								p_s("BLE");
								// BLE
								if (second <= 0) branch |= 1;
							}
							else {
								p_s("BGE");
								// BGE
								if (second >= 0) branch |= 1;
							}
						}
						else {
							// BGT, BLT
							if (op & 0x01) {
								p_s("BLT");
								// BLT
								if (second < 0) branch |= 1;
							}
							else {
								p_s("BGT");
								// BGT
								if (second > 0) branch |= 1;
							}
						}
					}
					else {
						// BZE, BNZ, BEQ, BNE
						if (op & 0x01) {
							// BNE, BNZ
							p_s("BN*");
							if (second) branch |= 1;
						}
						else {
							// BEQ, BZE
							p_s("BZ*");
							if (second == 0) branch |= 1;
						}
					}
					if (branch & 1) {
						vm->pc += value + 1;
						p_pc(vm->pc);
					}
					else {
						p(" x");
					}
				}
			}
			else {
				// PSC
				p_s("PSC");
				if ((errno = pvm_data_stack_pop(vm, &value))) return errno;
				value <<= 5;
				value |= op & 0x1F;
				goto push_value;
			}
		}
	}
	else {
		// PSH
		value = op & 0x7F;
		p_psh(value);
		push_value:
		if ((errno = pvm_data_stack_push(vm, value))) return errno;
	}

	p_end(vm);

	return 0;
}
