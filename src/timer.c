#include <arch/timer.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>

// Retorna o tempo atual do sistema
u64 timer_read()
{
    return csr_read(CSR_TIME);
}

// Habilita as interrupções do timer
void timer_irq_enable()
{
    csr_set(CSR_SIE, CSR_SIE_STIE);
}

// Desabilita as interrupções do timer
void timer_irq_disable()
{
    csr_clear(CSR_SIE, CSR_SIE_STIE);
}

// Programa o timer para disparar uma interrupção após secs segundos
void timer_set_alarm(u64 secs)
{
    u64 now = csr_read(CSR_TIME);
    csr_write(CSR_STIMECMP, now + secs * TIMER_FREQ);
}

// Rotina executada quando o alarme dispara
void timer_irq()
{
    print("alarm\n");
    timer_irq_disable();
}