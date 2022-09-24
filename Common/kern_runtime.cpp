#include <kern.h>
#include <kern_runtime.hpp>
#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#include <Sync/kern_spinlock.hpp>
#include <System/Proc/kern_cpu.hpp>
#include <System/Memory/kern_alloc.hpp>

#include <printf/printf.h>
#include <BoardRuntime/BoardRT.hpp>

void SERIAL_LOG_UNSAFE(const char *Fmt, ...)
{
	if (pantheon::Panicked())
	{
		return;
	}
	
	va_list Args;
	va_start(Args, Fmt);
	vprintf(Fmt, Args);
	va_end(Args);	
}

static pantheon::Spinlock PanicMutex;
static pantheon::Spinlock PrintMutex;

void SERIAL_LOG(const char *Fmt, ...)
{
	if (pantheon::Panicked())
	{
		return;
	}

	PrintMutex.Acquire();
	va_list Args;

	va_start(Args, Fmt);
	vprintf(Fmt, Args);
	va_end(Args);
	PrintMutex.Release();
}

[[noreturn]]
void pantheon::StopError(const char *Reason, void *Source)
{
	PanicMutex.Acquire();
	if (Reason)
	{
		if (Source)
		{
			SERIAL_LOG_UNSAFE("panic: %s [source: %lx, core %x]\n", Reason, Source, pantheon::CPU::GetProcessorNumber());
		}
		else
		{
			SERIAL_LOG_UNSAFE("panic: %s [core %lx]\n", Reason, pantheon::CPU::GetProcessorNumber());
		}
	}
	else
	{
		SERIAL_LOG_UNSAFE("panic: %s [core %lx]\n", "unknown reason", pantheon::CPU::GetProcessorNumber());
	}
	
	/* TODO: stop other cores */
	pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_PANIC);
	PanicMutex.Release();
	pantheon::CPU::CLI();
	for (;;){};
}

[[noreturn]]
void pantheon::StopErrorFmt(const char *Fmt, ...)
{
	PanicMutex.Acquire();
	SERIAL_LOG_UNSAFE("panic: ");
	va_list Args;
	va_start(Args, Fmt);
	vprintf(Fmt, Args);
	va_end(Args);

	/* TODO: stop other cores */
	pantheon::SetKernelStatus(pantheon::KERNEL_STATUS_PANIC);
	PanicMutex.Release();
	pantheon::CPU::CLI();
	for (;;){};
}

BOOL pantheon::Panicked()
{
	return pantheon::GetKernelStatus() == KERNEL_STATUS_PANIC;
}

void pantheon::InitBasicRuntime()
{
	PrintMutex = pantheon::Spinlock("print_mutex");
	PanicMutex = pantheon::Spinlock("panic_mutex");
}

void _putchar(char c)
{
	if (c == '\n')
	{
		PUTCHAR_FUNC('\r');
	}
	PUTCHAR_FUNC(c);
}