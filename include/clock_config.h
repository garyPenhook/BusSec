#ifndef CLOCK_CONFIG_H
#define CLOCK_CONFIG_H

#include <stdint.h>

/* Project clock contract shared by baud-rate and timer calculations. */
#define APP_CPU_CLOCK_HZ (UINT32_C(48000000))

/* Switch the device from the DFP reset default clock to a 48 MHz OSC48M setup. */
void clock_config_init_48mhz(void);

#endif /* CLOCK_CONFIG_H */
