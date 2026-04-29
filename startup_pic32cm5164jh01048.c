/*
 * GCC startup file for PIC32CM5164JH01048
 *
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "pic32cm5164jh01048.h"

/* Initialize segments */
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _srelocate;
extern uint32_t _erelocate;
extern uint32_t _szero;
extern uint32_t _ezero;
extern uint32_t _sstack;
extern uint32_t _estack;

/* Optional application-provided functions */
extern void __attribute__((weak)) _on_reset(void);
extern void __attribute__((weak)) _on_bootstrap(void);
extern void __attribute__((weak)) _on_exit(void);

/** \cond DOXYGEN_SHOULD_SKIP_THIS */
int main(void);
/** \endcond */

void __libc_init_array(void);

/* Reset handler */
void Reset_Handler(void);

/* Default empty handler */
void Default_Handler(void);

/* Cortex-M0PLUS core handlers */
void NonMaskableInt_Handler         (void) __attribute__ ((weak, alias("Default_Handler"))); /* Legacy handler name */
void SVCall_Handler                 (void) __attribute__ ((weak, alias("Default_Handler"))); /* Legacy handler name */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/* Trampoline functions for backward compatibility */
void NMI_Handler(void);
void __attribute__ ((weak)) NMI_Handler(void) {
    NonMaskableInt_Handler();
}
void SVC_Handler(void);
void __attribute__ ((weak)) SVC_Handler(void) {
    SVCall_Handler();
}
#pragma GCC diagnostic pop

void HardFault_Handler              (void) __attribute__ ((weak, alias("Default_Handler")));
void PendSV_Handler                 (void) __attribute__ ((weak, alias("Default_Handler")));
void SysTick_Handler                (void) __attribute__ ((weak, alias("Default_Handler")));

/* Peripherals handlers */
void SYSTEM_Handler                 ( void ) __attribute__ ((weak, alias("Default_Handler")));
void WDT_Handler                    ( void ) __attribute__ ((weak, alias("Default_Handler")));
void RTC_Handler                    ( void ) __attribute__ ((weak, alias("Default_Handler")));
void EIC_Handler                    ( void ) __attribute__ ((weak, alias("Default_Handler")));
void FREQM_Handler                  ( void ) __attribute__ ((weak, alias("Default_Handler")));
void MCRAMC_Handler                 ( void ) __attribute__ ((weak, alias("Default_Handler")));
void NVMCTRL_Handler                ( void ) __attribute__ ((weak, alias("Default_Handler")));
void DMAC_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void EVSYS_Handler                  ( void ) __attribute__ ((weak, alias("Default_Handler")));
void SERCOM0_Handler                ( void ) __attribute__ ((weak, alias("Default_Handler")));
void SERCOM1_Handler                ( void ) __attribute__ ((weak, alias("Default_Handler")));
void SERCOM2_Handler                ( void ) __attribute__ ((weak, alias("Default_Handler")));
void SERCOM3_Handler                ( void ) __attribute__ ((weak, alias("Default_Handler")));
void SERCOM4_Handler                ( void ) __attribute__ ((weak, alias("Default_Handler")));
void SERCOM5_Handler                ( void ) __attribute__ ((weak, alias("Default_Handler")));
void CAN0_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void CAN1_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void TCC0_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void TCC1_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void TCC2_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void TC0_5_Handler                  ( void ) __attribute__ ((weak, alias("Default_Handler")));
void TC1_6_Handler                  ( void ) __attribute__ ((weak, alias("Default_Handler")));
void TC2_7_Handler                  ( void ) __attribute__ ((weak, alias("Default_Handler")));
void TC3_Handler                    ( void ) __attribute__ ((weak, alias("Default_Handler")));
void TC4_Handler                    ( void ) __attribute__ ((weak, alias("Default_Handler")));
void ADC0_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void ADC1_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void AC_Handler                     ( void ) __attribute__ ((weak, alias("Default_Handler")));
void DAC_Handler                    ( void ) __attribute__ ((weak, alias("Default_Handler")));
void PDEC_Handler                   ( void ) __attribute__ ((weak, alias("Default_Handler")));
void PTC_Handler                    ( void ) __attribute__ ((weak, alias("Default_Handler")));
void ICM_Handler                    ( void ) __attribute__ ((weak, alias("Default_Handler")));

