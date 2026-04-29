#ifndef HOST_CONSOLE_H
#define HOST_CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

/* High-speed CDC UART baud used by the host console. */
#define HOST_CONSOLE_BAUD (UINT32_C(921600))

/* Interrupt-driven SERCOM1 console connected to the on-board debugger CDC port. */
void host_console_init(void);
void host_console_putc(char ch);
void host_console_write(const char *text);
/* Return one queued RX byte if the ISR has received one. */
bool host_console_read_byte(uint8_t *out);
uint32_t host_console_rx_overflow_count(void);

#endif /* HOST_CONSOLE_H */
