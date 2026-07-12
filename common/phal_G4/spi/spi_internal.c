#include "common/phal_G4/spi/spi_internal.h"

#include "common/phal_G4/rcc/rcc.h"

#define PHAL_SPI_STATE_COUNT 8U

typedef struct {
    PHAL_SPI_Handle_t *handle;
    bool registered;
    bool tx_complete;
    bool rx_complete;
} PHAL_SPI_TransferState_t;

static PHAL_SPI_TransferState_t transfer_states[PHAL_SPI_STATE_COUNT];
static uint8_t dma_zero;
static volatile uint8_t dma_discard;

static PHAL_SPI_TransferState_t *state_for_handle(PHAL_SPI_Handle_t *handle, bool create) {
    for (size_t i = 0U; i < PHAL_SPI_STATE_COUNT; ++i) {
        if (transfer_states[i].registered && transfer_states[i].handle == handle) {
            return &transfer_states[i];
        }
    }
    if (!create) {
        return NULL;
    }
    for (size_t i = 0U; i < PHAL_SPI_STATE_COUNT; ++i) {
        if (!transfer_states[i].registered) {
            transfer_states[i] = (PHAL_SPI_TransferState_t) {
                .handle = handle,
                .registered = true,
                .tx_complete = false,
                .rx_complete = false,
            };
            return &transfer_states[i];
        }
    }
    return NULL;
}

static bool supported_instance(SPI_TypeDef *instance) {
    return instance == SPI1 || instance == SPI2 || instance == SPI3;
}

static bool enable_clock(SPI_TypeDef *instance) {
    if (instance == SPI1) {
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    } else if (instance == SPI2) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;
    } else if (instance == SPI3) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_SPI3EN;
    } else {
        return false;
    }
    (void)RCC->APB1ENR1;
    (void)RCC->APB2ENR;
    return true;
}

static void set_chip_select(const PHAL_SPI_Handle_t *handle, bool asserted) {
    if (handle->software_chip_select && handle->chip_select_port != NULL) {
        PHAL_writeGPIO(
            handle->chip_select_port,
            handle->chip_select_pin,
            !asserted
        );
    }
}

static bool select_baud_rate(uint32_t input_hz, uint32_t requested_hz, uint32_t *br) {
    if (input_hz == 0U || requested_hz == 0U || br == NULL) {
        return false;
    }

    uint32_t divisor = 2U;
    for (uint32_t candidate = 0U; candidate <= 7U; ++candidate) {
        if (input_hz / divisor <= requested_hz) {
            *br = candidate;
            return true;
        }
        divisor <<= 1U;
    }
    return false;
}

bool PHAL_SPI_internalValidateConfig(const PHAL_SPI_Config_t *config) {
    return config != NULL && supported_instance(config->instance)
        && config->mode <= PHAL_SPI_MODE_SLAVE && config->frame_size_bits == 8U
        && (!config->software_chip_select || config->chip_select_port != NULL);
}

