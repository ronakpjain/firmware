#ifndef PHAL_G4_ADC_H
#define PHAL_G4_ADC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/phal_G4/dma/dma.h"
#include "common/phal_G4/phal_G4.h"

/**
 * @brief One channel in the continuous regular-conversion sequence.
 *
 * Array order determines conversion rank. PHAL uses the 480-cycle sample time
 * required by the current sensor inputs.
 */
typedef struct {
    uint8_t channel; /**< Hardware ADC channel number. */
} PHAL_ADC_ChannelConfig_t;

/**
 * @brief Minimal configuration used to initialize a continuous ADC acquisition.
 *
 * PHAL fixes conversion resolution to 12 bits, right-aligns results, disables
 * oversampling, uses the synchronous HCLK/4 ADC clock, and enables continuous
 * circular-DMA acquisition. Array order determines the conversion sequence,
 * and every channel uses the currently required 480-cycle sample time.
 */
typedef struct {
    ADC_TypeDef *instance;                    /**< ADC1 through ADC4 register block. */
    const PHAL_ADC_ChannelConfig_t *channels; /**< Ordered channel configuration array. */
    size_t channel_count;                     /**< Number of channels, from 1 through 16. */
} PHAL_ADC_Config_t;

/** Runtime state for one initialized ADC and its DMA route. */
typedef struct {
    ADC_TypeDef *instance;       /**< Configured ADC register block. */
    PHAL_DMA_Handle_t dma;       /**< Privately managed receive DMA handle. */
    size_t sequence_length;      /**< Number of configured regular ranks. */
    volatile bool busy;          /**< Whether an acquisition is active. */
    volatile bool success;       /**< Result of the most recently completed acquisition. */
    bool initialized;            /**< Whether initialization completed successfully. */
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
 * @param config ADC instance and a 1..16-entry channel list. PHAL selects
 *               12-bit, right-aligned, non-oversampled, continuous conversion.
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

/**
 * @brief Report completion of an ADC DMA buffer.
 * @param handle ADC handle whose buffer completed.
 * @param success Whether the DMA transfer completed without an error.
 * @note Weak default callback; override it for asynchronous acquisition handling.
 * @note Executes from a DMA interrupt context. In continuous mode it runs after
 *       every completed circular buffer while the handle remains busy.
 */
void PHAL_ADC_conversionCompleteCallback(PHAL_ADC_Handle_t *handle, bool success);

/** @name ADC1 channel 1 through 4 GPIO mappings
 *  Convenience constants for configuring the corresponding pins as analog inputs.
 *  @{ */
#define ADC1_CH1_GPIO_Port (GPIOA)
#define ADC1_CH1_Pin       (0)
#define ADC1_CH2_GPIO_Port (GPIOA)
#define ADC1_CH2_Pin       (1)
#define ADC1_CH3_GPIO_Port (GPIOA)
#define ADC1_CH3_Pin       (2)
#define ADC1_CH4_GPIO_Port (GPIOA)
#define ADC1_CH4_Pin       (3)
/** @} */

#endif
