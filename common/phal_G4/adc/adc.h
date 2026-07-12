#ifndef PHAL_G4_ADC_H
#define PHAL_G4_ADC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/phal_G4/dma/dma.h"
#include "common/phal_G4/phal_G4.h"

typedef enum {
    PHAL_ADC_RESOLUTION_12_BIT,
    PHAL_ADC_RESOLUTION_10_BIT,
    PHAL_ADC_RESOLUTION_8_BIT,
    PHAL_ADC_RESOLUTION_6_BIT
} PHAL_ADC_Resolution_t;

typedef enum {
    PHAL_ADC_ALIGNMENT_RIGHT,
    PHAL_ADC_ALIGNMENT_LEFT
} PHAL_ADC_Alignment_t;

typedef enum {
    PHAL_ADC_OVERSAMPLING_NONE = 0,
    PHAL_ADC_OVERSAMPLING_2 = 2,
    PHAL_ADC_OVERSAMPLING_4 = 4,
    PHAL_ADC_OVERSAMPLING_8 = 8,
    PHAL_ADC_OVERSAMPLING_16 = 16,
    PHAL_ADC_OVERSAMPLING_32 = 32,
    PHAL_ADC_OVERSAMPLING_64 = 64,
    PHAL_ADC_OVERSAMPLING_128 = 128,
    PHAL_ADC_OVERSAMPLING_256 = 256
} PHAL_ADC_Oversampling_t;

typedef enum {
    PHAL_ADC_SAMPLE_3_CYCLES,
    PHAL_ADC_SAMPLE_15_CYCLES,
    PHAL_ADC_SAMPLE_28_CYCLES,
    PHAL_ADC_SAMPLE_56_CYCLES,
    PHAL_ADC_SAMPLE_84_CYCLES,
    PHAL_ADC_SAMPLE_112_CYCLES,
    PHAL_ADC_SAMPLE_144_CYCLES,
    PHAL_ADC_SAMPLE_480_CYCLES
} PHAL_ADC_SampleTime_t;

typedef struct {
    uint8_t channel;
    uint8_t rank;
    PHAL_ADC_SampleTime_t sample_time;
} PHAL_ADC_ChannelConfig_t;

typedef struct {
    ADC_TypeDef *instance;
    PHAL_ADC_Resolution_t resolution;
    PHAL_ADC_Alignment_t alignment;
    PHAL_ADC_Oversampling_t oversampling;
    const PHAL_ADC_ChannelConfig_t *channels;
    size_t channel_count;
    bool continuous;
} PHAL_ADC_Config_t;

typedef struct {
    ADC_TypeDef *instance;
    PHAL_DMA_Handle_t dma;
    size_t sequence_length;
    volatile bool busy;
    volatile bool success;
    bool initialized;
} PHAL_ADC_Handle_t;

/**
 * @brief Initialize an ADC instance and its fixed 16-bit DMA route.
 *
 * The ADC common clock is selected from the RCC synchronous PCLK path, the
 * regulator startup delay and calibration are completed before configuration,
 * and the sequence/rank/sample-time fields are derived from the semantic
 * channel list. The DMA controller, channel, request, and IRQ are selected
 * privately from the ADC instance. Continuous mode configures both ADC CONT
 * and DMA circular operation; otherwise DMA is one-shot.
 *
 * @param handle Storage-backed ADC handle that remains valid after initialization.
 * @param config ADC settings and a 1..16-entry channel list.
 * @return true The ADC is enabled and ready.
 * @return false Invalid configuration, timeout, calibration failure, or DMA collision.
 * @note This function blocks for regulator, calibration, disable, and ready waits.
 * @note GPIO analog pins must be configured by the caller before initialization.
 */
bool PHAL_ADC_init(PHAL_ADC_Handle_t *handle, const PHAL_ADC_Config_t *config);

/**
 * @brief Start an asynchronous ADC acquisition into a sample buffer.
 * @param handle Initialized, idle ADC handle.
 * @param samples Destination for 16-bit samples; it remains valid until completion.
 * @param sample_count Number of samples and a multiple of the configured sequence length.
 * @return true DMA was armed before ADSTART was asserted.
 * @return false Invalid state/arguments or DMA/hardware unavailable.
 * @note This function does not block. Continuous acquisitions remain busy until stop.
 */
bool PHAL_ADC_start(PHAL_ADC_Handle_t *handle, uint16_t *samples, size_t sample_count);

/**
 * @brief Start an ADC DMA acquisition and wait for bounded completion.
 * @param handle Initialized ADC handle.
 * @param samples Destination buffer retained while this function runs.
 * @param sample_count Number of samples, multiple of sequence length.
 * @param timeout Maximum loop iterations spent waiting.
 * @return true The one-shot DMA acquisition completed successfully.
 * @return false Invalid state, timeout, or ADC/DMA failure.
 * @note This function blocks and stops a continuous acquisition on timeout.
 */
bool PHAL_ADC_readBlocking(
    PHAL_ADC_Handle_t *handle,
    uint16_t *samples,
    size_t sample_count,
    uint32_t timeout
);

/**
 * @brief Stop ADC conversion and abort its DMA transfer.
 * @param handle Initialized ADC handle.
 * @return true Conversion and DMA stopped before their bounded timeouts.
 * @return false Invalid state or hardware/DMA stop timeout.
 * @note This function blocks for bounded ADSTP and DMA disable waits.
 */
bool PHAL_ADC_stop(PHAL_ADC_Handle_t *handle);

/**
 * @brief Return whether an ADC acquisition is active.
 * @param handle ADC handle.
 * @return true A one-shot or continuous DMA acquisition is active.
 * @return false Invalid, uninitialized, or idle handle.
 * @note This function does not block.
 */
bool PHAL_ADC_busy(const PHAL_ADC_Handle_t *handle);

#define ADC1_CH1_GPIO_Port (GPIOA)
#define ADC1_CH1_Pin       (0)
#define ADC1_CH2_GPIO_Port (GPIOA)
#define ADC1_CH2_Pin       (1)
#define ADC1_CH3_GPIO_Port (GPIOA)
#define ADC1_CH3_Pin       (2)
#define ADC1_CH4_GPIO_Port (GPIOA)
#define ADC1_CH4_Pin       (3)

#endif
