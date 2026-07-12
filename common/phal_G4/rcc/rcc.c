#include "common/phal_G4/rcc/rcc_internal.h"

typedef struct {
    uint32_t system_clock_hz;
    uint32_t ahb_clock_hz;
    uint32_t apb1_clock_hz;
    uint32_t apb2_clock_hz;
} PHAL_RCC_ClockRates_t;

static PHAL_RCC_ClockRates_t clock_rates;

bool PHAL_RCC_configure(const PHAL_RCC_Config_t *config) {
    if (!PHAL_RCC_internalValidateConfig(config)
        || !PHAL_RCC_internalConfigureSystemClock(config)
        || !PHAL_RCC_internalConfigureAhbClock(config->ahb_clock_target_hz)
        || !PHAL_RCC_internalConfigureApb1Clock(config->apb1_clock_target_hz)
        || !PHAL_RCC_internalConfigureApb2Clock(config->apb2_clock_target_hz)) {
        return false;
    }

    PHAL_RCC_internalSelectFdcanClock();
    clock_rates = (PHAL_RCC_ClockRates_t) {
        .system_clock_hz = config->system_clock_target_hz,
        .ahb_clock_hz = config->ahb_clock_target_hz,
        .apb1_clock_hz = config->apb1_clock_target_hz,
        .apb2_clock_hz = config->apb2_clock_target_hz,
    };
    return true;
}

uint32_t PHAL_RCC_systemClockHz(void) {
    return clock_rates.system_clock_hz;
}

uint32_t PHAL_RCC_ahbClockHz(void) {
    return clock_rates.ahb_clock_hz;
}

uint32_t PHAL_RCC_apb1ClockHz(void) {
    return clock_rates.apb1_clock_hz;
}

uint32_t PHAL_RCC_apb2ClockHz(void) {
    return clock_rates.apb2_clock_hz;
}

uint32_t PHAL_RCC_fdcanClockHz(void) {
    return clock_rates.apb1_clock_hz;
}
