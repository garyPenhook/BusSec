/**
 * @file main.c
 * @author gary
 * @date 2026-04-28
 * @brief BusSec-PIC32CM firmware bring-up entry point.
 */

#include "board_led.h"
#include "bussec_cli.h"
#include "clock_config.h"
#include "host_console.h"
#include "pic32cm5164jh01048.h"
#include "system_pic32cm5164jh01048.h"
#include "tc0_periodic.h"

static void periodic_tick(void)
{
    /* TC0 owns the coarse 500 ms heartbeat used for visible liveness and CLI timekeeping. */
    board_led_toggle();
    bussec_cli_status_tick();
}

int main(void)
{
    /* Bring the chip out of the DFP reset defaults before enabling board peripherals. */
    SystemInit();
    clock_config_init_48mhz();
    board_led_init();
    host_console_init();
    bussec_cli_init();
    tc0_periodic_init_500ms(periodic_tick);

    for (;;) {
        /* Process all queued console input, then sleep until the next interrupt. */
        bussec_cli_task();
        __WFI();
    }
}
