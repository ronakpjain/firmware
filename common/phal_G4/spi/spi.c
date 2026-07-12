#include "common/phal_G4/spi/spi_internal.h"

#include <string.h>

#include "common/phal_G4/rcc/rcc.h"

#define PHAL_SPI_COMPAT_TRANSFER_MAX 256U

bool PHAL_SPI_init(PHAL_SPI_Handle_t *handle, const PHAL_SPI_Config_t *config) {
    if (handle == NULL || !PHAL_SPI_internalValidateConfig(config)
        || !PHAL_SPI_internalConfigureRegisters(config)
        || !PHAL_SPI_internalInitializeDma(handle, config->instance)) {
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

bool PHAL_SPI_transferBlocking(
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

bool PHAL_SPI_transfer_noDMA(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *out_data,
    uint32_t tx_length,
    uint32_t rx_length,
    uint8_t *in_data
) {
    const uint64_t total64 = (uint64_t)tx_length + rx_length;
    if (total64 == 0U || total64 > UINT16_MAX) {
        return false;
    }

    const size_t total = (size_t)total64;
    if (total > PHAL_SPI_COMPAT_TRANSFER_MAX) {
        return false;
    }
    uint8_t tx_buffer[PHAL_SPI_COMPAT_TRANSFER_MAX] = {0};
    if (out_data != NULL && tx_length != 0U) {
        memcpy(tx_buffer, out_data, tx_length);
    }
    return PHAL_SPI_transferBlocking(handle, tx_buffer, in_data, total, 1000000U);
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
    const uint32_t rx_length = skip_dummy ? 1U : 2U;
    if (!PHAL_SPI_transfer_noDMA(handle, tx_data, 1U, rx_length, rx_data)) {
        return false;
    }
    *value = skip_dummy ? rx_data[1] : rx_data[2];
    return true;
}

bool PHAL_SPI_writeRegister(
    PHAL_SPI_Handle_t *handle,
    uint8_t address,
    uint8_t write_data
) {
    const uint8_t tx_data[2] = {(uint8_t)(address & 0x7FU), write_data};
    return PHAL_SPI_transferBlocking(handle, tx_data, NULL, 2U, 1000000U);
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

uint8_t PHAL_SPI_readByte(PHAL_SPI_Handle_t *handle, uint8_t address, bool skip_dummy) {
    uint8_t value = 0U;
    (void)PHAL_SPI_readRegister(handle, address, skip_dummy, &value);
    return value;
}

uint8_t PHAL_SPI_writeByte(PHAL_SPI_Handle_t *handle, uint8_t address, uint8_t write_data) {
    const uint8_t tx_data[2] = {(uint8_t)(address & 0x7FU), write_data};
    uint8_t rx_data[2] = {0U, 0U};
    return PHAL_SPI_transferBlocking(handle, tx_data, rx_data, 2U, 1000000U)
        ? rx_data[1] : 0U;
}

void PHAL_SPI_ForceReset(PHAL_SPI_Handle_t *handle) {
    (void)PHAL_SPI_forceReset(handle);
}
