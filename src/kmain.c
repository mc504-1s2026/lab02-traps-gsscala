#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

static void execute_command(char *cmd)
{
	while (*cmd == ' ') {
		cmd++;
	}
	if (*cmd == '\0') {
		return;
	}

	char *args = cmd;
	while (*args != '\0' && *args != ' ') {
		args++;
	}

	if (*args == ' ') {
		*args = '\0';
		args++;
		while (*args == ' ') {
			args++;
		}
	} else {
		args = "";
	}

	if (strcmp(cmd, "uptime") == 0) {
		u64 secs = timer_read() / TIMER_FREQ;
		printk(LOG_INFO, "%llds\r\n", secs);
	} else if (strcmp(cmd, "echo") == 0) {
		printk(LOG_INFO, "%s\r\n", args);
	} else if (strcmp(cmd, "alarm") == 0) {
		u64 secs = strtou64(args, 10);
		timer_set_alarm(secs);
	} else {
		printk(LOG_INFO, "unknown command: %s\r\n", cmd);
	}
}

extern int _hartid[];
void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	char word_vomit[1024];
	size_t vomit_len = 0;

	printk(LOG_INFO, "> ");
	while (1) {
		char pain_buffer[128];
		size_t pain_count = serial_read(pain_buffer);
		if (pain_count > 0) {
			for (size_t i = 0; i < pain_count; i++) {
				char incoming_scream = pain_buffer[i];
				if (incoming_scream == '\r' || incoming_scream == '\n') {
					serial_puts("\r\n");
					word_vomit[vomit_len] = '\0';
					if (vomit_len > 0) {
						execute_command(word_vomit);
					}
					vomit_len = 0;
					printk(LOG_INFO, "> ");
				} else if (incoming_scream == 0x7f || incoming_scream == '\b') {
					if (vomit_len > 0) {
						vomit_len--;
						serial_puts("\b \b");
					}
				} else {
					if (vomit_len < sizeof(word_vomit) - 1) {
						word_vomit[vomit_len++] = incoming_scream;
						serial_putc(incoming_scream);
					}
				}
			}
		} else {
			__asm__ volatile("wfi");
		}
	}
}
