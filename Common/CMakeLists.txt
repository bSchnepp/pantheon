LIST(APPEND COMMON_HEADERS kern_integers.hpp)
LIST(APPEND COMMON_HEADERS kern_macro.hpp)
LIST(APPEND COMMON_HEADERS kern_status.hpp)
LIST(APPEND COMMON_HEADERS kern_container.hpp)
LIST(APPEND COMMON_HEADERS kern_datatypes.hpp)
LIST(APPEND COMMON_HEADERS kern_runtime.hpp)
LIST(APPEND COMMON_HEADERS kern_string.hpp)
LIST(APPEND COMMON_HEADERS kern_object.hpp)
LIST(APPEND COMMON_HEADERS Sync/kern_spinlock.hpp)
LIST(APPEND COMMON_HEADERS Sync/kern_atomic.hpp)
LIST(APPEND COMMON_HEADERS Structures/kern_optional.hpp)
LIST(APPEND COMMON_HEADERS Structures/kern_bitmap.hpp)
LIST(APPEND COMMON_HEADERS Structures/kern_rawbitmap.hpp)
LIST(APPEND COMMON_HEADERS Structures/kern_slab.hpp)
LIST(APPEND COMMON_HEADERS Structures/kern_linkedlist.hpp)


LIST(APPEND COMMON_SOURCES kern_runtime.cpp)
LIST(APPEND COMMON_SOURCES kern_basic_malloc.cpp)
LIST(APPEND COMMON_SOURCES kern_status.cpp)
LIST(APPEND COMMON_SOURCES kern_string.cpp)
LIST(APPEND COMMON_SOURCES kern_object.cpp)
LIST(APPEND COMMON_SOURCES Sync/kern_spinlock.cpp)
LIST(APPEND COMMON_SOURCES Structures/kern_bitmap.cpp)
LIST(APPEND COMMON_SOURCES Structures/kern_rawbitmap.cpp)

INCLUDE_DIRECTORIES(../externals)

ADD_SUBDIRECTORY(CxxRuntime)

ADD_LIBRARY(Common STATIC
	${COMMON_HEADERS}
	${COMMON_SOURCES})

TARGET_LINK_LIBRARIES(Common PRIVATE Printf)
TARGET_LINK_LIBRARIES(Common PUBLIC CxxRuntime)