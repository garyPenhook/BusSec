#include "board_led.h"

#include <stdint.h>

#include "pic32cm5164jh01048.h"

/* The Curiosity Nano+ Touch yellow user LED is PA19, active-low. */
#define BOARD_LED_PORT_GROUP (0u)
#define BOARD_LED_PIN        (19u)
#define BOARD_LED_MASK       (UINT32_C(1) << BOARD_LED_PIN)

void board_led_init(void)
{
    /* Drive high before enabling output so the active-low LED starts off without a glitch. */
    PORT_IOBUS_REGS->GROUP[BOARD_LED_PORT_GROUP].PORT_OUTSET = BOARD_LED_MASK;
    PORT_IOBUS_REGS->GROUP[BOARD_LED_PORT_GROUP].PORT_DIRSET = BOARD_LED_MASK;
}

void board_led_toggle(void)
{
    /* Use the atomic toggle alias instead of a read-modify-write on OUT. */
    PORT_IOBUS_REGS->GROUP[BOARD_LED_PORT_GROUP].PORT_OUTTGL = BOARD_LED_MASK;
}
