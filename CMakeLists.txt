###############################################################################
#
# (c) Copyright IBM Corp. 2017
#
#  This program and the accompanying materials are made available
#  under the terms of the Eclipse Public License v1.0 and
#  Apache License v2.0 which accompanies this distribution.
#
#      The Eclipse Public License is available at
#      http://www.eclipse.org/legal/epl-v10.html
#
#      The Apache License v2.0 is available at
#      http://www.opensource.org/licenses/apache2.0.php
#
# Contributors:
#    Multiple authors (IBM Corp.) - initial implementation and documentation
###############################################################################

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

# Warn user that CMake support is currently experimental
message(WARNING
"CMake support is currently experimental.\n\
Various components may fail to compile with default configuration options, and some are not even attempted.")

#TODO figure out method of sharing version info with makefile build
set(OMR_VERSION_MAJOR 0)
set(OMR_VERSION_MINOR 0)
set(OMR_VERSION_PATCH 1)
set(OMR_VERSION ${OMR_VERSION_MAJOR}.${OMR_VERSION_MINOR}.${OMR_VERSION_PATCH})

project(omr VERSION ${OMR_VERSION} LANGUAGES CXX C)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

###
### Getting the glue directory
###

if (NOT DEFINED OMR_GLUE)
    set(OMR_GLUE "./example/glue" CACHE PATH "The glue directory")
    message(STATUS "Glue not set, defaulting to example glue")
endif()

# TODO: OMR_EXAMPLE flag
# TODO: OMR_RTTI flag

# TODO: Version things

set(OMR_VERSION_STRING "<Unknown>" CACHE STRING "") # TODO: Set the version
set(OMR_JIT_VERSION_STRING "<Unknown>" CACHE STRING "") # TODO: Set the version

###
### Versions and stuff
###

include(cmake/platform.cmake)
include(cmake/config.cmake)

enable_testing()

#include current source dir and current bin dir automatically
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# VPathResolve takes a list of source files, a list of directories to search in,
# and produces a list of source files, prepended with the directory in which the
# file was found. This is inspired by the VPATH system of Make.
#
# ie. if you have a/a.c and b/b.c in your directories, and
#
#     inlist  = [a.c, b.c]
#     dirlist = [a, b]
#
# then
#
#     outlist = [a/a.c, b/b.c]
#
function(VPathResolve inlist dirlist outlist)
    set(output "")
  foreach(currentFile ${${inlist}})
    set(fileFound False)
    foreach(currentDir ${${dirlist}})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${currentDir}/${currentFile}")
            list(APPEND output "${currentDir}/${currentFile}")
            set(fileFound True)
            break()
        endif()
    endforeach()
    if(NOT fileFound)
        message(FATAL_ERROR "Could not find file ${currentFile}")
    endif()
  endforeach()
  set(${outlist} ${output} PARENT_SCOPE)
endfunction()

include_directories(
  ${PROJECT_BINARY_DIR}
  ./include/
  ./include_core/
  ./third_party/
)

add_definitions(
  -DUT_DIRECT_TRACE_REGISTRATION # TODO:  Deal with that stupid jni issue in tracegen
)

# Check for existing omrcfg in the source tree since this can cause alot of headaches
# Also check if we are building in tree while we are at it
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	if(NOT MSVC_IDE) #MSVC will handle in tree builds ok
		message(WARNING "In tree builds are not recommended")
	endif()
else()
include(cmake/CheckSourceTree.cmake)
add_custom_target(header_check ALL
	${CMAKE_COMMAND} "-Domr_SOURCE_DIR=${omr_SOURCE_DIR}" -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckSourceTree.cmake)
endif()

configure_file(./omrcfg.CMakeTemplate.h omrcfg.h)
configure_file(./omrversionstrings.CMakeTemplate.h omrversionstrings.h)

add_subdirectory(tools)
# Yeah, its dumb doing this here. Read note in tools/CMakeLists.txt
if(CMAKE_CROSSCOMPILING)
	include(${OMR_TOOLS_IMPORTFILE})
endif()
add_subdirectory(thread)
add_subdirectory(port)
add_subdirectory(util)
add_subdirectory(omrtrace)
add_subdirectory(omr)
add_subdirectory(third_party)
add_subdirectory(omrsigcompat)

#TODO should be wrapped in if()
add_subdirectory(fvtest)

if(OMR_GC)
  add_subdirectory(gc)
  add_subdirectory("${OMR_GLUE}")
endif(OMR_GC)

if(OMR_COMPILER)
  # TODO: Actually support the compiler
  add_subdirectory(compiler)
endif(OMR_COMPILER)
