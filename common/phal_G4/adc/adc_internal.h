#ifndef PHAL_G4_ADC_INTERNAL_H
#define PHAL_G4_ADC_INTERNAL_H

#include "common/phal_G4/adc/adc.h"
#include "common/phal_G4/dma/dma_internal.h"

#define PHAL_ADC_INTERNAL_TIMEOUT 100000U

typedef enum {
    PHAL_ADC_INTERNAL_RESOLUTION_12_BIT,
    PHAL_ADC_INTERNAL_RESOLUTION_10_BIT,
    PHAL_ADC_INTERNAL_RESOLUTION_8_BIT,
    PHAL_ADC_INTERNAL_RESOLUTION_6_BIT,
} PHAL_ADC_InternalResolution_t;

typedef enum {
    PHAL_ADC_INTERNAL_ALIGNMENT_RIGHT,
    PHAL_ADC_INTERNAL_ALIGNMENT_LEFT,
} PHAL_ADC_InternalAlignment_t;

typedef enum {
    PHAL_ADC_INTERNAL_OVERSAMPLING_NONE = 0,
    PHAL_ADC_INTERNAL_OVERSAMPLING_2 = 2,
    PHAL_ADC_INTERNAL_OVERSAMPLING_4 = 4,
    PHAL_ADC_INTERNAL_OVERSAMPLING_8 = 8,
    PHAL_ADC_INTERNAL_OVERSAMPLING_16 = 16,
    PHAL_ADC_INTERNAL_OVERSAMPLING_32 = 32,
    PHAL_ADC_INTERNAL_OVERSAMPLING_64 = 64,
    PHAL_ADC_INTERNAL_OVERSAMPLING_128 = 128,
    PHAL_ADC_INTERNAL_OVERSAMPLING_256 = 256,
} PHAL_ADC_InternalOversampling_t;

typedef enum {
    PHAL_ADC_INTERNAL_SAMPLE_3_CYCLES,
    PHAL_ADC_INTERNAL_SAMPLE_15_CYCLES,
    PHAL_ADC_INTERNAL_SAMPLE_28_CYCLES,
    PHAL_ADC_INTERNAL_SAMPLE_56_CYCLES,
    PHAL_ADC_INTERNAL_SAMPLE_84_CYCLES,
    PHAL_ADC_INTERNAL_SAMPLE_112_CYCLES,
    PHAL_ADC_INTERNAL_SAMPLE_144_CYCLES,
    PHAL_ADC_INTERNAL_SAMPLE_480_CYCLES,
} PHAL_ADC_InternalSampleTime_t;

typedef struct {
    uint8_t channel;
    uint8_t rank;
    PHAL_ADC_InternalSampleTime_t sample_time;
} PHAL_ADC_InternalChannelConfig_t;

typedef struct {
    ADC_TypeDef *instance;
    PHAL_ADC_InternalResolution_t resolution;
    PHAL_ADC_InternalAlignment_t alignment;
    PHAL_ADC_InternalOversampling_t oversampling;
    const PHAL_ADC_InternalChannelConfig_t *channels;
    size_t channel_count;
    bool continuous;
} PHAL_ADC_InternalConfig_t;

bool PHAL_ADC_internalValidateConfig(const PHAL_ADC_InternalConfig_t *config);
bool PHAL_ADC_internalConfigureHardware(const PHAL_ADC_InternalConfig_t *config);
bool PHAL_ADC_internalInitializeDma(PHAL_ADC_Handle_t *handle, ADC_TypeDef *adc);
bool PHAL_ADC_internalEnable(
    PHAL_ADC_Handle_t *handle,
    const PHAL_ADC_InternalConfig_t *config
);

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
