CMAKE_MINIMUM_REQUIRED(VERSION 3.13 FATAL_ERROR)

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
SET(CMAKE_CXX_CLANG_TIDY clang-tidy -header-filter=. -warnings-as-errors=* -checks=zircon*,cert*,bugprone*,clang-analyzer*,clang-analyzer-cplusplus*,-bugprone-easily-swappable-parameters,-bugprone-reserved-identifier,-cert-dcl37-c,-cert-dcl51-cpp,-cert-dcl50-cpp,concurrency*,modernize-use-nodiscard,modernize-loop-convert,modernize-use-noexcept,modernize-use-nullptr,modernize-use-override,performance*,readability-avoid-const-params-in-decls,readability-const-return-type,readability-function-cognitive-complexity,readability-function-size,readability-make-member-function-const,readability-misleading-indentation,readability-non-const-parameter,readability-simplify-boolean-expr,-cert-err58-cpp,-performance-no-int-to-ptr,-readability-function-cognitive-complexity)
SET(CMAKE_C_CLANG_TIDY clang-tidy -header-filter=. -warnings-as-errors=* -checks=zircon*,cert*,bugprone*,clang-analyzer*,-bugprone-reserved-identifier,-bugprone-easily-swappable-parameters,-cert-dcl37-c,-cert-dcl51-cpp,-cert-dcl50-cpp,concurrency*,performance*,readability-avoid-const-params-in-decls,readability-const-return-type,readability-function-cognitive-complexity,readability-function-size,readability-make-member-function-const,readability-misleading-indentation,readability-non-const-parameter,readability-simplify-boolean-expr,-bugprone-narrowing-conversions,-readability-function-cognitive-complexity,-performance-no-int-to-ptr)

SET(PANTHEON_VERSION_MAJOR 0)
SET(PANTHEON_VERSION_MINOR 0)
SET(PANTHEON_VERSION_PATCH 1)

SET(POISON_MEMORY TRUE)
SET(FSANITIZE TRUE)

CONFIGURE_FILE(
	"${CMAKE_SOURCE_DIR}/kern.h.in"
	"${CMAKE_BINARY_DIR}/kern.h"
	@ONLY)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/arch/${CMAKE_SYSTEM_PROCESSOR})

INCLUDE_DIRECTORIES(Common)
INCLUDE_DIRECTORIES(System)
INCLUDE_DIRECTORIES(BoardSupport/Common)
INCLUDE_DIRECTORIES(${TARGET_SYSTEM_BOARD})


IF (${FSANITIZE})
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FSANITIZE_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FSANITIZE_FLAGS}")
ENDIF()

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

ADD_SUBDIRECTORY(Common)
ADD_SUBDIRECTORY(Devices)
ADD_SUBDIRECTORY(System)

ADD_SUBDIRECTORY(arch)

# Get definitions specific to this board, or the mock board for tests
IF (${ONLY_TESTS})
	# Include mock definitions for arch.hpp
	FIND_PACKAGE(Threads REQUIRED)
	FIND_PACKAGE(GTest CONFIG)
	
	INCLUDE_DIRECTORIES(BoardSupport/mock)
	INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
	
	ADD_SUBDIRECTORY(BoardSupport/mock)
	ADD_SUBDIRECTORY(tests)
ELSE()
	ADD_SUBDIRECTORY(${TARGET_SYSTEM_BOARD})
	# Userland MUST be built first for link order to make sense.
	ADD_SUBDIRECTORY(Userland)
ENDIF()

IF (${ONLY_TESTS})
	ADD_DEPENDENCIES(ArchLibProper sysm prgm)
ENDIF()

# Also go and add in some external dependencies...
INCLUDE_DIRECTORIES(externals/printf)
ADD_SUBDIRECTORY(externals)

ADD_EXECUTABLE(pkernel kern_main.cpp cpprt.cpp ubsan.cpp)

IF (${ONLY_TESTS})
	TARGET_LINK_LIBRARIES(pkernel PUBLIC Tests)
ELSE()
	TARGET_LINK_OPTIONS(pkernel BEFORE PUBLIC "-T${KLINKER_SCRIPT}")
	# ArchLib doesn't make sense without at least initial userland.
	# But we no longer need to link to libUserland.a, since these are separate ELFs.
	TARGET_LINK_LIBRARIES(pkernel PUBLIC ArchLib)
	TARGET_LINK_LIBRARIES(pkernel PUBLIC BoardRT)
ENDIF()

TARGET_LINK_LIBRARIES(pkernel PUBLIC Common)
TARGET_LINK_LIBRARIES(pkernel PUBLIC Devices)
TARGET_LINK_LIBRARIES(pkernel PUBLIC Board)
TARGET_LINK_LIBRARIES(pkernel PUBLIC Printf)
TARGET_LINK_LIBRARIES(pkernel PUBLIC System)

ADD_CUSTOM_COMMAND(TARGET pkernel POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -O binary --set-section-flags .bss=alloc,load,contents ${CMAKE_BINARY_DIR}/pkernel ${CMAKE_BINARY_DIR}/pkernel.img)
