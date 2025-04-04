#include <stdio.h>
#include <time.h>
#include "pvm.h"

#if PVM_DATA_SIGN > 0x80000000
int32_t expand(pvm_data_t value) {
	if (value & PVM_DATA_SIGN) return (int32_t)value | (int32_t)PVM_DATA_SIGN;
	return value;
}
#else
#define expand(x) (int32_t)x
#endif

#ifdef PVM_DEBUG
// don't terminate strings when opcode debug is printed
#define ENDL ""
#else
// terminate strings when no opcode debug is printed
#define ENDL "\n"
#endif


/// \brief A necessary to implement function that is used for SLP instruction functionality.
/// It returns the current time in milliseconds since an unspecified starting point, which is not affected by system time changes.
///
/// \return The current time span in milliseconds.
///
/// \details The example function uses the clock_gettime() system call to get the current time in seconds and nanoseconds.
/// It then converts the seconds to milliseconds and adds the milliseconds obtained from the nanoseconds.
/// The result is returned as uint32_t, which means that the function will overflow after approximately 49.7 days of continuous operation.
///
/// \note The returned value is not the actual time of day, but rather the time elapsed since an unspecified starting point.
uint32_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void __attribute__((section(".pvm_builtins"))) pvm_builtin_print(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	#ifndef PVM_DEBUG
	printf(":");
	#endif
	if (args_size) {
		printf("%d", expand(arguments[0]));
		for (int i = 1; i < args_size; ++i) {
			printf(" %d", expand(arguments[i]));
		}
	}
	#ifndef PVM_DEBUG
	printf(ENDL);
	#endif
}

void __attribute__((section(".pvm_builtins"))) pvm_output(pvm_t *vm, pvm_data_t arguments[], pvm_data_stack_t args_size) {
	printf("OUTPUT: %d" ENDL, expand(arguments[0]));
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
