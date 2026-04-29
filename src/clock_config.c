#include "clock_config.h"

#include <stdbool.h>

#include "pic32cm5164jh01048.h"
#include "system_pic32cm5164jh01048.h"

static bool cpu_clock_48mhz_configured;

bool SystemCoreClockUpdate_PreHook(void)
{
    /* Keep CMSIS SystemCoreClock in sync after this project replaces the DFP default clock. */
    if (!cpu_clock_48mhz_configured) {
        return false;
    }

    SystemCoreClock = APP_CPU_CLOCK_HZ;
    return true;
}

void clock_config_init_48mhz(void)
{
    /* One flash wait state is required before running the CPU at the 48 MHz target clock. */
    NVMCTRL_REGS->NVMCTRL_CTRLB = (NVMCTRL_REGS->NVMCTRL_CTRLB & ~NVMCTRL_CTRLB_RWS_Msk) |
                                  NVMCTRL_CTRLB_RWS_DUAL;

    /* GCLK0 is the main generator used by the CPU and by simple bring-up peripherals. */
    GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_SRC_OSC48M |
                                 GCLK_GENCTRL_GENEN_Msk;
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL0_Msk) != 0u) {
    }

    /* Keep the CPU undivided so APP_CPU_CLOCK_HZ matches the actual core clock. */
    MCLK_REGS->MCLK_CPUDIV = MCLK_CPUDIV_CPUDIV_DIV1;
    while ((MCLK_REGS->MCLK_INTFLAG & MCLK_INTFLAG_CKRDY_Msk) == 0u) {
    }

    /* Enable and wait for the internal 48 MHz oscillator before relying on it. */
    OSCCTRL_REGS->OSCCTRL_OSC48MCTRL = OSCCTRL_OSC48MCTRL_ENABLE_Msk;
    while ((OSCCTRL_REGS->OSCCTRL_STATUS & OSCCTRL_STATUS_OSC48MRDY_Msk) == 0u) {
    }

    /* DIV1 keeps OSC48M at its full rated frequency. */
    OSCCTRL_REGS->OSCCTRL_OSC48MDIV = OSCCTRL_OSC48MDIV_DIV_DIV1;
    while ((OSCCTRL_REGS->OSCCTRL_OSC48MSYNCBUSY & OSCCTRL_OSC48MSYNCBUSY_OSC48MDIV_Msk) != 0u) {
    }

    cpu_clock_48mhz_configured = true;
    SystemCoreClockUpdate();
    /* Ensure subsequent code observes the completed clock configuration. */
    __DSB();
    __ISB();
}
