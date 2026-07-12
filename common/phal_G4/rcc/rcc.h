#ifndef PHAL_G4_RCC_H
#define PHAL_G4_RCC_H

#include <stdbool.h>
#include <stdint.h>

#include "common/phal_G4/phal_G4.h"

#define HSE_CLOCK_RATE_HZ 16000000U
#define HSI_CLOCK_RATE_HZ 16000000U

#define RCC_MAX_VCO_RATE_HZ           344000000U
#define RCC_MIN_VCO_RATE_HZ            64000000U
#define RCC_MIN_PLL_INPUT_DIVISOR            1U
#define RCC_MAX_PLL_INPUT_DIVISOR           16U
#define RCC_MIN_PLL_OUTPUT_MULTIPLIER        8U
#define RCC_MAX_PLL_OUTPUT_MULTIPLIER      127U
#define RCC_MAX_SYSCLK_TARGET_HZ     170000000U

typedef enum {
    PLL_SRC_HSI16,
    PLL_SRC_HSE
} PLLSrc_t;

typedef enum {
    CLOCK_SOURCE_HSI,
    CLOCK_SOURCE_HSE,
} ClockSrc_t;

typedef struct {
    /** Direct SYSCLK source when use_pll is false. */
    ClockSrc_t clock_source;
    /** Route SYSCLK from PLL R instead of directly from clock_source. */
    bool use_pll;
    /** Final SYSCLK frequency. Must exactly match the selected source/divider. */
    uint32_t system_clock_target_hz;
    /** HCLK frequency derived exactly from SYSCLK. */
    uint32_t ahb_clock_target_hz;
    /** PCLK1 frequency derived exactly from HCLK. */
    uint32_t apb1_clock_target_hz;
    /** PCLK2 frequency derived exactly from HCLK. */
    uint32_t apb2_clock_target_hz;
    /** PLL oscillator source; used only when use_pll is true. */
    PLLSrc_t pll_src;
    /** PLL VCO frequency; used only when use_pll is true. */
    uint32_t vco_output_rate_target_hz;
    /** Use an externally driven HSE clock rather than a crystal. */
    bool hse_bypass;
} PHAL_RCC_Config_t;

/** Compatibility name retained while G4 board initializers migrate. */
typedef PHAL_RCC_Config_t ClockRateConfig_t;

/**
 * @brief Configure the G4 system, AHB, and APB clocks.
 *
 * The requested oscillator and PLL source are enabled before use. Prescalers
 * are selected only when they produce the requested rates exactly; the
 * resulting clocks are published through the PHAL_RCC_*ClockHz functions.
 * FDCAN is routed to PCLK1 by this configuration.
 *
 * @param config Semantic clock configuration; it is not modified.
 * @return true All requested clock transitions completed.
 * @return false An argument, divider, oscillator, PLL, or status wait failed.
 *
 * @note This function blocks for bounded oscillator, PLL, and clock-switch waits.
 * @note Flash wait-state thresholds assume the STM32G474 is operated in voltage
 *       range 1 (boost mode above 150 MHz) at a board supply allowed by the
 *       corresponding RM0440 flash-latency table.
 * @note No DMA, GPIO, peripheral, or NVIC state is configured here.
 */
bool PHAL_RCC_configure(const PHAL_RCC_Config_t *config);

/**
 * @brief Return the selected SYSCLK frequency in hertz.
 * @return The published system clock frequency, or zero before configuration.
 */
uint32_t PHAL_RCC_systemClockHz(void);

/**
 * @brief Return the selected AHB/HCLK frequency in hertz.
 * @return The published AHB clock frequency, or zero before configuration.
 */
uint32_t PHAL_RCC_ahbClockHz(void);

/**
 * @brief Return the selected APB1 peripheral clock frequency in hertz.
 * @return The published APB1 clock frequency, or zero before configuration.
 */
uint32_t PHAL_RCC_apb1ClockHz(void);

/**
 * @brief Return the selected APB2 peripheral clock frequency in hertz.
 * @return The published APB2 clock frequency, or zero before configuration.
 */
uint32_t PHAL_RCC_apb2ClockHz(void);

/**
 * @brief Return the FDCAN kernel clock selected in RCC.
 * @return The FDCAN kernel clock frequency, or zero for an unsupported source.
 */
uint32_t PHAL_RCC_fdcanClockHz(void);

/**
 * @deprecated Use PHAL_RCC_configure(), which returns true on success.
 * @return 0 on success; 1 on failure. Retained only for source compatibility.
 */
uint8_t PHAL_configureClockRates(ClockRateConfig_t *config);

/* Deprecated data symbols for source-compatible board migration only. */
extern uint32_t APB1ClockRateHz;
extern uint32_t APB2ClockRateHz;
extern uint32_t AHBClockRateHz;
extern uint32_t PLLClockRateHz;

#endif
