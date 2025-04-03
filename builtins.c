#include <stdio.h>
#include "pvm/pvm.h"

#if PVM_DATA_SIGN > 0x80000000
int32_t expand(pvm_data_t value) {
	if (value & PVM_DATA_SIGN) return (int32_t)value | (int32_t)PVM_DATA_SIGN;
	return value;
}
#else
#define expand(x) (int32_t)x
#endif

#include <time.h>

uint32_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void __attribute__((section(".pvm_builtins"))) pvm_builtin_print(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	for (int i = 0; i < args_size; ++i) {
		printf(" %d", expand(arguments[i]));
	}
}

void __attribute__((section(".pvm_builtins"))) pvm_output(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	printf("OUTPUT= %d", expand(arguments[0]));
}

void pvm_get_tick(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	arguments[0] = (int32_t)now_ms();
}

void pvm_get_time(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	arguments[0] = (int32_t)ts.tv_sec;
}

void pvm_get_realtime(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
    time_t t;
	time(&t);
	const struct tm *tm = localtime(&t);

    arguments[0] = tm->tm_hour;
    arguments[1] = tm->tm_min;
    arguments[2] = tm->tm_sec;
}

void pvm_get_date(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	time_t t;
	time(&t);
	const struct tm *tm = localtime(&t);

	arguments[0] = tm->tm_year + 1900; // year
	arguments[1] = tm->tm_mon + 1; // month
	arguments[2] = tm->tm_mday; // date
}

void pvm_get_weekday(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	time_t t;
	time(&t);
	const struct tm *tm = localtime(&t);

	arguments[0] = tm->tm_wday; // weekday
}

void __attribute__((section(".pvm_builtins"))) pvm_sh_section_state(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	arguments[0] = 2;
}

void __attribute__((section(".pvm_builtins"))) pvm_sh_get_entry_timer(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	arguments[0] = 0;
}

void __attribute__((section(".pvm_builtins"))) pvm_sh_get_exit_timer(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	arguments[0] = 0;
}


const packed_struct pvm_builtins pvm_builtins[] = {
{ pvm_builtin_print },
{ pvm_output },
{ pvm_get_tick },
{ pvm_get_time },
{ pvm_get_realtime },
{ pvm_get_date },
{ pvm_get_weekday },
{ pvm_sh_get_entry_timer },
{ pvm_sh_get_exit_timer },
{ pvm_sh_section_state },
};

const size_t pvm_builtins_size = sizeof(pvm_builtins) / sizeof(pvm_builtins[0]);
