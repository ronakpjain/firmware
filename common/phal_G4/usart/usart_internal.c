#include "common/phal_G4/usart/usart_internal.h"

#include <stddef.h>

#include "common/phal_G4/rcc/rcc.h"

static PHAL_USART_State_t states[PHAL_USART_SLOT_COUNT];

static bool supported_instance(USART_TypeDef *instance) {
    return instance == USART1 || instance == USART2 || instance == USART3;
}

static PHAL_USART_State_t *state_for_handle(PHAL_USART_Handle_t *handle) {
    if (handle == NULL) {
        return NULL;
    }
    for (size_t i = 0U; i < PHAL_USART_SLOT_COUNT; ++i) {
        if (states[i].registered && states[i].handle == handle) {
            return &states[i];
        }
    }
    return NULL;
}

PHAL_USART_State_t *PHAL_USART_internalStateForInstance(USART_TypeDef *instance) {
    for (size_t i = 0U; i < PHAL_USART_SLOT_COUNT; ++i) {
        if (states[i].registered && states[i].handle->instance == instance) {
            return &states[i];
        }
    }
    return NULL;
}

static PHAL_USART_State_t *allocate_state(PHAL_USART_Handle_t *handle, USART_TypeDef *instance) {
    PHAL_USART_State_t *existing = PHAL_USART_internalStateForInstance(instance);
    if (existing != NULL) {
        return existing->handle == handle && !handle->tx_busy && !handle->rx_busy
            ? existing : NULL;
    }
    existing = state_for_handle(handle);
    if (existing != NULL) {
        return !handle->tx_busy && !handle->rx_busy ? existing : NULL;
    }
    for (size_t i = 0U; i < PHAL_USART_SLOT_COUNT; ++i) {
        if (!states[i].registered) {
            states[i] = (PHAL_USART_State_t) {
                .handle = handle,
                .registered = true,
            };
            return &states[i];
        }
    }
    return NULL;
}

static void enable_clock(USART_TypeDef *instance) {
    if (instance == USART1) {
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    } else if (instance == USART2) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    } else {
        RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN;
    }
    (void)RCC->APB1ENR1;
    (void)RCC->APB2ENR;
}

static uint32_t clock_for_instance(USART_TypeDef *instance) {
    return instance == USART1 ? PHAL_RCC_apb2ClockHz() : PHAL_RCC_apb1ClockHz();
}

static void enable_usart_irq(USART_TypeDef *instance) {
    if (instance == USART1) {
        NVIC_EnableIRQ(USART1_IRQn);
    } else if (instance == USART2) {
        NVIC_EnableIRQ(USART2_IRQn);
    } else if (instance == USART3) {
        NVIC_EnableIRQ(USART3_IRQn);
    }
}

static bool wait_for_disabled(USART_TypeDef *instance) {
    for (uint32_t timeout = PHAL_USART_INTERNAL_TIMEOUT; timeout != 0U; --timeout) {
        if ((instance->CR1 & USART_CR1_UE) == 0U) {
            return true;
        }
    }
    return false;
}

bool PHAL_USART_internalValidateConfig(const PHAL_USART_Config_t *config) {
    return config != NULL && supported_instance(config->instance)
        && config->baud_rate != 0U;
}

bool PHAL_USART_internalPrepareInstance(
    const PHAL_USART_Config_t *config,
    uint32_t *baud_register
) {
    const uint32_t clock_hz = clock_for_instance(config->instance);
    const uint32_t brr = (clock_hz + (config->baud_rate / 2U)) / config->baud_rate;
    if (baud_register == NULL || clock_hz == 0U || brr == 0U || brr > 0xFFFFU) {
        return false;
    }

    enable_clock(config->instance);
    config->instance->CR1 &= ~USART_CR1_UE;
    if (!wait_for_disabled(config->instance)) {
        return false;
    }
    *baud_register = brr;
    return true;
}

bool PHAL_USART_internalInitializeStateAndDma(
    PHAL_USART_Handle_t *handle,
    const PHAL_USART_Config_t *config
) {
    PHAL_USART_State_t *state = allocate_state(handle, config->instance);
    if (state == NULL) {
        return false;
    }

    const PHAL_DMA_Route_t *rx_route = PHAL_DMA_internalUsartRxRoute(config->instance);
    const PHAL_DMA_Route_t *tx_route = PHAL_DMA_internalUsartTxRoute(config->instance);
    if (rx_route == NULL || tx_route == NULL) {
        state->registered = false;
        return false;
    }
    if (!PHAL_DMA_internalInit(&handle->rx_dma, rx_route)) {
        state->registered = false;
        return false;
    }
    if (!PHAL_DMA_internalInit(&handle->tx_dma, tx_route)) {
        (void)PHAL_DMA_internalRelease(&handle->rx_dma);
        state->registered = false;
        return false;
    }
    return true;
}

bool PHAL_USART_internalValidateHardwareConfig(
    const PHAL_USART_InternalConfig_t *config
) {
    return config != NULL && supported_instance(config->instance)
        && config->baud_rate != 0U && config->word_length >= 7U
        && config->word_length <= 9U
        && config->parity <= PHAL_USART_INTERNAL_PARITY_ODD
        && (config->stop_bits == 1U || config->stop_bits == 2U);
}

