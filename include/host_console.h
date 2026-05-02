#ifndef HOST_CONSOLE_H
#define HOST_CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

/* Debugger CDC baud used by the host console. Kept software-TX-friendly. */
#define HOST_CONSOLE_BAUD (UINT32_C(115200))

/* SERCOM1 RX plus software TX console connected to the debugger CDC port. */
void host_console_init(void);
void host_console_putc(char ch);
void host_console_write(const char *text);
/* Return one queued RX byte if the ISR has received one. */
bool host_console_read_byte(uint8_t *out);
bool host_console_has_pending_rx(void);
uint32_t host_console_rx_overflow_count(void);
uint32_t host_console_rx_error_count(void);

#endif /* HOST_CONSOLE_H */
