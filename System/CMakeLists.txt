LIST(APPEND SYSTEM_HEADERS Syscalls/Syscalls.hpp)
LIST(APPEND SYSTEM_SOURCES Syscalls/Syscalls.cpp)

LIST(APPEND SYSTEM_HEADERS IPC/kern_event.hpp)
LIST(APPEND SYSTEM_SOURCES IPC/kern_event.cpp)

LIST(APPEND SYSTEM_HEADERS IPC/kern_port.hpp)
LIST(APPEND SYSTEM_SOURCES IPC/kern_port.cpp)

LIST(APPEND SYSTEM_HEADERS IPC/kern_client_port.hpp)
LIST(APPEND SYSTEM_SOURCES IPC/kern_client_port.cpp)

LIST(APPEND SYSTEM_HEADERS IPC/kern_client_connection.hpp)
LIST(APPEND SYSTEM_SOURCES IPC/kern_client_connection.cpp)

LIST(APPEND SYSTEM_HEADERS IPC/kern_server_port.hpp)
LIST(APPEND SYSTEM_SOURCES IPC/kern_server_port.cpp)

LIST(APPEND SYSTEM_HEADERS IPC/kern_server_connection.hpp)
LIST(APPEND SYSTEM_SOURCES IPC/kern_server_connection.cpp)

LIST(APPEND SYSTEM_HEADERS IPC/kern_connection.hpp)
LIST(APPEND SYSTEM_SOURCES IPC/kern_connection.cpp)

LIST(APPEND SYSTEM_HEADERS IPC/kern_packet.hpp)
LIST(APPEND SYSTEM_SOURCES IPC/kern_packet.cpp)

LIST(APPEND SYSTEM_HEADERS Handle/kern_handle.hpp)
LIST(APPEND SYSTEM_SOURCES Handle/kern_handle.cpp)

LIST(APPEND SYSTEM_HEADERS Handle/kern_handletable.hpp)
LIST(APPEND SYSTEM_SOURCES Handle/kern_handletable.cpp)

LIST(APPEND SYSTEM_HEADERS Exec/kern_elf.hpp)
LIST(APPEND SYSTEM_SOURCES Exec/kern_elf.cpp)

LIST(APPEND SYSTEM_HEADERS Memory/kern_alloc.hpp)
LIST(APPEND SYSTEM_SOURCES Memory/kern_alloc.cpp)

LIST(APPEND SYSTEM_HEADERS Exec/kern_initialprograms.hpp)
LIST(APPEND SYSTEM_SOURCES Exec/kern_initialprograms.cpp)

INCLUDE_DIRECTORIES(PhyProtocol)
ADD_SUBDIRECTORY(PhyProtocol)

ADD_LIBRARY(System STATIC
	${SYSTEM_HEADERS}
	${SYSTEM_SOURCES})

INCLUDE_DIRECTORIES(Proc)
ADD_SUBDIRECTORY(Proc)

TARGET_LINK_LIBRARIES(System PRIVATE Common Proc PhyProtocol)