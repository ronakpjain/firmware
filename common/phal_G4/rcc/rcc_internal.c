#include "common/phal_G4/rcc/rcc_internal.h"

#include <stddef.h>

extern uint32_t SystemCoreClock;
extern void SystemCoreClockUpdate(void);

uint32_t APB1ClockRateHz;
uint32_t APB2ClockRateHz;
uint32_t AHBClockRateHz;
uint32_t PLLClockRateHz;

static uint32_t system_clock_hz;
static uint32_t ahb_clock_hz;
static uint32_t apb1_clock_hz;
static uint32_t apb2_clock_hz;
static uint32_t pll_vco_hz;

static bool wait_for_flag(volatile uint32_t *registers, uint32_t mask, bool set) {
    for (uint32_t timeout = 100000U; timeout != 0U; --timeout) {
        const bool active = ((*registers & mask) != 0U);
        if (active == set) {
            return true;
        }
    }
    return false;
}

static bool wait_for_field(volatile uint32_t *registers, uint32_t mask, uint32_t value) {
    for (uint32_t timeout = 100000U; timeout != 0U; --timeout) {
        if ((*registers & mask) == value) {
            return true;
        }
    }
    return false;
}

static void publish_rates(void) {
    APB1ClockRateHz = apb1_clock_hz;
    APB2ClockRateHz = apb2_clock_hz;
    AHBClockRateHz = ahb_clock_hz;
    PLLClockRateHz = pll_vco_hz;
}

static uint32_t source_rate(ClockSrc_t source) {
    return source == CLOCK_SOURCE_HSE ? HSE_CLOCK_RATE_HZ : HSI_CLOCK_RATE_HZ;
}

static bool enable_hsi(void) {
    RCC->CR |= RCC_CR_HSION;
    return wait_for_flag(&RCC->CR, RCC_CR_HSIRDY, true);
}

static bool enable_hse(bool bypass) {
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_HSEBYP);
    if (!wait_for_flag(&RCC->CR, RCC_CR_HSERDY, false)) {
        return false;
    }

    if (bypass) {
        RCC->CR |= RCC_CR_HSEBYP;
    }
    RCC->CR |= RCC_CR_HSEON;
    return wait_for_flag(&RCC->CR, RCC_CR_HSERDY, true);
}

static bool select_system_source(uint32_t source, uint32_t status) {
    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~RCC_CFGR_SW_Msk;
    cfgr |= source & RCC_CFGR_SW_Msk;
    RCC->CFGR = cfgr;
    return wait_for_field(&RCC->CFGR, RCC_CFGR_SWS_Msk, status);
}

static bool set_flash_latency(uint32_t frequency_hz) {
    if (frequency_hz > RCC_MAX_SYSCLK_TARGET_HZ) {
        return false;
    }

    uint32_t latency = FLASH_ACR_LATENCY_0WS;
    if (frequency_hz > 136000000U) {
        latency = FLASH_ACR_LATENCY_4WS;
    } else if (frequency_hz > 102000000U) {
        latency = FLASH_ACR_LATENCY_3WS;
    } else if (frequency_hz > 68000000U) {
        latency = FLASH_ACR_LATENCY_2WS;
    } else if (frequency_hz > 34000000U) {
        latency = FLASH_ACR_LATENCY_1WS;
    }

    uint32_t acr = FLASH->ACR;
    acr &= ~FLASH_ACR_LATENCY_Msk;
    acr |= latency;
    FLASH->ACR = acr;
    __DSB();
    return true;
}

