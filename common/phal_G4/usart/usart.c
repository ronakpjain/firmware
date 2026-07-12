#include "common/phal_G4/usart/usart_internal.h"

#include <stddef.h>

bool PHAL_USART_init(PHAL_USART_Handle_t *handle, const PHAL_USART_Config_t *config) {
    uint32_t baud_register;
    if (handle == NULL || !PHAL_USART_internalValidateConfig(config)
        || !PHAL_USART_internalPrepareInstance(config, &baud_register)
        || !PHAL_USART_internalInitializeStateAndDma(handle, config)) {
        return false;
    }

    PHAL_USART_internalConfigureRegisters(handle, config, baud_register);
    return true;
}

bool PHAL_USART_transmit(PHAL_USART_Handle_t *handle, const uint8_t *data, size_t length) {
    if (!PHAL_USART_internalValidateTransmit(handle, data, length)) {
        return false;
    }
    return PHAL_USART_internalArmTransmit(handle, data, length);
}

bool PHAL_USART_transmitBlocking(
    PHAL_USART_Handle_t *handle,
    const uint8_t *data,
    size_t length,
    uint32_t timeout
) {
    if (!PHAL_USART_transmit(handle, data, length)) {
        return false;
    }
    while (handle->tx_busy && timeout != 0U) {
        --timeout;
    }
    if (handle->tx_busy) {
        (void)PHAL_DMA_abort(&handle->tx_dma);
        handle->instance->CR3 &= ~USART_CR3_DMAT;
        handle->instance->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
        handle->tx_busy = false;
        handle->tx_success = false;
        return false;
    }
    return handle->tx_success;
}

bool PHAL_USART_receive(PHAL_USART_Handle_t *handle, uint8_t *data, size_t length) {
    if (!PHAL_USART_internalValidateReceive(handle, data, length)) {
        return false;
    }
    return PHAL_USART_internalArmReceive(handle, data, length, false);
}

bool PHAL_USART_receiveBlocking(
    PHAL_USART_Handle_t *handle,
    uint8_t *data,
    size_t length,
    uint32_t timeout
) {
    if (!PHAL_USART_receive(handle, data, length)) {
        return false;
    }
    while (handle->rx_busy && timeout != 0U) {
        --timeout;
    }
    if (handle->rx_busy) {
        (void)PHAL_USART_stopReceive(handle);
        return false;
    }
    return handle->rx_success;
}

bool PHAL_USART_startIdleReceive(PHAL_USART_Handle_t *handle, uint8_t *data, size_t capacity) {
    if (!PHAL_USART_internalValidateReceive(handle, data, capacity)) {
        return false;
    }
    return PHAL_USART_internalArmReceive(handle, data, capacity, true);
}

bool PHAL_USART_stopReceive(PHAL_USART_Handle_t *handle) {
    if (!PHAL_USART_internalValidateHandle(handle)) {
        return false;
    }

    PHAL_USART_internalDisableReceiveRequests(handle);
    const bool stopped = PHAL_DMA_abort(&handle->rx_dma);
    PHAL_USART_internalCompleteStopReceive(handle);
    return stopped;
}

bool PHAL_USART_txBusy(const PHAL_USART_Handle_t *handle) {
    return handle != NULL && handle->initialized && handle->tx_busy;
}

bool PHAL_USART_rxBusy(const PHAL_USART_Handle_t *handle) {
    return handle != NULL && handle->initialized && handle->rx_busy;
}

__attribute__((weak)) void PHAL_USART_transmitCompleteCallback(
    PHAL_USART_Handle_t *handle,
    bool success
) {
    (void)handle;
    (void)success;
}

__attribute__((weak)) void PHAL_USART_receiveCompleteCallback(
    PHAL_USART_Handle_t *handle,
    bool success,
    size_t received_length
) {
    (void)handle;
    (void)success;
    (void)received_length;
}

void USART1_IRQHandler(void) { PHAL_USART_internalHandleIRQ(USART1); }
void USART2_IRQHandler(void) { PHAL_USART_internalHandleIRQ(USART2); }
void USART3_IRQHandler(void) { PHAL_USART_internalHandleIRQ(USART3); }
