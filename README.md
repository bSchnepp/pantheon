# pantheon
A microkernel based OS for aarch64 machines

## Why?
Operating systems tend to be one of the most fun and exiciting areas to work with: lots of ideas can be explored here with wide-ranging effects on what a given computing platform can do.
Being inspired by some of the developments of cutting-edge microkernel operating systems, most of which radically depart from UNIX-style design, it would be worthwhile to try them out, and experiment with a platform of my own.

Something really fascinating is being able to put GPU drivers and other ASIC drivers entirely in userspace, and still achieve good performance out of them. A really interesting challenge is finding efficient ways to control peripherals
like a display controller or other graphics accelerator safely in userspace, while avoiding context switches as much as possible. Likewise, many of the services traditionally associated with the kernel itself could equivalently be expressed through small userspace servers using RPCs to communicate with each other: this drags in the entire field of network operating systems and other distributed systems theory in, which is something I find fascinating.

## IPC Design
The main method of communication between processes is a Port, which requires use of a thread-local area for each thread issuing system calls, where the kernel does not typically have to validate any of the data on the fast path to be structured in any particular way. A message is typically structured by having a small header, a description of kernel-validated and enforced handles, and then data to be sent between processes: if no handles are bundled, then only the header has to be correct, which will be enforced by the kernel anyway to designate boundaries and size limits and other data for the message. These get transferred into the thread local area for the other process, which can access and manipulate the data accordingly: specific formats for accessing handles inside the IPC messages are yet to be done, but this scheme is complete enough to at least transfer data between two known programs.

## Goals to solve
There's a couple of things that would be nice to focus on:

	- Support for ports and services, allowing a clean way to handle IPC between processes: ie, a string-based identifier to handle IPC with well-known services, like a GPU driver at "gpu:Gpu", or "gpu:Disp"
	- Support fine-grained process creation, only possible through a program launcher service ("app:App"), and no other program.
	- Handle IPC between processes in a safe and efficient manner, ideally avoiding kernel interaction at all beyond setup. Force all IPC through such protocol!
	- Permissions handled through capabilities, rather than something like file access permissions. Also allow masking of system call permissions.
	- Handle scheduling between many different processes in efficient manner to reduce *latency*
	- Expose various hardware protocols to userspace applications in a nice and fair way (ie, create daemons at "i2c:port1", "gpio:pin1", etc.)
	- Have a real wifi/ethernet driver! And be able to host a small static website from it.
	- Perhaps allow system calls to be deferred to a userland process? (ie, a process thinks it has one kernel ABI, but it actually has its syscalls routed to another process, and that process calls into the kernel. This allows execution of binaries intended for another platform, or emulation of another OS's APIs)
	- Support DG2 graphics, possibly by porting i915 code into a userland GPU service. Requires complete PCIe driver first though.

## Organization
Most core kernel code is located under `Common/`, with code for
various physical protocols, processes, synchronization primitives, and other
basic runtime code located there.

Code for a specific board, such as `qemu-virt` can be found under `BoardSupport/`.
For now, only qemu-virt is supported, and addresses of it's peripherals,
such as the PL011 UART, are hardcoded in. This should be changed to use the
device tree more properly at a later time. A branch exists for a port to the BCM2711 platform,
but this is incomplete.

Code for a specific processor architecture is under `arch/`. For now, the only
supported processor architecture is `aarch64`, and specifically assumed to be
an A72-compatible core (at least Aarch64 v8.0).

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
These programs should be included directly in the kernel binary image, so that it
should not be possible to remove them without also removing the kernel: this is
how the initramfs problem is solved for pantheon.