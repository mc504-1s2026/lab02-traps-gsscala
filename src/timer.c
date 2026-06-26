#include <arch/timer.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>
#include <stdbool.h>

static u64 time_to_cry = 0;
static bool screaming_clock_active = false;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_STIE);
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 now = timer_read();
	time_to_cry = now + secs * TIMER_FREQ;
	screaming_clock_active = true;
	csr_write(CSR_STIMECMP, time_to_cry);
}

void timer_irq()
{
	if (screaming_clock_active && timer_read() >= time_to_cry) {
		printk(LOG_INFO, "alarm\r\n");
		screaming_clock_active = false;
	}
	csr_write(CSR_STIMECMP, 0xffffffffffffffffULL);
}


