#include "common/phal_G4/spi/spi_internal.h"

#include "common/phal_G4/rcc/rcc.h"

bool PHAL_SPI_init(PHAL_SPI_Handle_t *handle, const PHAL_SPI_Config_t *config) {
    if (handle == NULL || config == NULL) {
        return false;
    }
    const PHAL_SPI_InternalConfig_t hardware_config = {
        .instance = config->instance,
        .mode = config->mode,
        .data_rate_hz = config->data_rate_hz,
        .frame_size_bits = 8U,
        .clock_polarity_high = false,
        .clock_phase_second_edge = false,
        .software_chip_select = config->software_chip_select,
        .chip_select_port = config->chip_select_port,
        .chip_select_pin = config->chip_select_pin,
    };
    if (!PHAL_SPI_internalValidateConfig(&hardware_config)
        || !PHAL_SPI_internalConfigureRegisters(&hardware_config)
        || !PHAL_SPI_internalInitializeDma(handle, hardware_config.instance)) {
        return false;
    }

    PHAL_SPI_internalInitializeHandle(handle, config);
    return true;
}

bool PHAL_SPI_transfer(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t length
) {
    if (!PHAL_SPI_internalValidateTransfer(handle, length)
        || !PHAL_SPI_internalPrepareTransfer(handle)) {
        return false;
    }
    if (!PHAL_SPI_internalStartReceiveDma(handle, rx_data, length)) {
        PHAL_SPI_internalCancelTransferSetup(handle, false);
        return false;
    }
    if (!PHAL_SPI_internalStartTransmitDma(handle, tx_data, length)) {
        PHAL_SPI_internalCancelTransferSetup(handle, true);
        return false;
    }

    PHAL_SPI_internalEnableTransfer(handle);
    return true;
}

bool PHAL_SPI_transfer_noDMA(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t length,
    uint32_t timeout
) {
    if (!PHAL_SPI_transfer(handle, tx_data, rx_data, length)) {
        return false;
    }

    while (handle->busy && timeout != 0U) {
        --timeout;
    }
    if (handle->busy) {
        (void)PHAL_SPI_abort(handle);
        return false;
    }
    return handle->transfer_success;
}

bool PHAL_SPI_busy(const PHAL_SPI_Handle_t *handle) {
    return handle != NULL && handle->initialized && handle->busy;
}

bool PHAL_SPI_abort(PHAL_SPI_Handle_t *handle) {
    if (!PHAL_SPI_internalValidateHandle(handle)) {
        return false;
    }
    if (!handle->busy) {
        PHAL_SPI_internalDisablePeripheral(handle);
        return true;
    }

    const bool stopped = PHAL_SPI_internalTeardown(handle, false);
    return stopped;
}

bool PHAL_SPI_readRegister(
    PHAL_SPI_Handle_t *handle,
    uint8_t address,
    bool skip_dummy,
    uint8_t *value
) {
    if (value == NULL) {
        return false;
    }
    uint8_t tx_data[3] = {(uint8_t)(0x80U | (address & 0x7FU)), 0U, 0U};
    uint8_t rx_data[3] = {0U, 0U, 0U};
    const size_t transfer_length = skip_dummy ? 2U : 3U;
    if (!PHAL_SPI_transfer_noDMA(
            handle, tx_data, rx_data, transfer_length, 1000000U)) {
        return false;
    }
    *value = rx_data[transfer_length - 1U];
    return true;
}

bool PHAL_SPI_writeRegister(
    PHAL_SPI_Handle_t *handle,
    uint8_t address,
    uint8_t write_data
) {
    const uint8_t tx_data[2] = {(uint8_t)(address & 0x7FU), write_data};
    return PHAL_SPI_transfer_noDMA(handle, tx_data, NULL, 2U, 1000000U);
}

__attribute__((weak)) void PHAL_SPI_transferCompleteCallback(
    PHAL_SPI_Handle_t *handle,
    bool success
) {
    (void)handle;
    (void)success;
}

bool PHAL_SPI_forceReset(PHAL_SPI_Handle_t *handle) {
    if (handle == NULL || !handle->initialized || handle->instance == NULL
        || (handle->instance != SPI1 && handle->instance != SPI2
            && handle->instance != SPI3)) {
        return false;
    }
    if (!PHAL_SPI_abort(handle)) {
        return false;
    }
    if (handle->instance == SPI1) {
        RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
        RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
    } else if (handle->instance == SPI2) {
        RCC->APB1RSTR1 |= RCC_APB1RSTR1_SPI2RST;
        RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_SPI2RST;
    } else {
        RCC->APB1RSTR1 |= RCC_APB1RSTR1_SPI3RST;
        RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_SPI3RST;
    }
    handle->busy = false;
    handle->transfer_success = false;
    return true;
}
