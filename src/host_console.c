#include "host_console.h"

#include <stddef.h>
#include <stdint.h>

#include "clock_config.h"
#include "pic32cm5164jh01048.h"

/* SERCOM1 is wired to the on-board debugger CDC UART on the Curiosity Nano+ Touch. */
#define HOST_CONSOLE_RX_RING_SIZE (1024u)
#define HOST_CONSOLE_TX_PIN       (1u)
#define HOST_CONSOLE_RX_PIN       (0u)
#define HOST_CONSOLE_PMUX_INDEX   (HOST_CONSOLE_TX_PIN / 2u)

/* RX is interrupt-driven so the foreground CLI can sleep between commands. */
static uint8_t host_console_rx_ring[HOST_CONSOLE_RX_RING_SIZE];
static volatile uint16_t host_console_rx_head;
static volatile uint16_t host_console_rx_tail;
static volatile uint32_t host_console_rx_overflows;

static void host_console_wait_sync(void)
{
    /* SERCOM control registers cross clock domains; wait after reset/enable/config writes. */
    while ((SERCOM1_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_Msk) != 0u) {
    }
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
    /* PA00/PA01 use peripheral function D for SERCOM1 PAD1/PAD0 on this board. */
    PORT_REGS->GROUP[0].PORT_PMUX[HOST_CONSOLE_PMUX_INDEX] =
        (uint8_t)(PORT_PMUX_PMUXE_D | PORT_PMUX_PMUXO_D);

    PORT_REGS->GROUP[0].PORT_PINCFG[HOST_CONSOLE_TX_PIN] = PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PINCFG[HOST_CONSOLE_RX_PIN] =
        (uint8_t)(PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_INEN_Msk);
}

void host_console_init(void)
{
    host_console_rx_head = 0u;
    host_console_rx_tail = 0u;
    host_console_rx_overflows = 0u;

    host_console_configure_pins();

    /* Feed SERCOM1 from GCLK0, which clock_config_init_48mhz() sets to 48 MHz. */
    MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_SERCOM1_Msk;
    GCLK_REGS->GCLK_PCHCTRL[SERCOM1_GCLK_ID_CORE] = GCLK_PCHCTRL_GEN_GCLK0 |
                                                     GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[SERCOM1_GCLK_ID_CORE] & GCLK_PCHCTRL_CHEN_Msk) == 0u) {
    }

    SERCOM1_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_SWRST_Msk;
    host_console_wait_sync();

    /* Internal-clock USART, 8N1, LSB first, TX on PAD0 and RX on PAD1. */
    SERCOM1_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK |
                                            SERCOM_USART_INT_CTRLA_SAMPR_16X_ARITHMETIC |
                                            SERCOM_USART_INT_CTRLA_TXPO_PAD0 |
                                            SERCOM_USART_INT_CTRLA_RXPO_PAD1 |
                                            SERCOM_USART_INT_CTRLA_FORM_USART_FRAME_NO_PARITY |
                                            SERCOM_USART_INT_CTRLA_DORD_LSB;
    SERCOM1_REGS->USART_INT.SERCOM_BAUD = host_console_baud_reg(HOST_CONSOLE_BAUD);
    SERCOM1_REGS->USART_INT.SERCOM_CTRLB = SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT |
                                            SERCOM_USART_INT_CTRLB_TXEN_Msk |
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
    /* Most terminal programs expect CRLF line endings on a CDC UART console. */
    if (ch == '\n') {
        host_console_putc('\r');
    }

    while ((SERCOM1_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) == 0u) {
    }
    SERCOM1_REGS->USART_INT.SERCOM_DATA = (uint16_t)(uint8_t)ch;
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
    host_console_rx_tail = (uint16_t)((tail + 1u) % HOST_CONSOLE_RX_RING_SIZE);
    return true;
}

uint32_t host_console_rx_overflow_count(void)
{
    return host_console_rx_overflows;
}

void SERCOM1_Handler(void)
{
    const uint8_t intflag = SERCOM1_REGS->USART_INT.SERCOM_INTFLAG;

    if ((intflag & SERCOM_USART_INT_INTFLAG_ERROR_Msk) != 0u) {
        /* Clear all USART status bits so later bytes are not blocked by a stale error. */
        SERCOM1_REGS->USART_INT.SERCOM_STATUS = SERCOM_USART_INT_STATUS_Msk;
        SERCOM1_REGS->USART_INT.SERCOM_INTFLAG = SERCOM_USART_INT_INTFLAG_ERROR_Msk;
    }

    if ((intflag & SERCOM_USART_INT_INTFLAG_RXC_Msk) != 0u) {
        /* Reading DATA acknowledges RXC; store the byte unless the ring is full. */
        const uint8_t byte = (uint8_t)SERCOM1_REGS->USART_INT.SERCOM_DATA;
        const uint16_t next_head = (uint16_t)((host_console_rx_head + 1u) %
                                             HOST_CONSOLE_RX_RING_SIZE);

        if (next_head == host_console_rx_tail) {
            host_console_rx_overflows++;
        } else {
            host_console_rx_ring[host_console_rx_head] = byte;
            host_console_rx_head = next_head;
        }
    }
}
