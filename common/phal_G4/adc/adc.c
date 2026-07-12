#include "common/phal_G4/adc/adc_internal.h"

bool PHAL_ADC_init(PHAL_ADC_Handle_t *handle, const PHAL_ADC_Config_t *config) {
    if (handle == NULL || !PHAL_ADC_internalValidateConfig(config)
        || !PHAL_ADC_internalConfigureHardware(config)
        || !PHAL_ADC_internalInitializeDma(handle, config->instance)) {
        return false;
    }
    return PHAL_ADC_internalEnable(handle, config);
}

bool PHAL_ADC_start(PHAL_ADC_Handle_t *handle, uint16_t *samples, size_t sample_count) {
    if (!PHAL_ADC_internalValidateStart(handle, samples, sample_count)) {
        return false;
    }
    return PHAL_ADC_internalStartDma(handle, samples, sample_count);
}

bool PHAL_ADC_readBlocking(
    PHAL_ADC_Handle_t *handle,
    uint16_t *samples,
    size_t sample_count,
    uint32_t timeout
) {
    if (handle == NULL || !handle->initialized
        || (handle->instance->CFGR & ADC_CFGR_CONT) != 0U
        || !PHAL_ADC_start(handle, samples, sample_count)) {
        return false;
    }

    while (handle->busy && timeout != 0U) {
        --timeout;
    }
    if (handle->busy) {
        (void)PHAL_ADC_stop(handle);
        return false;
    }
    return handle->success;
}

bool PHAL_ADC_stop(PHAL_ADC_Handle_t *handle) {
    if (!PHAL_ADC_internalValidateHandle(handle)) {
        return false;
    }

    bool success = PHAL_ADC_internalStopConversion(handle);
    if (!PHAL_DMA_abort(&handle->dma)) {
        success = false;
    }
    PHAL_ADC_internalCompleteStop(handle, success);
    return success;
}

bool PHAL_ADC_busy(const PHAL_ADC_Handle_t *handle) {
    return handle != NULL && handle->initialized && handle->busy;
}
