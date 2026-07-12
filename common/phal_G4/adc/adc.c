#include "common/phal_G4/adc/adc_internal.h"

bool PHAL_ADC_init(PHAL_ADC_Handle_t *handle, const PHAL_ADC_Config_t *config) {
    if (handle == NULL || config == NULL) {
        return false;
    }
    if (config->channels == NULL || config->channel_count == 0U
        || config->channel_count > 16U) {
        return false;
    }

    PHAL_ADC_InternalChannelConfig_t channels[16];
    for (size_t i = 0U; i < config->channel_count; ++i) {
        channels[i] = (PHAL_ADC_InternalChannelConfig_t) {
            .channel = config->channels[i].channel,
            .rank = (uint8_t)(i + 1U),
            .sample_time = PHAL_ADC_INTERNAL_SAMPLE_480_CYCLES,
        };
    }
    const PHAL_ADC_InternalConfig_t hardware_config = {
        .instance = config->instance,
        .resolution = PHAL_ADC_INTERNAL_RESOLUTION_12_BIT,
        .alignment = PHAL_ADC_INTERNAL_ALIGNMENT_RIGHT,
        .oversampling = PHAL_ADC_INTERNAL_OVERSAMPLING_NONE,
        .channels = channels,
        .channel_count = config->channel_count,
        .continuous = true,
    };
    if (!PHAL_ADC_internalValidateConfig(&hardware_config)
        || !PHAL_ADC_internalConfigureHardware(&hardware_config)
        || !PHAL_ADC_internalInitializeDma(handle, hardware_config.instance)) {
        return false;
    }
    return PHAL_ADC_internalEnable(handle, &hardware_config);
}

bool PHAL_ADC_start(PHAL_ADC_Handle_t *handle, uint16_t *samples, size_t sample_count) {
    if (!PHAL_ADC_internalValidateStart(handle, samples, sample_count)) {
        return false;
    }
    return PHAL_ADC_internalStartDma(handle, samples, sample_count);
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

__attribute__((weak)) void PHAL_ADC_conversionCompleteCallback(
    PHAL_ADC_Handle_t *handle,
    bool success
) {
    (void)handle;
    (void)success;
}
