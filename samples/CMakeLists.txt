cmake_minimum_required(VERSION 3.10)

project(pvm-sample C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(pvm-sample
		builtins.c
		main.c
)

target_link_libraries(pvm-sample PRIVATE pvm)

if (DEFINED PVM_DEBUG)
	target_compile_definitions(pvm-sample PRIVATE PVM_DEBUG)
endif ()