static bool configure_pll_vco_internal(PLLSrc_t pll_source, uint32_t target_hz, bool hse_bypass) {
    if (target_hz < RCC_MIN_VCO_RATE_HZ || target_hz > RCC_MAX_VCO_RATE_HZ) {
        return false;
    }

    uint32_t input_hz;
    uint32_t source_bits;
    if (pll_source == PLL_SRC_HSI16) {
        if (!enable_hsi()) {
            return false;
        }
        input_hz = HSI_CLOCK_RATE_HZ;
        source_bits = RCC_PLLCFGR_PLLSRC_HSI;
    } else if (pll_source == PLL_SRC_HSE) {
        if (!enable_hse(hse_bypass)) {
            return false;
        }
        input_hz = HSE_CLOCK_RATE_HZ;
        source_bits = RCC_PLLCFGR_PLLSRC_HSE;
    } else {
        return false;
    }

    if ((RCC->CFGR & RCC_CFGR_SWS_Msk) == RCC_CFGR_SWS_PLL) {
        if (!enable_hsi() || !select_system_source(RCC_CFGR_SW_HSI, RCC_CFGR_SWS_HSI)) {
            return false;
        }
    }

    RCC->CR &= ~RCC_CR_PLLON;
    if (!wait_for_flag(&RCC->CR, RCC_CR_PLLRDY, false)) {
        return false;
    }

    uint32_t m = 0U;
    uint32_t n = 0U;
    for (uint32_t candidate_m = RCC_MIN_PLL_INPUT_DIVISOR;
         candidate_m <= RCC_MAX_PLL_INPUT_DIVISOR && n == 0U;
         ++candidate_m) {
        const uint32_t vco_input_hz = input_hz / candidate_m;
        if (vco_input_hz < 2660000U || vco_input_hz > 16000000U) {
            continue;
        }
        for (uint32_t candidate_n = RCC_MIN_PLL_OUTPUT_MULTIPLIER;
             candidate_n <= RCC_MAX_PLL_OUTPUT_MULTIPLIER;
             ++candidate_n) {
            const uint64_t calculated = ((uint64_t)input_hz * candidate_n) / candidate_m;
            if (calculated == target_hz) {
                m = candidate_m;
                n = candidate_n;
                break;
            }
        }
    }

    if (m == 0U || n == 0U) {
        return false;
    }

    uint32_t pllcfgr = RCC->PLLCFGR;
    pllcfgr &= ~(RCC_PLLCFGR_PLLSRC_Msk | RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk);
    pllcfgr |= source_bits;
    pllcfgr |= ((m - 1U) << RCC_PLLCFGR_PLLM_Pos) & RCC_PLLCFGR_PLLM_Msk;
    pllcfgr |= (n << RCC_PLLCFGR_PLLN_Pos) & RCC_PLLCFGR_PLLN_Msk;
    RCC->PLLCFGR = pllcfgr;

    pll_vco_hz = target_hz;
    publish_rates();
    return true;
}

static bool configure_pll_system_clock(uint32_t system_clock_target_hz) {
    if (pll_vco_hz == 0U || system_clock_target_hz == 0U
        || system_clock_target_hz > RCC_MAX_SYSCLK_TARGET_HZ) {
        return false;
    }

    uint32_t r_encoding;
    switch (pll_vco_hz / system_clock_target_hz) {
        case 2U: r_encoding = 0U; break;
        case 4U: r_encoding = 1U; break;
        case 6U: r_encoding = 2U; break;
        case 8U: r_encoding = 3U; break;
        default: return false;
    }
    const uint32_t divisor = 2U + (r_encoding * 2U);
    if (pll_vco_hz / divisor != system_clock_target_hz) {
        return false;
    }

    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    const uint32_t previous_hpre = RCC->CFGR & RCC_CFGR_HPRE_Msk;
    const bool boost_mode = system_clock_target_hz > 150000000U;
    if (boost_mode) {
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | RCC_CFGR_HPRE_DIV2;
        PWR->CR5 &= ~PWR_CR5_R1MODE;
        if (!wait_for_flag(&PWR->SR2, PWR_SR2_VOSF, false)) {
            return false;
        }
    }
    if (!set_flash_latency(system_clock_target_hz)) {
        return false;
    }

    uint32_t pllcfgr = RCC->PLLCFGR;
    pllcfgr &= ~(RCC_PLLCFGR_PLLREN | RCC_PLLCFGR_PLLR_Msk
                 | RCC_PLLCFGR_PLLQEN | RCC_PLLCFGR_PLLQ_Msk
                 | RCC_PLLCFGR_PLLPEN | RCC_PLLCFGR_PLLP_Msk
                 | RCC_PLLCFGR_PLLPDIV_Msk);
    pllcfgr |= RCC_PLLCFGR_PLLREN;
    pllcfgr |= (r_encoding << RCC_PLLCFGR_PLLR_Pos) & RCC_PLLCFGR_PLLR_Msk;
    RCC->PLLCFGR = pllcfgr;

    RCC->CR |= RCC_CR_PLLON;
    if (!wait_for_flag(&RCC->CR, RCC_CR_PLLRDY, true)) {
        return false;
    }
    if (!select_system_source(RCC_CFGR_SW_PLL, RCC_CFGR_SWS_PLL)) {
        return false;
    }
    if (boost_mode) {
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE_Msk) | previous_hpre;
    }

    system_clock_hz = system_clock_target_hz;
    SystemCoreClockUpdate();
    publish_rates();
    return true;
}

