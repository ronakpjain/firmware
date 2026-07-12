#ifndef PHAL_G4_ADC_INTERNAL_H
#define PHAL_G4_ADC_INTERNAL_H

#include "common/phal_G4/adc/adc.h"
#include "common/phal_G4/dma/dma_internal.h"

#define PHAL_ADC_INTERNAL_TIMEOUT 100000U

bool PHAL_ADC_internalValidateConfig(const PHAL_ADC_Config_t *config);
bool PHAL_ADC_internalConfigureHardware(const PHAL_ADC_Config_t *config);
bool PHAL_ADC_internalInitializeDma(PHAL_ADC_Handle_t *handle, ADC_TypeDef *adc);
bool PHAL_ADC_internalEnable(PHAL_ADC_Handle_t *handle, const PHAL_ADC_Config_t *config);

bool PHAL_ADC_internalValidateStart(
    const PHAL_ADC_Handle_t *handle,
    const uint16_t *samples,
    size_t sample_count
);
bool PHAL_ADC_internalStartDma(
    PHAL_ADC_Handle_t *handle,
    uint16_t *samples,
    size_t sample_count
);

bool PHAL_ADC_internalValidateHandle(const PHAL_ADC_Handle_t *handle);
bool PHAL_ADC_internalStopConversion(PHAL_ADC_Handle_t *handle);
void PHAL_ADC_internalCompleteStop(PHAL_ADC_Handle_t *handle, bool success);

#endif
