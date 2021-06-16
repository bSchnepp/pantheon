CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)

SET(CMAKE_C_STANDARD 11)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

SET(CMAKE_INSTALL_MESSAGE NEVER)

PROJECT(pantheon CXX C ASM)

# FIXME: Follow that embedded code standard with clang-tidy. 
# This means re-allowing any and all reserved namespace headers 
# (ie, _KERN_MACROS_HPP_ is technically in violation of the C standard), 
# and dropping the printf library because it assumes the same thing.
SET(CMAKE_CXX_CLANG_TIDY clang-tidy -header-filter=. -warnings-as-errors=* -checks=zircon*,cert*,bugprone*,clang-analyzer*,clang-analyzer-cplusplus*,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp,-cert-dcl50-cpp,concurrency*,modernize-use-nodiscard,modernize-loop-convert,modernize-use-noexcept,modernize-use-nullptr,modernize-use-override,performance*,readability-avoid-const-params-in-decls,readability-const-return-type,readability-function-cognitive-complexity,readability-function-size,readability-make-member-function-const,readability-misleading-indentation,readability-non-const-parameter,readability-simplify-boolean-expr,-cert-err58-cpp,-performance-no-int-to-ptr,-readability-function-cognitive-complexity)
SET(CMAKE_C_CLANG_TIDY clang-tidy -header-filter=. -warnings-as-errors=* -checks=zircon*,cert*,bugprone*,clang-analyzer*,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp,-cert-dcl50-cpp,concurrency*,performance*,readability-avoid-const-params-in-decls,readability-const-return-type,readability-function-cognitive-complexity,readability-function-size,readability-make-member-function-const,readability-misleading-indentation,readability-non-const-parameter,readability-simplify-boolean-expr,-bugprone-narrowing-conversions,-readability-function-cognitive-complexity,-performance-no-int-to-ptr)

SET(PANTHEON_VERSION_MAJOR 0)
SET(PANTHEON_VERSION_MINOR 0)
SET(PANTHEON_VERSION_PATCH 1)

CONFIGURE_FILE(
	"${CMAKE_SOURCE_DIR}/kern.h.in"
	"${CMAKE_BINARY_DIR}/kern.h"
	@ONLY)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
INCLUDE_DIRECTORIES(Common)


# Get definitions specific to this board, or the mock board for tests
IF (${ONLY_TESTS})
	# Include mock definitions for arch.hpp
	FIND_PACKAGE(Threads REQUIRED)
	FIND_PACKAGE(GTest CONFIG REQUIRED)
	
	INCLUDE_DIRECTORIES(board/mock)
	INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
	
	ADD_SUBDIRECTORY(board/mock)
	ADD_SUBDIRECTORY(tests)
ELSE()
	ADD_SUBDIRECTORY(${TARGET_SYSTEM_BOARD})
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/arch/${CMAKE_SYSTEM_PROCESSOR})

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

ADD_SUBDIRECTORY(arch)
ADD_SUBDIRECTORY(Common)
ADD_SUBDIRECTORY(Devices)

# Also go and add in some external dependencies...
INCLUDE_DIRECTORIES(externals/printf)
ADD_SUBDIRECTORY(externals)

ADD_EXECUTABLE(pantheon kern_main.cpp cpprt.cpp)

IF (NOT ${ONLY_TESTS})
	SET_TARGET_PROPERTIES(pantheon PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
ELSEIF(${ONLY_TESTS})
	TARGET_LINK_LIBRARIES(pantheon Tests)
ENDIF()

TARGET_LINK_LIBRARIES(pantheon ArchLib)
TARGET_LINK_LIBRARIES(pantheon Common)
TARGET_LINK_LIBRARIES(pantheon Devices)
TARGET_LINK_LIBRARIES(pantheon Board)
TARGET_LINK_LIBRARIES(pantheon Printf)

ADD_CUSTOM_COMMAND(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME} kernel.img)
