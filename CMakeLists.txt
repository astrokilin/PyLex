cmake_minimum_required(VERSION 3.14)
project(PyLex)
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(reg_compiler_flags " ")

set(reg_compiler_src 
		"c/dfa_table.c" 
		"c/reg_ast/reg_ast.c" 
		"c/data_structs/set/object_set/object_set.c" 
		"c/data_structs/set/long_set/long_set.c" 
		"c/data_structs/queue/obj_queue.c" 
		"c/data_structs/list/list.c"
		)

set(reg_compiler_includes
		"c/" 
		"c/reg_ast/" 
		"c/data_structs/set/long_set/" 
		"c/data_structs/set/object_set/" 
		"c/data_structs/list/" 
		"c/data_structs/queue/"
		)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # using Clang
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(reg_compiler_flags "-Wall -Werror -Wextra -pedantic -g")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # using Visual Studio C++
endif()

add_library(reg_compiler OBJECT ${reg_compiler_src})

target_include_directories(reg_compiler PRIVATE ${reg_compiler_includes})

set_target_properties(
	reg_compiler
    PROPERTIES
        PREFIX ""
        OUTPUT_NAME "reg_compiler"
		POSITION_INDEPENDENT_CODE ON
		COMPILE_FLAGS ${reg_compiler_flags}
        LINKER_LANGUAGE C
		)


set(pylex_src 
		"cpy/pylex_module.c" 
		"cpy/Pattern/pattern_py_type.c" 
		"cpy/Matcher/matcher_py_type.c"
		)

set (pylex_src_includes 
		"cpy/Pattern/" 
		"cpy/Matcher/" 
		"c/"
		)

find_package(PythonLibs REQUIRED)

add_library(PyLex SHARED ${pylex_src})

target_include_directories(PyLex PRIVATE ${PYTHON_INCLUDE_DIR} ${pylex_src_includes})

set_target_properties(
	PyLex
    PROPERTIES
        PREFIX ""
        OUTPUT_NAME "pylex"
        LINKER_LANGUAGE C
        LINK_LIBRARIES "${PYTHON_LIBRARIES};reg_compiler"
        # COMPILE_DEFINITIONS "Py_LIMITED_API=0x03090000" # (broke in default installation)
		)
