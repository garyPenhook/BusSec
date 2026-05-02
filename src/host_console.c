#include "host_console.h"

#include <stddef.h>
#include <stdint.h>

#include "clock_config.h"
#include "pic32cm5164jh01048.h"

/*
 * EV29G58A routes debugger CDC TX to PA00 and debugger CDC RX to PA01.
 * SERCOM1 can receive on PA00/PAD0, but the PIC32CM USART cannot transmit on
 * PAD1, so target TX on PA01 is a blocking GPIO UART.
 */
#define HOST_CONSOLE_RX_RING_SIZE (1024u)
#define HOST_CONSOLE_RX_RING_MASK (HOST_CONSOLE_RX_RING_SIZE - 1u)
#define HOST_CONSOLE_RX_PIN       (0u)
#define HOST_CONSOLE_TX_PIN       (1u)
#define HOST_CONSOLE_RX_PMUX_INDEX (HOST_CONSOLE_RX_PIN / 2u)
#define HOST_CONSOLE_RX_MASK      (UINT32_C(1) << HOST_CONSOLE_RX_PIN)
#define HOST_CONSOLE_TX_MASK      (UINT32_C(1) << HOST_CONSOLE_TX_PIN)
#define HOST_CONSOLE_TX_BIT_CYCLES \
    ((APP_CPU_CLOCK_HZ + (HOST_CONSOLE_BAUD / 2u)) / HOST_CONSOLE_BAUD)
#define HOST_CONSOLE_SYNC_TIMEOUT (UINT32_C(1000000))
#define HOST_CONSOLE_USART_ERROR_STATUS_MASK \
    (SERCOM_USART_INT_STATUS_PERR_Msk | SERCOM_USART_INT_STATUS_FERR_Msk | \
     SERCOM_USART_INT_STATUS_BUFOVF_Msk | SERCOM_USART_INT_STATUS_ISF_Msk | \
     SERCOM_USART_INT_STATUS_COLL_Msk)

_Static_assert((HOST_CONSOLE_RX_RING_SIZE & HOST_CONSOLE_RX_RING_MASK) == 0u,
               "HOST_CONSOLE_RX_RING_SIZE must be a power of two");
_Static_assert(HOST_CONSOLE_TX_BIT_CYCLES < SysTick_LOAD_RELOAD_Msk,
               "HOST_CONSOLE_TX_BIT_CYCLES must fit in the SysTick counter");

/* RX is interrupt-driven so the foreground CLI can sleep between commands. */
static uint8_t host_console_rx_ring[HOST_CONSOLE_RX_RING_SIZE];
static volatile uint16_t host_console_rx_head;
static volatile uint16_t host_console_rx_tail;
static volatile uint32_t host_console_rx_overflows;
static volatile uint32_t host_console_rx_errors;

static void host_console_fault_halt(void)
{
    __disable_irq();
    for (;;) {
        __NOP();
    }
}

static void host_console_wait_sync(void)
{
    /* SERCOM control registers cross clock domains; wait after reset/enable/config writes. */
    for (uint32_t timeout = HOST_CONSOLE_SYNC_TIMEOUT; timeout != 0u; timeout--) {
        if ((SERCOM1_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_Msk) == 0u) {
            return;
        }
    }

    host_console_fault_halt();
}

static uint16_t host_console_baud_reg(uint32_t baud)
{
    /* Arithmetic baud mode: BAUD = 65536 * (1 - 16 * baud / fref), rounded to nearest. */
    const uint64_t sample_rate = UINT64_C(16) * (uint64_t)baud;
    const uint64_t scaled = ((UINT64_C(65536) * sample_rate) + (APP_CPU_CLOCK_HZ / 2u)) /
                            APP_CPU_CLOCK_HZ;

    return (uint16_t)(UINT64_C(65536) - scaled);
}

