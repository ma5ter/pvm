cmake_minimum_required(VERSION 3.10)

project(pvm C)

set(CMAKE_C_STANDARD 11)

add_executable(pvm
		pvm/pvm.c
		builtins.c
		main.c
)

target_include_directories(pvm PRIVATE ./)

target_compile_definitions(pvm PRIVATE
		PVM_DATA_STACK_SIZE=30
		PVM_CALL_STACK_SIZE=10
		PVM_DEBUG="pvm_debug.h"
)

add_custom_command(TARGET pvm POST_BUILD
		COMMAND size -Ax "${CMAKE_BINARY_DIR}/pvm" | grep -P "\\.pvm_|section"
		VERBATIM
)
