#include <kernel/serial.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <arch/io.h>
#include <arch/plic.h>
#include <arch/spinlock.h>

#define SERIAL_REG(offset) ((void*)((u64)SERIAL_BASE + (offset)))
#define SERIAL_BUF_SIZE 128

static char serial_buf[SERIAL_BUF_SIZE];
static size_t serial_len;
static struct spinlock serial_lock;

// Inicializa a porta serial
void serial_init()
{
	spin_init(&serial_lock);
    serial_len = 0;

    iowrite8(SERIAL_FCR_FIFO_ENABLE | SERIAL_FCR_RX_FIFO_CLEAR | SERIAL_FCR_TX_FIFO_CLEAR, SERIAL_REG(SERIAL_FCR));
    iowrite8(SERIAL_IER_ERBFI, SERIAL_REG(SERIAL_IER));
}

// Configura como lidar com interrupções da porta serial
void serial_irq_enable()
{
    plic_irq_set_priority(IRQ_SERIAL, 1);
    plic_hart_enable_irq(0, IRQ_SERIAL);
    plic_hart_set_threshold(0, 0);
    csr_set(CSR_SIE, CSR_SIE_SEIE);
}

// Desliga as interrupções externas
void serial_irq_disable()
{
    csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

// Guarda os dados recebidos via interrupção em um buffer
void serial_irq()
{
    spin_lock(&serial_lock);

    while (ioread8(SERIAL_REG(SERIAL_LSR)) & SERIAL_LSR_DTR) {
        char c = (char) ioread8(SERIAL_REG(SERIAL_RBR));
        if (serial_len < SERIAL_BUF_SIZE)
            serial_buf[serial_len++] = c;
    }

    spin_unlock(&serial_lock);
}

// Lê os dados do buffer para um ponteiro externo e retorna seu tamanho
size_t serial_read(char *buf)
{
    size_t size;

    spin_lock_irq(&serial_lock);
    size = serial_len;
    for (size_t i = 0; i < size; i++)
        buf[i] = serial_buf[i];
    serial_len = 0; 
    spin_unlock_irq(&serial_lock);

    return size;
}

// Envia uma string
void serial_puts(char *str)
{
    while (*str != '\0') {
        serial_putc(*str);
        str++;
    }
}

// Envia um caractere
void serial_putc(char c)
{
    while (!(ioread8(SERIAL_REG(SERIAL_LSR)) & SERIAL_LSR_THRE)) { }
    iowrite8((u8) c, SERIAL_REG(SERIAL_THR));
}