static bool configure_hsi_system_clock(void) {
    if (!enable_hsi() || !select_system_source(RCC_CFGR_SW_HSI, RCC_CFGR_SWS_HSI)) {
        return false;
    }
    if (!set_flash_latency(HSI_CLOCK_RATE_HZ)) {
        return false;
    }

    system_clock_hz = HSI_CLOCK_RATE_HZ;
    SystemCoreClockUpdate();
    return true;
}

static bool configure_hse_system_clock(void) {
    if (!enable_hse((RCC->CR & RCC_CR_HSEBYP) != 0U)
        || !select_system_source(RCC_CFGR_SW_HSE, RCC_CFGR_SWS_HSE)) {
        return false;
    }
    if (!set_flash_latency(HSE_CLOCK_RATE_HZ)) {
        return false;
    }

    system_clock_hz = HSE_CLOCK_RATE_HZ;
    SystemCoreClockUpdate();
    return true;
}

static bool ahb_divider(uint32_t ratio, uint32_t *encoded) {
    switch (ratio) {
        case 1U: *encoded = RCC_CFGR_HPRE_DIV1; return true;
        case 2U: *encoded = RCC_CFGR_HPRE_DIV2; return true;
        case 4U: *encoded = RCC_CFGR_HPRE_DIV4; return true;
        case 8U: *encoded = RCC_CFGR_HPRE_DIV8; return true;
        case 16U: *encoded = RCC_CFGR_HPRE_DIV16; return true;
        case 64U: *encoded = RCC_CFGR_HPRE_DIV64; return true;
        case 128U: *encoded = RCC_CFGR_HPRE_DIV128; return true;
        case 256U: *encoded = RCC_CFGR_HPRE_DIV256; return true;
        case 512U: *encoded = RCC_CFGR_HPRE_DIV512; return true;
        default: return false;
    }
}

static bool apb1_divider(uint32_t ratio, uint32_t *encoded) {
    switch (ratio) {
        case 1U: *encoded = RCC_CFGR_PPRE1_DIV1; return true;
        case 2U: *encoded = RCC_CFGR_PPRE1_DIV2; return true;
        case 4U: *encoded = RCC_CFGR_PPRE1_DIV4; return true;
        case 8U: *encoded = RCC_CFGR_PPRE1_DIV8; return true;
        case 16U: *encoded = RCC_CFGR_PPRE1_DIV16; return true;
        default: return false;
    }
}

static bool apb2_divider(uint32_t ratio, uint32_t *encoded) {
    switch (ratio) {
        case 1U: *encoded = RCC_CFGR_PPRE2_DIV1; return true;
        case 2U: *encoded = RCC_CFGR_PPRE2_DIV2; return true;
        case 4U: *encoded = RCC_CFGR_PPRE2_DIV4; return true;
        case 8U: *encoded = RCC_CFGR_PPRE2_DIV8; return true;
        case 16U: *encoded = RCC_CFGR_PPRE2_DIV16; return true;
        default: return false;
    }
}

static bool configure_ahb_clock(uint32_t ahb_clock_target_hz) {
    if (system_clock_hz == 0U || ahb_clock_target_hz == 0U
        || system_clock_hz % ahb_clock_target_hz != 0U) {
        return false;
    }

    uint32_t encoded;
    const uint32_t ratio = system_clock_hz / ahb_clock_target_hz;
    if (!ahb_divider(ratio, &encoded)) {
        return false;
    }

    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~RCC_CFGR_HPRE_Msk;
    cfgr |= encoded;
    RCC->CFGR = cfgr;
    ahb_clock_hz = system_clock_hz / ratio;
    SystemCoreClockUpdate();
    publish_rates();
    return true;
}

static bool configure_apb1_clock(uint32_t apb1_clock_target_hz) {
    if (ahb_clock_hz == 0U || apb1_clock_target_hz == 0U
        || ahb_clock_hz % apb1_clock_target_hz != 0U) {
        return false;
    }

    uint32_t encoded;
    const uint32_t ratio = ahb_clock_hz / apb1_clock_target_hz;
    if (!apb1_divider(ratio, &encoded)) {
        return false;
    }

    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~RCC_CFGR_PPRE1_Msk;
    cfgr |= encoded;
    RCC->CFGR = cfgr;
    apb1_clock_hz = ahb_clock_hz / ratio;
    publish_rates();
    return true;
}

