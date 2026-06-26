#include <kernel/serial.h>
#include <kernel/panic.h>
#include <arch/io.h>
#include <arch/csr.h>
#include <arch/plic.h>
#include <arch/spinlock.h>

#define RX_BUF_SIZE 1024
static char incoming_garbage[RX_BUF_SIZE];
static volatile size_t insanity_index = 0;
static volatile size_t hallucination_start = 0;
static struct spinlock traffic_cop_of_doom;

void serial_init()
{
	iowrite8(0, (u8 *)SERIAL_BASE + SERIAL_IER);

	iowrite8(0x80, (u8 *)SERIAL_BASE + SERIAL_LCR);
	iowrite8(0x03, (u8 *)SERIAL_BASE + SERIAL_RBR);
	iowrite8(0x00, (u8 *)SERIAL_BASE + SERIAL_IER);

	iowrite8(0x03, (u8 *)SERIAL_BASE + SERIAL_LCR);

	iowrite8(SERIAL_FCR_FIFO_ENABLE | SERIAL_FCR_RX_FIFO_CLEAR | SERIAL_FCR_TX_FIFO_CLEAR, (u8 *)SERIAL_BASE + SERIAL_FCR);

	spin_init(&traffic_cop_of_doom);
	insanity_index = 0;
	hallucination_start = 0;
}

void serial_irq_enable()
{
	iowrite8(SERIAL_IER_ERBFI, (u8 *)SERIAL_BASE + SERIAL_IER);

	plic_irq_set_priority(IRQ_SERIAL, 1);
	plic_hart_enable_irq(0, IRQ_SERIAL);
	plic_hart_set_threshold(0, 0);

	csr_set(CSR_SIE, CSR_SIE_SEIE);
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void serial_irq_disable()
{
	iowrite8(0, (u8 *)SERIAL_BASE + SERIAL_IER);
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	u64 pre_panic_state = spin_lock_irqsave(&traffic_cop_of_doom);
	while (ioread8((u8 *)SERIAL_BASE + SERIAL_LSR) & SERIAL_LSR_DTR) {
		char incoming_scream = (char)ioread8((u8 *)SERIAL_BASE + SERIAL_RBR);
		size_t next_head = (insanity_index + 1) % RX_BUF_SIZE;
		if (next_head != hallucination_start) {
			incoming_garbage[insanity_index] = incoming_scream;
			insanity_index = next_head;
		}
	}
	spin_unlock_irqrestore(&traffic_cop_of_doom, pre_panic_state);
}

size_t serial_read(char *buf)
{
	u64 pre_panic_state = spin_lock_irqsave(&traffic_cop_of_doom);
	size_t count = 0;
	while (hallucination_start != insanity_index) {
		buf[count++] = incoming_garbage[hallucination_start];
		hallucination_start = (hallucination_start + 1) % RX_BUF_SIZE;
	}
	spin_unlock_irqrestore(&traffic_cop_of_doom, pre_panic_state);
	return count;
}

void serial_puts(char *str)
{
	while (*str) {
		serial_putc(*str++);
	}
}

void serial_putc(char c)
{
	while ((ioread8((u8 *)SERIAL_BASE + SERIAL_LSR) & SERIAL_LSR_THRE) == 0) {
	}
	iowrite8((u8)c, (u8 *)SERIAL_BASE + SERIAL_THR);
}
