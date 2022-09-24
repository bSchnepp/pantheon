ADD_SUBDIRECTORY(Boot)
ADD_SUBDIRECTORY(BoardInit)
ADD_SUBDIRECTORY(BoardRuntime)

ADD_EXECUTABLE(pantheon cmake_stub.cpp)
SET_TARGET_PROPERTIES(pantheon PROPERTIES LINK_DEPENDS ${BOOT_LINKER_SCRIPT})

# We do however need to link the bootloader, and have a dependency set for it.
TARGET_LINK_LIBRARIES(pantheon Boot)
TARGET_LINK_OPTIONS(pantheon PRIVATE "-T${BOOT_LINKER_SCRIPT}")

ADD_CUSTOM_COMMAND(TARGET pantheon POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/pantheon ${CMAKE_BINARY_DIR}/kernel.img)
