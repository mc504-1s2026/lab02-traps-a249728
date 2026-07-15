#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

#define CMD_BUF_SIZE 128
#define CHUNK_SIZE 128

extern int _hartid[];

void kmain()
{
	char chunk[CHUNK_SIZE];
	char cmd[CMD_BUF_SIZE];
	size_t cmd_len = 0;

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

	print("> ");
	while (1) {
		size_t n = serial_read(chunk);
		
		for (size_t i = 0; i < n; i++) {
			char c = chunk[i];
			
			serial_putc(c); // Envia o caractere recebido de volta para a porta serial

            // Se o comando estiver completo, processa-o
			if (c == '\r') {
				cmd[cmd_len] = '\0'; // Transforma o buffer em uma string
				print("\n");
				
				if (strncmp(cmd, "uptime", 6) == 0) { // Comando para mostrar o tempo de execução
					u64 secs = timer_read() / TIMER_FREQ;
					printk(LOG_INFO, "%lus\n", secs);
				}  
				else if (strncmp(cmd, "echo ", 5) == 0) { // Comando para escrever uma mensagem
					printk(LOG_INFO, "%s\n", cmd + 5);
				} 
				else if (strncmp(cmd, "alarm ", 6) == 0) { // Comando para configurar um alarme
					u64 secs = strtou64(cmd + 6, 10);
					timer_set_alarm(secs);
					timer_irq_enable();
				}

				cmd_len = 0; // Limpa para receber a próxima linha
				print("> ");
			} 
			else if (cmd_len < CMD_BUF_SIZE - 1) { 
				cmd[cmd_len++] = c;
			}
		}
	}

}
