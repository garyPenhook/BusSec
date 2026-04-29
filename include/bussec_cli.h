#ifndef BUSSEC_CLI_H
#define BUSSEC_CLI_H

#include <stdint.h>

/* Minimal UART command interface used during firmware bring-up. */
void bussec_cli_init(void);
void bussec_cli_task(void);
/* Called by the periodic timer callback to advance CLI status timekeeping. */
void bussec_cli_status_tick(void);

#endif /* BUSSEC_CLI_H */
