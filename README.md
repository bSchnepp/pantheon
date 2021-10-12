# pantheon
A microkernel based OS for aarch64 machines

## Why?
Operating system code is one of the most fun areas to work with!
I've been fascinated with alternative designs to the traditional monolithic, *NIX-like
architecture for a while now. As it turns out, there are a lot of different
designs that radically depart from *NIX design and manage to better handle certain problems!
Something really fascinating is being able to put GPU drivers and other ASIC drivers 
entirely in userspace, and still achieve good performance out of them. A really interesting challenge
is dealing with the case of a device with three distinct displays where a process draws on all of them: 
each process to draw on all of them must have a minimum of 4 context switches per screen to handle drawing (proc --> gpu --> kernel --> proc), 
with 2 framebuffers each, meaning there would be 24 context switches, and these need to be done 60 times per second at a minimum, or a budget of at most about 700 microseconds to do work at each stage.
If the system can be carefully architected, perhaps these context switches can be mitigated, or can simply be made fast enough to not matter.
Because of this, I believe it would be a wonderful excercise to simply try to create something vaguely inspired by these, aiming to solve many of the same problems, through 
being different in some ways I think are important.

## Goals to solve
There's a couple of things that would be nice to focus on:

	- Support for ports and services, allowing a clean way to handle IPC between processes: ie, a string-based identifier to handle IPC with well-known services, like a GPU driver at "gpu:Gpu", or "gpu:Disp"
	- Support fine-grained process creation, only possible through a program launcher service ("app:App"), and no other program.
	- Handle IPC between processes in a safe and efficient manner, ideally avoiding kernel interaction at all beyond setup. Force all IPC through such protocol!
	- Permissions handled through capabilities, rather than something like file access permissions. Also allow masking of system call permissions.
	- Handle scheduling between many different processes in efficient manner to reduce *latency*
	- Expose various hardware protocols to userspace applications in a nice and fair way (ie, create daemons at "i2c:port1", "gpio:pin1", etc.)
	- Have a real wifi/ethernet driver! And be able to host a small static website from it.
	- Perhaps allow system calls to be deferred to a userland process? (ie, a process thinks it has one kernel ABI, but it actually has its syscalls routed to another process, and that process calls into the kernel. This allows execution of binaries intended for another platform)

## Organization
Most core kernel code is located under `Common/`, with code for
various physical protocols, processes, synchronization primitives, and other
basic runtime code located there.

Code for a specific board, such as `qemu-virt` can be found under `board/`.
For now, only qemu-virt is supported, and addresses of it's peripherals,
such as the PL011 UART, are hardcoded in. This should be changed to use the
device tree more properly at a later time.

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
These programs should be included directly in the kernel binary image, so that it
should not be possible to remove them without also removing the kernel: this is
how the initramfs problem is solved for pantheon.