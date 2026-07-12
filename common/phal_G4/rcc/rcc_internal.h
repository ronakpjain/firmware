#ifndef PHAL_G4_RCC_INTERNAL_H
#define PHAL_G4_RCC_INTERNAL_H

#include "common/phal_G4/rcc/rcc.h"

bool PHAL_RCC_internalConfigure(const PHAL_RCC_Config_t *config);
uint32_t PHAL_RCC_internalSystemClockHz(void);
uint32_t PHAL_RCC_internalAhbClockHz(void);
uint32_t PHAL_RCC_internalApb1ClockHz(void);
uint32_t PHAL_RCC_internalApb2ClockHz(void);
uint32_t PHAL_RCC_internalFdcanClockHz(void);

#endif
