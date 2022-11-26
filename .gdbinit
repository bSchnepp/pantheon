set confirm off
set architecture aarch64
set disassemble-next-line auto

add-symbol-file ./build/pantheon -o 0x0000000040080000
add-symbol-file ./build/pkernel -o 0xFFFFFFFF70000000
break StopError
break StopErrorFmt

target ext :1234