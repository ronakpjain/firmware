#ifndef PHAL_G4_ADC_INTERNAL_H
#define PHAL_G4_ADC_INTERNAL_H

#include "common/phal_G4/adc/adc.h"
#include "common/phal_G4/dma/dma_internal.h"

#define PHAL_ADC_INTERNAL_TIMEOUT 100000U

bool PHAL_ADC_internalConfigure(PHAL_ADC_Handle_t *handle, const PHAL_ADC_Config_t *config);
bool PHAL_ADC_internalStart(PHAL_ADC_Handle_t *handle, uint16_t *samples, size_t sample_count);
bool PHAL_ADC_internalStop(PHAL_ADC_Handle_t *handle);

#endif
