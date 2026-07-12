#ifndef PHAL_G4_DMA_H
#define PHAL_G4_DMA_H

#include <stdbool.h>
#include <stdint.h>

#include "common/phal_G4/phal_G4.h"

/**
 * @brief Caller-owned runtime state for one DMA channel.
 *
 * A handle is a concrete object, not an opaque byte buffer. It must remain
 * allocated for the lifetime of any peripheral handle that embeds it.
 * Application code should treat the fields as PHAL-owned state.
 */
typedef struct PHAL_DMA_Handle {
    const struct PHAL_DMA_Route *route; /**< Fixed channel and request routing. */
    DMA_Channel_TypeDef *registers;    /**< Assigned DMA channel registers. */
    void (*callback)(void *context, bool success); /**< Owning peripheral completion hook. */
    void *callback_context;            /**< Context passed to the completion hook. */
    volatile bool busy;                /**< Whether a transfer is currently active. */
    bool circular;                     /**< Whether the active transfer repeats. */
    bool initialized;                  /**< Whether initialization completed successfully. */
} PHAL_DMA_Handle_t;

/**
 * @brief Return whether a DMA transfer is active.
 *
 * @param handle Initialized DMA handle.
 * @return true A one-shot transfer is active, or a circular transfer remains armed.
 * @return false The handle is invalid or no transfer is active.
 *
 * @note This function does not block. The result may change immediately from an
 * interrupt context.
 */
bool PHAL_DMA_busy(const PHAL_DMA_Handle_t *handle);

/**
 * @brief Stop an active DMA transfer and clear its pending flags.
 *
 * @param handle Initialized DMA handle.
 * @return true The channel was stopped or was already idle.
 * @return false The handle is invalid or the channel did not disable before the timeout.
 *
 * @note This function blocks for a bounded hardware-disable wait. It does not
 * invoke the completion callback. The DMA route and channel remain owned by
 * the handle and may be started again.
 */
bool PHAL_DMA_abort(PHAL_DMA_Handle_t *handle);

/**
 * @brief Report completion or failure of a DMA channel transfer.
 * @param handle DMA handle whose transfer generated the interrupt.
 * @param success true for transfer-complete; false for transfer error.
 * @note Weak default callback. Peripheral drivers still receive their private
 *       completion hook before this public notification.
 * @note Executes in DMA interrupt context and repeats for each completed buffer
 *       when circular mode is active.
 */
void PHAL_DMA_transferCompleteCallback(PHAL_DMA_Handle_t *handle, bool success);

#endif
