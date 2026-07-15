#include <kernel/trap.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <arch/plic.h>
#include <arch/timer.h>
#include <kernel/printf.h>
#include <kernel/serial.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

// Roteia interrupções assíncronas para o handler apropriado
void handle_irq()
{
	u64 scause = csr_read(CSR_SCAUSE); // Causa

	if (scause == TRAP_TIMER_IRQ) { // Interrupção de timer
		timer_irq();
	} 
	else if (scause == TRAP_EXTERNAL_IRQ) { // Interrupção externa
		u32 irq = plic_hart_claim_irq(0);
		if (irq == IRQ_SERIAL)
			serial_irq();
		plic_hart_complete_irq(0, irq);
	} 
	else {
		error("unhandled interrupt: scause=0x%x\n", scause);
		BUG();
	}
}

// Trata erros síncronos de execução
void handle_exception()
{
	u64 scause = csr_read(CSR_SCAUSE); // Causa
	u64 stval = csr_read(CSR_STVAL); // Endereço de falha
	u64 sepc = csr_read(CSR_SEPC); // Endereço da instrução que causou a falha

	switch (scause) {
	case EXCEPTION_INST_ACCESS_FAULT:
		error("instruction access fault: 0x%x, sepc = 0x%x\n", stval, sepc);
		break;
	case EXCEPTION_LOAD_ACCESS_FAULT:
		error("load access fault: 0x%x, sepc = 0x%x\n", stval, sepc);
		break;
	case EXCEPTION_STORE_ACCESS_FAULT:
		error("store access fault: 0x%x, sepc = 0x%x\n", stval, sepc);
		break;
	case EXCEPTION_INST_PAGE_FAULT:
		error("instruction page fault: 0x%x, sepc = 0x%x\n", stval, sepc);
		break;
	case EXCEPTION_LOAD_PAGE_FAULT:
		error("load page fault: 0x%x, sepc = 0x%x\n", stval, sepc);
		break;
	case EXCEPTION_STORE_PAGE_FAULT:
		error("store page fault: 0x%x, sepc = 0x%x\n", stval, sepc);
		break;
	default:
		error("uncaught exception: 0x%x, sepc = 0x%x\n", scause, sepc);
	}

	panic("unhandled exception\n");
}

// Inicializa o sistema de traps
void trap_setup()
{
	csr_write(CSR_STVEC, trap_entry);
	hart_irq_enable();
}

// Roteia traps para o handler apropriado
void handle_trap()
{
	u64 scause = csr_read(CSR_SCAUSE); // Causa

	if (scause & TRAP_IRQ_BIT) // Interrupção
		handle_irq();
	else // Exceção
		handle_exception();
}

// Liga as interrupções
void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

// Salva o estado das interrupções e desliga as interrupções
u64 hart_irq_save()
{
	u64 sstatus = csr_read(CSR_SSTATUS);
	hart_irq_disable();
	return sstatus & CSR_SSTATUS_SIE;
}

// Restaura o estado das interrupções
void hart_irq_restore(u64 flags)
{
	if (flags & CSR_SSTATUS_SIE) 
		hart_irq_enable();
}

// Desliga as interrupções
void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