static void host_console_configure_pins(void)
{
    uint8_t pmux = PORT_REGS->GROUP[0].PORT_PMUX[HOST_CONSOLE_RX_PMUX_INDEX];

    /* PA00 uses peripheral function D for SERCOM1 PAD0 RX. */
    pmux = (uint8_t)((pmux & (uint8_t)~PORT_PMUX_PMUXE_Msk) | PORT_PMUX_PMUXE_D);
    PORT_REGS->GROUP[0].PORT_PMUX[HOST_CONSOLE_RX_PMUX_INDEX] = pmux;
    PORT_REGS->GROUP[0].PORT_PINCFG[HOST_CONSOLE_RX_PIN] =
        (uint8_t)(PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_INEN_Msk);

    /* PA01 is target TX to the debugger CDC RX input; idle high as GPIO. */
    PORT_IOBUS_REGS->GROUP[0].PORT_OUTSET = HOST_CONSOLE_TX_MASK;
    PORT_IOBUS_REGS->GROUP[0].PORT_DIRSET = HOST_CONSOLE_TX_MASK;
    PORT_REGS->GROUP[0].PORT_PINCFG[HOST_CONSOLE_TX_PIN] = 0u;
}

static void host_console_tx_timer_init(void)
{
    SysTick->LOAD = SysTick_LOAD_RELOAD_Msk;
    SysTick->VAL = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

static void host_console_delay_cycles(uint32_t cycles)
{
    const uint32_t start = SysTick->VAL;

    while (((start - SysTick->VAL) & SysTick_LOAD_RELOAD_Msk) < cycles) {
    }
}

static void host_console_tx_level(bool high)
{
    if (high) {
        PORT_IOBUS_REGS->GROUP[0].PORT_OUTSET = HOST_CONSOLE_TX_MASK;
    } else {
        PORT_IOBUS_REGS->GROUP[0].PORT_OUTCLR = HOST_CONSOLE_TX_MASK;
    }
}

void host_console_init(void)
{
    host_console_rx_head = 0u;
    host_console_rx_tail = 0u;
    host_console_rx_overflows = 0u;
    host_console_rx_errors = 0u;

    host_console_configure_pins();
    host_console_tx_timer_init();

    /* Feed SERCOM1 from GCLK0, which clock_config_init_48mhz() sets to 48 MHz. */
    MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_SERCOM1_Msk;
    GCLK_REGS->GCLK_PCHCTRL[SERCOM1_GCLK_ID_CORE] = GCLK_PCHCTRL_GEN_GCLK0 |
                                                     GCLK_PCHCTRL_CHEN_Msk;
    for (uint32_t timeout = HOST_CONSOLE_SYNC_TIMEOUT; timeout != 0u; timeout--) {
        if ((GCLK_REGS->GCLK_PCHCTRL[SERCOM1_GCLK_ID_CORE] & GCLK_PCHCTRL_CHEN_Msk) != 0u) {
            break;
        }
        if (timeout == 1u) {
            host_console_fault_halt();
        }
    }

    SERCOM1_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_SWRST_Msk;
    host_console_wait_sync();

    /* Internal-clock USART, 8N1, LSB first, RX on PAD0. TX is GPIO bit-banged. */
    SERCOM1_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK |
                                            SERCOM_USART_INT_CTRLA_SAMPR_16X_ARITHMETIC |
                                            SERCOM_USART_INT_CTRLA_TXPO_PAD0 |
                                            SERCOM_USART_INT_CTRLA_RXPO_PAD0 |
                                            SERCOM_USART_INT_CTRLA_FORM_USART_FRAME_NO_PARITY |
                                            SERCOM_USART_INT_CTRLA_IBON_Msk |
                                            SERCOM_USART_INT_CTRLA_DORD_LSB;
    SERCOM1_REGS->USART_INT.SERCOM_BAUD = host_console_baud_reg(HOST_CONSOLE_BAUD);
    SERCOM1_REGS->USART_INT.SERCOM_CTRLB = SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT |
                                            SERCOM_USART_INT_CTRLB_RXEN_Msk;
    host_console_wait_sync();

    /* RX complete wakes the CLI; ERROR lets the ISR clear framing/overflow conditions. */
    SERCOM1_REGS->USART_INT.SERCOM_INTFLAG = SERCOM_USART_INT_INTFLAG_Msk;
    SERCOM1_REGS->USART_INT.SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_RXC_Msk |
                                               SERCOM_USART_INT_INTENSET_ERROR_Msk;

    NVIC_ClearPendingIRQ(SERCOM1_IRQn);
    NVIC_EnableIRQ(SERCOM1_IRQn);

    SERCOM1_REGS->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE_Msk;
    host_console_wait_sync();
}

void host_console_putc(char ch)
{
    uint8_t frame = (uint8_t)ch;
    uint32_t primask;

    /* Most terminal programs expect CRLF line endings on a CDC UART console. */
    if (ch == '\n') {
        host_console_putc('\r');
    }

    primask = __get_PRIMASK();
    __disable_irq();

    host_console_tx_level(false);
    host_console_delay_cycles(HOST_CONSOLE_TX_BIT_CYCLES);

    for (uint32_t bit = 0u; bit < 8u; bit++) {
        host_console_tx_level((frame & UINT8_C(1)) != 0u);
        frame >>= 1u;
        host_console_delay_cycles(HOST_CONSOLE_TX_BIT_CYCLES);
    }

    host_console_tx_level(true);
    host_console_delay_cycles(HOST_CONSOLE_TX_BIT_CYCLES);

    __set_PRIMASK(primask);
}

void host_console_write(const char *text)
{
    while (*text != '\0') {
        host_console_putc(*text);
        text++;
    }
}

bool host_console_read_byte(uint8_t *out)
{
    /* Single foreground consumer, single ISR producer: 16-bit indices are atomic on Cortex-M0+. */
    const uint16_t tail = host_console_rx_tail;

    if (tail == host_console_rx_head) {
        return false;
    }

    *out = host_console_rx_ring[tail];
    host_console_rx_tail = (uint16_t)((tail + 1u) & HOST_CONSOLE_RX_RING_MASK);
    return true;
}

bool host_console_has_pending_rx(void)
{
    return host_console_rx_tail != host_console_rx_head;
}

uint32_t host_console_rx_overflow_count(void)
{
    return host_console_rx_overflows;
}

uint32_t host_console_rx_error_count(void)
{
    return host_console_rx_errors;
}

void SERCOM1_Handler(void)
{
    const uint8_t intflag = SERCOM1_REGS->USART_INT.SERCOM_INTFLAG;

    if ((intflag & SERCOM_USART_INT_INTFLAG_ERROR_Msk) != 0u) {
        const uint16_t status = SERCOM1_REGS->USART_INT.SERCOM_STATUS;

        if ((status & HOST_CONSOLE_USART_ERROR_STATUS_MASK) != 0u) {
            host_console_rx_errors++;
        }
        if ((status & SERCOM_USART_INT_STATUS_BUFOVF_Msk) != 0u) {
            host_console_rx_overflows++;
        }

        SERCOM1_REGS->USART_INT.SERCOM_INTFLAG = SERCOM_USART_INT_INTFLAG_ERROR_Msk;
        SERCOM1_REGS->USART_INT.SERCOM_STATUS = (uint16_t)HOST_CONSOLE_USART_ERROR_STATUS_MASK;
        while ((SERCOM1_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_RXC_Msk) != 0u) {
            (void)SERCOM1_REGS->USART_INT.SERCOM_DATA;
        }
        return;
    }

    if ((intflag & SERCOM_USART_INT_INTFLAG_RXC_Msk) != 0u) {
        /* Reading DATA acknowledges RXC; store the byte unless the ring is full. */
        const uint8_t byte = (uint8_t)SERCOM1_REGS->USART_INT.SERCOM_DATA;
        const uint16_t next_head = (uint16_t)((host_console_rx_head + 1u) &
                                             HOST_CONSOLE_RX_RING_MASK);

        if (next_head == host_console_rx_tail) {
            host_console_rx_overflows++;
        } else {
            host_console_rx_ring[host_console_rx_head] = byte;
            host_console_rx_head = next_head;
        }
    }
}
