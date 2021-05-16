#include <kern_datatypes.hpp>

namespace pantheon
{

namespace arm
{

VOID LoadInterruptTable(VOID *Table);

}

}

extern "C" void sync_handler_el1_sp0();
extern "C" void err_handler_el1_sp0();
extern "C" void fiq_handler_el1_sp0();
extern "C" void irq_handler_handler_el1_sp0();
extern "C" void sync_handler_el1();
extern "C" void err_handler_el1();
extern "C" void fiq_handler_el1();
extern "C" void irq_handler_handler_el1();
extern "C" void sync_handler_el0();
extern "C" void err_handler_el0();
extern "C" void fiq_handler_el0();
extern "C" void irq_handler_handler_el0();
extern "C" void sync_handler_el0_32();
extern "C" void err_handler_el0_32();
extern "C" void fiq_handler_el0_32();
extern "C" void irq_handler_handler_el0_32();