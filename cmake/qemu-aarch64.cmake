# Assume aarch64 and not EFI for now.

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_PROCESSOR aarch64)
SET(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_ASM_COMPILER "aarch64-none-elf-as")
SET(CMAKE_LINKER "ld.lld")
SET(CMAKE_OBJCOPY "aarch64-none-elf-objcopy")

SET(BOOT_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/BoardSupport/qemu-aarch64/linkin.ld)
SET(KLINKER_SCRIPT ${CMAKE_SOURCE_DIR}/BoardSupport/klinkin.ld)

SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}")
SET(CMAKE_C_FLAGS "-target aarch64-none-elf -fPIE -mgeneral-regs-only -ffreestanding -nostdlib -c -Wall -Wextra -Wvla -mcpu=cortex-a72 -mtune=cortex-a72 -fno-builtin -fstack-protector ${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "-target aarch64-none-elf -fPIE -mgeneral-regs-only -ffreestanding -nostdlib -c -Wall -Wextra -Wvla -fno-rtti -fno-exceptions -mcpu=cortex-a72 -mtune=cortex-a72 -fno-builtin -fstack-protector ${CMAKE_CXX_FLAGS}")
SET(FSANITIZE_FLAGS "-fsanitize=undefined")

SET(TARGET_SYSTEM "qemu-aarch64-virt")
SET(TARGET_SYSTEM_QEMU_AARCH64_VIRT ON)
SET(TARGET_SYSTEM_BOARD "BoardSupport/qemu-aarch64")

# Be sure about the name...
SET(TARGET_PROCESSOR "aarch64")
SET(TARGET_PROCESSOR_AARCH64 ON)

# Don't even try with host stuff.
SET(CMAKE_SHARED_LIBRARY_PREFIX "")
SET(CMAKE_SHARED_MODULE_PREFIX "")


# Force the appropriate linker usage.
SET(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> -m aarch64elf -pie -nostdlib <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
SET(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> -m aarch64elf -pie -nostdlib <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

