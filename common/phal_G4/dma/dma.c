#include "common/phal_G4/dma/dma_internal.h"

static PHAL_DMA_Handle_t *validated_handle(const PHAL_DMA_Handle_t *handle) {
    return handle != NULL && handle->initialized
        ? (PHAL_DMA_Handle_t *)(uintptr_t)handle
        : NULL;
}

bool PHAL_DMA_busy(const PHAL_DMA_Handle_t *handle) {
    const PHAL_DMA_Handle_t *state = validated_handle(handle);
    return state != NULL && state->initialized && state->busy;
}

bool PHAL_DMA_abort(PHAL_DMA_Handle_t *handle) {
    PHAL_DMA_Handle_t *state = validated_handle(handle);
    if (state == NULL || !state->initialized) {
        return false;
    }

    const bool disabled = PHAL_DMA_internalDisable(state);
    PHAL_DMA_internalClearTransfer(state);
    return disabled;
}

__attribute__((weak)) void PHAL_DMA_transferCompleteCallback(
    PHAL_DMA_Handle_t *handle,
    bool success
) {
    (void)handle;
    (void)success;
}

void DMA1_Channel1_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA1, 1U); }
void DMA1_Channel2_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA1, 2U); }
void DMA1_Channel3_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA1, 3U); }
void DMA1_Channel4_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA1, 4U); }
void DMA1_Channel5_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA1, 5U); }
void DMA1_Channel6_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA1, 6U); }
void DMA1_Channel7_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA1, 7U); }
void DMA1_Channel8_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA1, 8U); }

void DMA2_Channel1_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA2, 1U); }
void DMA2_Channel2_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA2, 2U); }
void DMA2_Channel3_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA2, 3U); }
void DMA2_Channel4_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA2, 4U); }
void DMA2_Channel5_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA2, 5U); }
void DMA2_Channel6_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA2, 6U); }
void DMA2_Channel7_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA2, 7U); }
void DMA2_Channel8_IRQHandler(void) { PHAL_DMA_internalHandleIRQ(DMA2, 8U); }
