#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include "pvm.h"

const char *pvm_errno_strings[] = {
"No error",
"Main function returned",
"Call stack overflow",
"Data stack underflow",
"Data stack overflow",
"Argument out of stack",
"Variable out of stack",
"Return out of stack",
"Data stack smashed",
"Program counter overrun",
"Executable has no function",
"Built-in has no function",
"No variable",
"No constant",
"Variadic size"
};

uint8_t *read_exe(const char *filename) {
	FILE *file;
	long file_size;

	file = fopen(filename, "rb");
	if (!file) {
		perror("Failed to open file");
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t *data = (uint8_t *)malloc(file_size);
	if (!data) {
		perror("Failed to allocate memory");
		fclose(file);
		return NULL;
	}

	if (fread(data, 1, file_size, file) != file_size) {
		perror("Failed to read file");
		free(data);
		fclose(file);
		return NULL;
	}

	fclose(file);

	if (pvm_exe_check((const pvm_exe_t *)data, file_size)) {
		free(data);
		fprintf(stderr, "Invalid exe\n");
		return NULL;
	}

	return data;
}

pvm_t pvm[1];

int main(const int argc, const char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	pvm_t *vm = &pvm[0];
	vm->persist.exe = (const pvm_exe_t *)read_exe(argv[1]);
	if (!vm->persist.exe) return 1;
	pvm_reset(vm);

	printf("MIN_VM_VERSION: %u\nFUNCTIONS: %u\nCONSTANTS:%u\n", vm->persist.exe->min_vm_version, vm->persist.exe->functions_size, vm->persist.exe->constants_size);

	int err = 0;
	while (!(err = pvm_op(vm))) {
		// emulate MCU speed
		usleep(10);
	}

	free((void *)vm->persist.exe);

	if (PVM_MAIN_RETURN == err) {
		printf("\nEND\n");
		return 0;
	}
	else {
		printf("\nERROR: %s PC=%u\n", pvm_errno_strings[err], vm->pc - 1);
		pvm_reset(vm);
		return 1;
	}
}
