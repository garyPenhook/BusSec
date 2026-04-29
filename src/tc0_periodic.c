#include "tc0_periodic.h"

#include <stdint.h>

#include "clock_config.h"
#include "pic32cm5164jh01048.h"

/* TC0 runs from 48 MHz GCLK0; DIV1024 gives a 46.875 kHz timer tick. */
#define TC0_GCLK_HZ       APP_CPU_CLOCK_HZ
#define TC0_PRESCALER_DIV (UINT32_C(1024))
#define TC0_500MS_COMPARE ((uint16_t)((TC0_GCLK_HZ / TC0_PRESCALER_DIV / 2u) - 1u))

static tc0_periodic_callback_t tc0_periodic_callback;

static void tc0_wait_sync(void)
{
    /* TC0 registers are synchronized into the timer clock domain. */
    while (TC0_REGS->COUNT16.TC_SYNCBUSY != 0u) {
    }
}

void tc0_periodic_init_500ms(tc0_periodic_callback_t callback)
{
    tc0_periodic_callback = callback;

    MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_TC0_Msk;

    /* Connect TC0 to the same 48 MHz generator used for the CPU clock. */
    GCLK_REGS->GCLK_PCHCTRL[TC0_GCLK_ID] = GCLK_PCHCTRL_GEN_GCLK0 |
                                           GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[TC0_GCLK_ID] & GCLK_PCHCTRL_CHEN_Msk) == 0u) {
    }

    TC0_REGS->COUNT16.TC_CTRLA = TC_CTRLA_SWRST_Msk;
    tc0_wait_sync();

    /* Match-frequency mode resets COUNT on CC0, creating a fixed 500 ms period. */
    TC0_REGS->COUNT16.TC_CTRLA = TC_CTRLA_MODE_COUNT16 |
                                 TC_CTRLA_PRESCALER_DIV1024 |
                                 TC_CTRLA_PRESCSYNC_PRESC;
    TC0_REGS->COUNT16.TC_WAVE = TC_WAVE_WAVEGEN_MFRQ;
    TC0_REGS->COUNT16.TC_COUNT = 0u;
    TC0_REGS->COUNT16.TC_CC[0] = TC0_500MS_COMPARE;
    tc0_wait_sync();

    TC0_REGS->COUNT16.TC_INTFLAG = TC_INTFLAG_Msk;
    TC0_REGS->COUNT16.TC_INTENSET = TC_INTENSET_MC0_Msk;

    NVIC_ClearPendingIRQ(TC0_IRQn);
    NVIC_EnableIRQ(TC0_IRQn);

    TC0_REGS->COUNT16.TC_CTRLA |= TC_CTRLA_ENABLE_Msk;
    tc0_wait_sync();
}

void TC0_5_Handler(void)
{
    if ((TC0_REGS->COUNT16.TC_INTFLAG & TC_INTFLAG_MC0_Msk) != 0u) {
        /* Clear MC0 before calling back so a long callback cannot hide a pending match. */
        TC0_REGS->COUNT16.TC_INTFLAG = TC_INTFLAG_MC0_Msk;
        if (tc0_periodic_callback != 0) {
            tc0_periodic_callback();
        }
    }
}