static bool configure_apb2_clock(uint32_t apb2_clock_target_hz) {
    if (ahb_clock_hz == 0U || apb2_clock_target_hz == 0U
        || ahb_clock_hz % apb2_clock_target_hz != 0U) {
        return false;
    }

    uint32_t encoded;
    const uint32_t ratio = ahb_clock_hz / apb2_clock_target_hz;
    if (!apb2_divider(ratio, &encoded)) {
        return false;
    }

    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~RCC_CFGR_PPRE2_Msk;
    cfgr |= encoded;
    RCC->CFGR = cfgr;
    apb2_clock_hz = ahb_clock_hz / ratio;
    publish_rates();
    return true;
}

bool PHAL_RCC_internalConfigure(const PHAL_RCC_Config_t *config) {
    if (config == NULL || config->clock_source > CLOCK_SOURCE_HSE
        || config->system_clock_target_hz == 0U || config->ahb_clock_target_hz == 0U
        || config->apb1_clock_target_hz == 0U || config->apb2_clock_target_hz == 0U
        || config->system_clock_target_hz % config->ahb_clock_target_hz != 0U
        || config->ahb_clock_target_hz % config->apb1_clock_target_hz != 0U
        || config->ahb_clock_target_hz % config->apb2_clock_target_hz != 0U
        || (!config->use_pll
            && config->system_clock_target_hz != source_rate(config->clock_source))) {
        return false;
    }

    uint32_t ignored;
    const uint32_t pll_ratio = config->use_pll && config->system_clock_target_hz != 0U
        ? config->vco_output_rate_target_hz / config->system_clock_target_hz : 0U;
    if (!ahb_divider(config->system_clock_target_hz / config->ahb_clock_target_hz, &ignored)
        || !apb1_divider(config->ahb_clock_target_hz / config->apb1_clock_target_hz, &ignored)
        || !apb2_divider(config->ahb_clock_target_hz / config->apb2_clock_target_hz, &ignored)
        || (config->use_pll && (config->pll_src > PLL_SRC_HSE
            || config->vco_output_rate_target_hz % config->system_clock_target_hz != 0U
            || (pll_ratio != 2U && pll_ratio != 4U && pll_ratio != 6U && pll_ratio != 8U)))) {
        return false;
    }

    if (config->use_pll) {
        if (!configure_pll_vco_internal(
                config->pll_src,
                config->vco_output_rate_target_hz,
                config->hse_bypass)
            || !configure_pll_system_clock(config->system_clock_target_hz)) {
            return false;
        }
    } else {
        if (config->clock_source == CLOCK_SOURCE_HSI) {
            if (!configure_hsi_system_clock()) {
                return false;
            }
        } else {
            RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_HSEBYP);
            if (config->hse_bypass) {
                RCC->CR |= RCC_CR_HSEBYP;
            }
            if (!configure_hse_system_clock()) {
                return false;
            }
        }
    }

    if (!configure_ahb_clock(config->ahb_clock_target_hz)
        || !configure_apb1_clock(config->apb1_clock_target_hz)
        || !configure_apb2_clock(config->apb2_clock_target_hz)) {
        return false;
    }

    uint32_t ccipr = RCC->CCIPR;
    ccipr &= ~RCC_CCIPR_FDCANSEL_Msk;
    ccipr |= RCC_CCIPR_FDCANSEL_1; /* PCLK1 */
    RCC->CCIPR = ccipr;
    publish_rates();
    return true;
}

uint32_t PHAL_RCC_internalSystemClockHz(void) { return system_clock_hz; }
uint32_t PHAL_RCC_internalAhbClockHz(void) { return ahb_clock_hz; }
uint32_t PHAL_RCC_internalApb1ClockHz(void) { return apb1_clock_hz; }
uint32_t PHAL_RCC_internalApb2ClockHz(void) { return apb2_clock_hz; }

uint32_t PHAL_RCC_internalFdcanClockHz(void) {
    const uint32_t source = (RCC->CCIPR & RCC_CCIPR_FDCANSEL_Msk) >> RCC_CCIPR_FDCANSEL_Pos;
    switch (source) {
        case 0U: return HSE_CLOCK_RATE_HZ;
        case 2U: return apb1_clock_hz;
        default: return 0U;
    }
}
