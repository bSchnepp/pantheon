LIST(APPEND ARCH_HEADERS arch.hpp)
LIST(APPEND ARCH_HEADERS gic.hpp)
LIST(APPEND ARCH_HEADERS ints.hpp)
LIST(APPEND ARCH_HEADERS mmu.hpp)
LIST(APPEND ARCH_HEADERS sync.hpp)

LIST(APPEND ARCH_SOURCES gic.cpp)
LIST(APPEND ARCH_SOURCES ints.cpp)
LIST(APPEND ARCH_SOURCES mmu.cpp)
LIST(APPEND ARCH_SOURCES arch.cpp)

LIST(APPEND ARCH_SOURCES isr.S)
LIST(APPEND ARCH_SOURCES sched.S)
LIST(APPEND ARCH_SOURCES metadata.S)

ADD_SUBDIRECTORY(vmm)

ADD_LIBRARY(ArchLibProper STATIC
	${ARCH_HEADERS} ${ARCH_SOURCES})

TARGET_LINK_LIBRARIES(ArchLibProper PUBLIC Common VMMLib)
ADD_DEPENDENCIES(ArchLibProper sysm prgm)

ADD_LIBRARY(FreestandingArchLib STATIC gic.cpp mmu.cpp arch.cpp)