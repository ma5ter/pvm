#ifndef PVM_DEBUG_H
#define PVM_DEBUG_H

#include "pvm.h"
#include <stdio.h>

#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#define p(fmt, ...) printf(fmt, ##__VA_ARGS__)

static void __attribute__((section(".pvm_debug"))) p_s(const char *op) {
	p("%s", op);
}

static void __attribute__((section(".pvm_debug"))) p_begin(const pvm_t *vm) {
	p("PC:%u ", vm->pc);
}

static void __attribute__((section(".pvm_debug"))) p_end(const pvm_t *vm) {
	p(" {");
	for (int i = vm->data_top - 1; i >= 0; --i) {
		p("%d", vm->data_stack[i]);
		if (i) p(", ");
	}
	p("}\n");
}

static void __attribute__((section(".pvm_debug"))) p_pc(const pvm_address_t pc) {
	p(" <%u>", pc);
}

static void __attribute__((section(".pvm_debug"))) p_pop(uint8_t count) {
	p("POP X←(%u)", count);
}

static void __attribute__((section(".pvm_debug"))) p_cal(const pvm_function_t *const fun, uint8_t args_size) {
	p("CAL <%s%u> (%u) =", fun->sys_lib ? "*" : "", fun->address, args_size + fun->variables_size);
}

static void __attribute__((section(".pvm_debug"))) p_ret(pvm_address_t pc, const pvm_function_t *const fun, uint8_t args_size) {
	p(" <%u> (%u+%u)", pc, args_size + fun->variables_size, fun->returns_size);
}

static void __attribute__((section(".pvm_debug"))) p_psh(uint8_t value) {
	p("PSH %u →", value);
}

static void __attribute__((section(".pvm_debug"))) p_ld(const char *op, int var, int value) {
	p("%s [%d] %d →", op, var, value);
}

static void __attribute__((section(".pvm_debug"))) p_stv(int var, int value) {
	p("STV [%d] %d ←", var, value);
}

static void __attribute__((section(".pvm_debug"))) p_slp(int32_t value) {
	p("SLP %d", value);
}



#endif