/* Exception Table */
__attribute__ ((section(".vectors")))
const DeviceVectors exception_table = {

        /* Configure Initial Stack Pointer, using linker-generated symbols */
        .pvStack = (void*) (&_estack),



        .pfnReset_Handler              = (void*) Reset_Handler,
        .pfnNMI_Handler                = (void*) NMI_Handler,
        .pfnHardFault_Handler          = (void*) HardFault_Handler,
        .pvReservedC12                 = (void*) (0UL), /* Reserved */
        .pvReservedC11                 = (void*) (0UL), /* Reserved */
        .pvReservedC10                 = (void*) (0UL), /* Reserved */
        .pvReservedC9                  = (void*) (0UL), /* Reserved */
        .pvReservedC8                  = (void*) (0UL), /* Reserved */
        .pvReservedC7                  = (void*) (0UL), /* Reserved */
        .pvReservedC6                  = (void*) (0UL), /* Reserved */
        .pfnSVC_Handler                = (void*) SVC_Handler,
        .pvReservedC4                  = (void*) (0UL), /* Reserved */
        .pvReservedC3                  = (void*) (0UL), /* Reserved */
        .pfnPendSV_Handler             = (void*) PendSV_Handler,
        .pfnSysTick_Handler            = (void*) SysTick_Handler,

        /* Configurable interrupts */
        .pfnSYSTEM_Handler             = (void*) SYSTEM_Handler, /* 0  Main Clock */
        .pfnWDT_Handler                = (void*) WDT_Handler,    /* 1  Watchdog Timer */
        .pfnRTC_Handler                = (void*) RTC_Handler,    /* 2  Real-Time Counter */
        .pfnEIC_Handler                = (void*) EIC_Handler,    /* 3  External Interrupt Controller */
        .pfnFREQM_Handler              = (void*) FREQM_Handler,  /* 4  Frequency Meter */
        .pfnMCRAMC_Handler             = (void*) MCRAMC_Handler, /* 5  Multi-Channel RAM Controller */
        .pfnNVMCTRL_Handler            = (void*) NVMCTRL_Handler, /* 6  Non-Volatile Memory Controller */
        .pfnDMAC_Handler               = (void*) DMAC_Handler,   /* 7  Direct Memory Access Controller */
        .pfnEVSYS_Handler              = (void*) EVSYS_Handler,  /* 8  Event System Interface */
        .pfnSERCOM0_Handler            = (void*) SERCOM0_Handler, /* 9  Serial Communication Interface */
        .pfnSERCOM1_Handler            = (void*) SERCOM1_Handler, /* 10 Serial Communication Interface */
        .pfnSERCOM2_Handler            = (void*) SERCOM2_Handler, /* 11 Serial Communication Interface */
        .pfnSERCOM3_Handler            = (void*) SERCOM3_Handler, /* 12 Serial Communication Interface */
        .pfnSERCOM4_Handler            = (void*) SERCOM4_Handler, /* 13 Serial Communication Interface */
        .pfnSERCOM5_Handler            = (void*) SERCOM5_Handler, /* 14 Serial Communication Interface */
        .pfnCAN0_Handler               = (void*) CAN0_Handler,   /* 15 Control Area Network */
        .pfnCAN1_Handler               = (void*) CAN1_Handler,   /* 16 Control Area Network */
        .pfnTCC0_Handler               = (void*) TCC0_Handler,   /* 17 Timer Counter for Control Applications */
        .pfnTCC1_Handler               = (void*) TCC1_Handler,   /* 18 Timer Counter for Control Applications */
        .pfnTCC2_Handler               = (void*) TCC2_Handler,   /* 19 Timer Counter for Control Applications */
        .pfnTC0_5_Handler              = (void*) TC0_5_Handler,  /* 20 Basic Timer Counter */
        .pfnTC1_6_Handler              = (void*) TC1_6_Handler,  /* 21 Basic Timer Counter */
        .pfnTC2_7_Handler              = (void*) TC2_7_Handler,  /* 22 Basic Timer Counter */
        .pfnTC3_Handler                = (void*) TC3_Handler,    /* 23 Basic Timer Counter */
        .pfnTC4_Handler                = (void*) TC4_Handler,    /* 24 Basic Timer Counter */
        .pfnADC0_Handler               = (void*) ADC0_Handler,   /* 25 Analog Digital Converter */
        .pfnADC1_Handler               = (void*) ADC1_Handler,   /* 26 Analog Digital Converter */
        .pfnAC_Handler                 = (void*) AC_Handler,     /* 27 Analog Comparators */
        .pfnDAC_Handler                = (void*) DAC_Handler,    /* 28 Digital Analog Converter */
        .pfnPDEC_Handler               = (void*) PDEC_Handler,   /* 29 Quadrature Decodeur */
        .pfnPTC_Handler                = (void*) PTC_Handler,    /* 30 Peripheral Touch Controller */
        .pfnICM_Handler                = (void*) ICM_Handler     /* 31 Integrity Check Monitor */
};

/**
 * \brief This is the code that gets called on processor reset.
 * To initialize the device, and call the main() routine.
 */
void Reset_Handler(void)
{
        const uint32_t *pSrc;
        uint32_t *pDest;

        /* Initialize the relocate segment */
        pSrc = &_etext;
        pDest = &_srelocate;

        if (pSrc != pDest) {
                while ((uintptr_t)pDest < (uintptr_t)&_erelocate) {
                        *pDest++ = *pSrc++;
                }
        }


        /* Clear the zero segment */
        for (pDest = &_szero; (uintptr_t)pDest < (uintptr_t)&_ezero; ++pDest) {
                *pDest = 0U;
        }


        /* Set the vector table base address */
        pSrc = (uint32_t *) & _sfixed;
        SCB->VTOR = ((uint32_t) pSrc & SCB_VTOR_TBLOFF_Msk);

        /* Call the optional application-provided _on_reset() function. */
        if ((bool)_on_reset) {
                _on_reset();
        }

        /* Initialize the C library */
        __libc_init_array();

        /* Call the optional application-provided _on_bootstrap() function. */
        if ((bool)_on_bootstrap) {
                _on_bootstrap();
        }

        /* Branch to main function */
        (void)main();

        /* Call the optional application-provided _on_exit() function. */
        if ((bool)_on_exit) {
                _on_exit();
        } else {
                /* Infinite loop */
                while (true) {}
        }
}

/**
 * \brief Default interrupt handler for unused IRQs.
 */
void Default_Handler(void)
{
        while (true) {}
}
