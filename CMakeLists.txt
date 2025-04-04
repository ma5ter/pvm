cmake_minimum_required(VERSION 3.10)

project(pvm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (NOT DEFINED PVM_DATA_STACK_SIZE)
	set(PVM_DATA_STACK_SIZE 30 CACHE STRING "Size of data stack")
endif ()

if (NOT DEFINED PVM_CALL_STACK_SIZE)
	set(PVM_CALL_STACK_SIZE 10 CACHE STRING "Size of call stack")
endif ()

add_library(pvm
		pvm.c
)

if (DEFINED PVM_DEBUG)
	target_compile_definitions(pvm PRIVATE PVM_DEBUG="${PVM_DEBUG}")
endif ()

target_compile_definitions(pvm PRIVATE
		PVM_DATA_STACK_SIZE=${PVM_DATA_STACK_SIZE}
		PVM_CALL_STACK_SIZE=${PVM_CALL_STACK_SIZE}
)

add_custom_command(TARGET pvm POST_BUILD
		COMMENT "Printing valuable size information for pvm sections"
		COMMAND echo "PVM sections sizes:"
		COMMAND size -Ax "$<TARGET_FILE:pvm>" | grep -P "\\.pvm_|section"
		VERBATIM
)

target_include_directories(pvm PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

# Install targets (optional)
install(TARGETS pvm
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
)

install(FILES pvm.h
		DESTINATION include
)

add_subdirectory(samples)
