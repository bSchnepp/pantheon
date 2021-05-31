# pantheon
A microkernel based OS for aarch64 machines

## Organization
Most core kernel code is located under `Common/`, with code for
various physical protocols, processes, synchronization primitives, and other
basic runtime code located there.

Code for a specific board, such as `qemu-virt` can be found under `board/`.
For now, only qemu-virt is supported, and addresses of it's peripherals,
such as the PL011 UART, are hardcoded in.

Code for a specific processor architecture is under `arch/`. For now, the only
supported processor architecture is `aarch64`, and specifically assumed to be
an A72-compatible core.

Specific CMake toolchain scripts are located under the `cmake/` directory.
For supporting a different board combination, the format `<board>-<arch>.cmake`
should be used, such as `qemu-aarch64.cmake`.

Helper scripts to help build the project with default or generic settings are
under `buildscripts/`. For example, to build a basic pantheon system, running
`./buildscripts/test-nosd.sh` will build the whole platform without any storage
devices.

Drivers for common devices, such as a PL011 UART, are located under `Devices/`
This should not be confused with driver code interacting with a physical protocol,
such as PSCI drivers or PCIe drivers.

External libraries are located under `externals/`. These should generally be
kept to a minimum, as pantheon should strive to keep as much system functionality
un usermode as possible.

Finally, some important system daemons will be located under `Userland/`. These
services include things such as graphics drivers, filesystem drivers, and so on.