void PHAL_USART_internalConfigureRegisters(
    PHAL_USART_Handle_t *handle,
    const PHAL_USART_InternalConfig_t *config,
    uint32_t baud_register
) {
    uint32_t cr1 = 0U;
    if (config->parity != PHAL_USART_INTERNAL_PARITY_NONE) {
        cr1 |= USART_CR1_PCE;
        if (config->parity == PHAL_USART_INTERNAL_PARITY_ODD) {
            cr1 |= USART_CR1_PS;
        }
    }
    if (config->word_length == 7U) {
        cr1 |= USART_CR1_M1;
    } else if (config->word_length == 9U) {
        cr1 |= USART_CR1_M0;
    }

    uint32_t cr3 = 0U;
    if (config->hardware_rts) {
        cr3 |= USART_CR3_RTSE;
    }
    if (config->hardware_cts) {
        cr3 |= USART_CR3_CTSE;
    }

    config->instance->BRR = baud_register;
    config->instance->CR1 = cr1;
    config->instance->CR2 = config->stop_bits == 2U ? (2U << USART_CR2_STOP_Pos) : 0U;
    config->instance->CR3 = cr3;
    config->instance->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF
        | USART_ICR_ORECF | USART_ICR_IDLECF | USART_ICR_TCCF;
    config->instance->CR1 |= USART_CR1_UE;

    handle->instance = config->instance;
    handle->tx_busy = false;
    handle->rx_busy = false;
    handle->tx_success = false;
    handle->rx_success = false;
    handle->initialized = true;
    PHAL_USART_State_t *state = state_for_handle(handle);
    if (state == NULL) {
        return;
    }
    state->rx_buffer = NULL;
    state->rx_capacity = 0U;
    state->rx_length = 0U;
    state->rx_idle = false;
    state->tx_dma_complete = false;
    enable_usart_irq(config->instance);
}

static void usart_tx_dma_complete(void *context, bool success) {
    PHAL_USART_Handle_t *handle = (PHAL_USART_Handle_t *)context;
    PHAL_USART_State_t *state = state_for_handle(handle);
    if (state == NULL || !handle->tx_busy) {
        return;
    }
    if (!success) {
        (void)PHAL_USART_internalCompleteTransmit(handle, false);
        return;
    }
    state->tx_dma_complete = true;
    if ((handle->instance->ISR & USART_ISR_TC) != 0U) {
        (void)PHAL_USART_internalCompleteTransmit(handle, true);
    }
}

static void complete_receive(PHAL_USART_Handle_t *handle, bool success, size_t received) {
    PHAL_USART_State_t *state = state_for_handle(handle);
    if (state == NULL || !handle->rx_busy) {
        return;
    }
    handle->instance->CR3 &= ~(USART_CR3_DMAR | USART_CR3_EIE);
    handle->instance->CR1 &= ~(USART_CR1_IDLEIE | USART_CR1_PEIE);
    (void)PHAL_DMA_abort(&handle->rx_dma);
    handle->rx_busy = false;
    handle->rx_success = success;
    const size_t reported = received <= state->rx_capacity ? received : state->rx_capacity;
    PHAL_USART_receiveCompleteCallback(handle, success, reported);
}

static void usart_rx_dma_complete(void *context, bool success) {
    PHAL_USART_Handle_t *handle = (PHAL_USART_Handle_t *)context;
    PHAL_USART_State_t *state = state_for_handle(handle);
    if (state == NULL || !handle->rx_busy) {
        return;
    }
    complete_receive(handle, success, success ? state->rx_length : 0U);
}

bool PHAL_USART_internalValidateTransmit(
    const PHAL_USART_Handle_t *handle,
    const uint8_t *data,
    size_t length
) {
    return handle != NULL && handle->initialized && handle->instance != NULL
        && data != NULL && length != 0U && length <= UINT16_MAX && !handle->tx_busy
        && state_for_handle((PHAL_USART_Handle_t *)handle) != NULL;
}

bool PHAL_USART_internalArmTransmit(
    PHAL_USART_Handle_t *handle,
    const uint8_t *data,
    size_t length
) {
    PHAL_USART_State_t *state = state_for_handle(handle);
    if (state == NULL) {
        return false;
    }
    state->tx_dma_complete = false;
    handle->tx_busy = true;
    handle->tx_success = false;
    handle->instance->ICR = USART_ICR_TCCF;
    handle->instance->CR1 |= USART_CR1_TE | USART_CR1_TCIE;

    if (!PHAL_DMA_internalStart(
            &handle->tx_dma,
            (volatile void *)&handle->instance->TDR,
            (void *)data,
            length,
            true,
            false,
            usart_tx_dma_complete,
            handle)) {
        handle->instance->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
        handle->tx_busy = false;
        return false;
    }
    handle->instance->CR3 |= USART_CR3_DMAT;
    return true;
}

