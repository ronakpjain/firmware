#ifndef PHAL_G4_RCC_INTERNAL_H
#define PHAL_G4_RCC_INTERNAL_H

#include "common/phal_G4/rcc/rcc.h"

bool PHAL_RCC_internalValidateConfig(const PHAL_RCC_Config_t *config);
bool PHAL_RCC_internalConfigureSystemClock(const PHAL_RCC_Config_t *config);
bool PHAL_RCC_internalConfigureAhbClock(uint32_t ahb_clock_target_hz);
bool PHAL_RCC_internalConfigureApb1Clock(uint32_t apb1_clock_target_hz);
bool PHAL_RCC_internalConfigureApb2Clock(uint32_t apb2_clock_target_hz);
void PHAL_RCC_internalSelectFdcanClock(void);


#endif