bool PHAL_SPI_internalConfigureRegisters(const PHAL_SPI_Config_t *config) {
    const uint32_t input_hz = config->instance == SPI1
        ? PHAL_RCC_apb2ClockHz()
        : PHAL_RCC_apb1ClockHz();
    uint32_t baud_encoding;
    if (!select_baud_rate(input_hz, config->data_rate_hz, &baud_encoding)
        || !enable_clock(config->instance)) {
        return false;
    }

    config->instance->CR1 &= ~SPI_CR1_SPE;
    for (uint32_t timeout = PHAL_SPI_INTERNAL_TIMEOUT; timeout != 0U; --timeout) {
        if ((config->instance->SR & SPI_SR_BSY) == 0U) {
            break;
        }
        if (timeout == 1U) {
            return false;
        }
    }

    uint32_t cr1 = SPI_CR1_BR_Msk | SPI_CR1_CPHA | SPI_CR1_CPOL
        | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
        | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE | SPI_CR1_RXONLY;
    uint32_t cr1_value = 0U;
    if (config->mode == PHAL_SPI_MODE_MASTER) {
        cr1_value |= SPI_CR1_MSTR;
        cr1_value |= (baud_encoding << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk;
    }
    if (config->clock_polarity_high) {
        cr1_value |= SPI_CR1_CPOL;
    }
    if (config->clock_phase_second_edge) {
        cr1_value |= SPI_CR1_CPHA;
    }
    if (config->software_chip_select) {
        cr1_value |= SPI_CR1_SSM;
        if (config->mode == PHAL_SPI_MODE_MASTER) {
            cr1_value |= SPI_CR1_SSI;
        }
    }
    config->instance->CR1 = (config->instance->CR1 & ~cr1) | cr1_value;

    uint32_t cr2 = config->instance->CR2;
    cr2 &= ~(SPI_CR2_DS_Msk | SPI_CR2_FRXTH | SPI_CR2_SSOE
             | SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
    cr2 |= (7U << SPI_CR2_DS_Pos) & SPI_CR2_DS_Msk;
    cr2 |= SPI_CR2_FRXTH;
    if (config->mode == PHAL_SPI_MODE_MASTER && !config->software_chip_select) {
        cr2 |= SPI_CR2_SSOE;
    }
    config->instance->CR2 = cr2;

    return true;
}

bool PHAL_SPI_internalInitializeDma(PHAL_SPI_Handle_t *handle, SPI_TypeDef *instance) {
    const PHAL_DMA_Route_t *rx_route = PHAL_DMA_internalSpiRxRoute(instance);
    const PHAL_DMA_Route_t *tx_route = PHAL_DMA_internalSpiTxRoute(instance);
    if (rx_route == NULL || tx_route == NULL) {
        return false;
    }
    if (!PHAL_DMA_internalInit(&handle->rx_dma, rx_route)) {
        return false;
    }
    if (!PHAL_DMA_internalInit(&handle->tx_dma, tx_route)) {
        (void)PHAL_DMA_internalRelease(&handle->rx_dma);
        return false;
    }
    return true;
}

void PHAL_SPI_internalInitializeHandle(
    PHAL_SPI_Handle_t *handle,
    const PHAL_SPI_Config_t *config
) {
    handle->instance = config->instance;
    handle->chip_select_port = config->chip_select_port;
    handle->chip_select_pin = config->chip_select_pin;
    handle->software_chip_select = config->software_chip_select;
    handle->initialized = true;
    handle->busy = false;
    handle->transfer_success = false;
    set_chip_select(handle, false);
}

static void spi_tx_complete(void *context, bool success);
static void spi_rx_complete(void *context, bool success);

bool PHAL_SPI_internalValidateTransfer(
    const PHAL_SPI_Handle_t *handle,
    size_t length
) {
    return handle != NULL && handle->initialized && handle->instance != NULL
        && !handle->busy && length != 0U && length <= UINT16_MAX;
}

bool PHAL_SPI_internalPrepareTransfer(PHAL_SPI_Handle_t *handle) {
    PHAL_SPI_TransferState_t *state = state_for_handle(handle, true);
    if (state == NULL) {
        return false;
    }
    state->tx_complete = false;
    state->rx_complete = false;
    set_chip_select(handle, true);
    handle->busy = true;
    handle->transfer_success = false;
    return true;
}

bool PHAL_SPI_internalStartReceiveDma(
    PHAL_SPI_Handle_t *handle,
    uint8_t *rx_data,
    size_t length
) {
    return PHAL_DMA_internalStart(
        &handle->rx_dma,
        (volatile void *)&handle->instance->DR,
        rx_data != NULL ? (void *)rx_data : (void *)&dma_discard,
        length,
        rx_data != NULL,
        false,
        spi_rx_complete,
        handle);
}

bool PHAL_SPI_internalStartTransmitDma(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *tx_data,
    size_t length
) {
    return PHAL_DMA_internalStart(
        &handle->tx_dma,
        (volatile void *)&handle->instance->DR,
        tx_data != NULL ? (void *)tx_data : (void *)&dma_zero,
        length,
        tx_data != NULL,
        false,
        spi_tx_complete,
        handle);
}

void PHAL_SPI_internalEnableTransfer(PHAL_SPI_Handle_t *handle) {
    /* DMA channels are armed while SPI request generation remains masked. */
    handle->instance->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
    handle->instance->CR1 |= SPI_CR1_SPE;
}

void PHAL_SPI_internalCancelTransferSetup(PHAL_SPI_Handle_t *handle, bool abort_receive) {
    if (abort_receive) {
        (void)PHAL_DMA_abort(&handle->rx_dma);
    }
    handle->busy = false;
    set_chip_select(handle, false);

    PHAL_SPI_TransferState_t *state = state_for_handle(handle, false);
    if (state != NULL) {
        state->registered = false;
    }
}

bool PHAL_SPI_internalValidateHandle(const PHAL_SPI_Handle_t *handle) {
    return handle != NULL && handle->initialized && handle->instance != NULL;
}

void PHAL_SPI_internalDisablePeripheral(PHAL_SPI_Handle_t *handle) {
    handle->instance->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
    handle->instance->CR1 &= ~SPI_CR1_SPE;
}

bool PHAL_SPI_internalTeardown(PHAL_SPI_Handle_t *handle, bool success) {
    if (handle == NULL || handle->instance == NULL) {
        return false;
    }

    bool result = success;
    for (uint32_t timeout = PHAL_SPI_INTERNAL_TIMEOUT; timeout != 0U; --timeout) {
        const uint32_t status = handle->instance->SR;
        if ((status & SPI_SR_BSY) == 0U && (status & SPI_SR_TXE) != 0U) {
            break;
        }
        if (timeout == 1U) {
            result = false;
        }
    }

    handle->instance->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
    if (!PHAL_DMA_abort(&handle->rx_dma) || !PHAL_DMA_abort(&handle->tx_dma)) {
        result = false;
    }

    if (handle->instance->SR & SPI_SR_OVR) {
        (void)*(volatile uint8_t *)&handle->instance->DR;
        (void)handle->instance->SR;
        result = false;
    }
    handle->instance->CR1 &= ~SPI_CR1_SPE;
    set_chip_select(handle, false);
    handle->transfer_success = result;
    handle->busy = false;

    PHAL_SPI_TransferState_t *state = state_for_handle(handle, false);
    if (state != NULL) {
        state->registered = false;
    }
    return result;
}

static void spi_tx_complete(void *context, bool success) {
    PHAL_SPI_Handle_t *handle = (PHAL_SPI_Handle_t *)context;
    PHAL_SPI_TransferState_t *state = state_for_handle(handle, false);
    if (state == NULL || !handle->busy) {
        return;
    }
    if (!success) {
        (void)PHAL_SPI_internalTeardown(handle, false);
        return;
    }
    state->tx_complete = true;
    if (state->rx_complete) {
        (void)PHAL_SPI_internalTeardown(handle, true);
    }
}

static void spi_rx_complete(void *context, bool success) {
    PHAL_SPI_Handle_t *handle = (PHAL_SPI_Handle_t *)context;
    PHAL_SPI_TransferState_t *state = state_for_handle(handle, false);
    if (state == NULL || !handle->busy) {
        return;
    }
    if (!success) {
        (void)PHAL_SPI_internalTeardown(handle, false);
        return;
    }
    state->rx_complete = true;
    if (state->tx_complete) {
        (void)PHAL_SPI_internalTeardown(handle, true);
    }
}
