#ifndef PHAL_G4_DMA_H
#define PHAL_G4_DMA_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Opaque, storage-backed DMA transfer handle.
 *
 * The handle contains all state required by PHAL and must remain allocated for
 * the lifetime of any peripheral handle that embeds it. Register addresses,
 * channel numbers, and request IDs are intentionally private to PHAL.
 */
typedef struct PHAL_DMA_Handle {
    uintptr_t _storage[8]; /**< Private aligned storage; callers must not inspect or modify it. */
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