bool PHAL_USART_internalValidateReceive(
    const PHAL_USART_Handle_t *handle,
    const uint8_t *data,
    size_t length
) {
    return handle != NULL && handle->initialized && handle->instance != NULL
        && data != NULL && length != 0U && length <= UINT16_MAX && !handle->rx_busy
        && state_for_handle((PHAL_USART_Handle_t *)handle) != NULL;
}

bool PHAL_USART_internalArmReceive(
    PHAL_USART_Handle_t *handle,
    uint8_t *data,
    size_t length,
    bool idle
) {
    PHAL_USART_State_t *state = state_for_handle(handle);
    if (state == NULL) {
        return false;
    }
    state->rx_buffer = data;
    state->rx_capacity = length;
    state->rx_length = length;
    state->rx_idle = idle;
    handle->rx_busy = true;
    handle->rx_success = false;
    handle->instance->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF
        | USART_ICR_ORECF | USART_ICR_IDLECF;
    handle->instance->CR1 |= USART_CR1_RE;

    if (!PHAL_DMA_internalStart(
            &handle->rx_dma,
            (volatile void *)&handle->instance->RDR,
            data,
            length,
            true,
            false,
            usart_rx_dma_complete,
            handle)) {
        handle->rx_busy = false;
        return false;
    }
    handle->instance->CR3 |= USART_CR3_DMAR | USART_CR3_EIE;
    handle->instance->CR1 |= USART_CR1_PEIE;
    if (idle) {
        handle->instance->CR1 |= USART_CR1_IDLEIE;
    } else {
        handle->instance->CR1 &= ~USART_CR1_IDLEIE;
    }
    return true;
}

bool PHAL_USART_internalValidateHandle(const PHAL_USART_Handle_t *handle) {
    return handle != NULL && handle->initialized && handle->instance != NULL
        && state_for_handle((PHAL_USART_Handle_t *)handle) != NULL;
}

void PHAL_USART_internalDisableReceiveRequests(PHAL_USART_Handle_t *handle) {
    handle->instance->CR3 &= ~(USART_CR3_DMAR | USART_CR3_EIE);
    handle->instance->CR1 &= ~(USART_CR1_IDLEIE | USART_CR1_PEIE);
}

void PHAL_USART_internalCompleteStopReceive(PHAL_USART_Handle_t *handle) {
    PHAL_USART_State_t *state = state_for_handle(handle);
    if (handle == NULL || state == NULL) {
        return;
    }
    handle->rx_busy = false;
    state->rx_idle = false;
}

bool PHAL_USART_internalCompleteTransmit(PHAL_USART_Handle_t *handle, bool success) {
    PHAL_USART_State_t *state = state_for_handle(handle);
    if (state == NULL || !handle->tx_busy) {
        return false;
    }
    if (!success) {
        (void)PHAL_DMA_abort(&handle->tx_dma);
    }
    handle->instance->CR3 &= ~USART_CR3_DMAT;
    handle->instance->CR1 &= ~(USART_CR1_TE | USART_CR1_TCIE);
    handle->instance->ICR = USART_ICR_TCCF;
    handle->tx_busy = false;
    handle->tx_success = success;
    state->tx_dma_complete = false;
    PHAL_USART_transmitCompleteCallback(handle, success);
    return success;
}

void PHAL_USART_internalHandleIRQ(USART_TypeDef *instance) {
    PHAL_USART_State_t *state = PHAL_USART_internalStateForInstance(instance);
    if (state == NULL) {
        return;
    }
    PHAL_USART_Handle_t *handle = state->handle;
    const uint32_t isr = instance->ISR;
    const uint32_t error_status = isr & (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE);
    if (error_status != 0U) {
        uint32_t error_clear = 0U;
        if ((error_status & USART_ISR_PE) != 0U) error_clear |= USART_ICR_PECF;
        if ((error_status & USART_ISR_FE) != 0U) error_clear |= USART_ICR_FECF;
        if ((error_status & USART_ISR_NE) != 0U) error_clear |= USART_ICR_NECF;
        if ((error_status & USART_ISR_ORE) != 0U) error_clear |= USART_ICR_ORECF;
        instance->ICR = error_clear;
        if (handle->tx_busy) {
            (void)PHAL_USART_internalCompleteTransmit(handle, false);
        }
        if (handle->rx_busy) {
            const size_t remaining = PHAL_DMA_internalRemaining(&handle->rx_dma);
            const size_t received = state->rx_capacity >= remaining
                ? state->rx_capacity - remaining : 0U;
            complete_receive(handle, false, received);
        }
    }

    if (handle->tx_busy && state->tx_dma_complete && (isr & USART_ISR_TC) != 0U) {
        (void)PHAL_USART_internalCompleteTransmit(handle, true);
    }

    if ((isr & USART_ISR_IDLE) != 0U) {
        instance->ICR = USART_ICR_IDLECF;
        if (handle->rx_busy && state->rx_idle) {
            const size_t remaining = PHAL_DMA_internalRemaining(&handle->rx_dma);
            const size_t received = state->rx_capacity >= remaining
                ? state->rx_capacity - remaining : 0U;
            complete_receive(handle, true, received);
        }
    }
}
