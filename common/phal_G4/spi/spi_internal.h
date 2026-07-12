#ifndef PHAL_G4_SPI_INTERNAL_H
#define PHAL_G4_SPI_INTERNAL_H

#include "common/phal_G4/spi/spi.h"
#include "common/phal_G4/dma/dma_internal.h"

#define PHAL_SPI_INTERNAL_TIMEOUT 100000U

typedef struct {
    SPI_TypeDef *instance;
    PHAL_SPI_Mode_t mode;
    uint32_t data_rate_hz;
    uint8_t frame_size_bits;
    bool clock_polarity_high;
    bool clock_phase_second_edge;
    bool software_chip_select;
    GPIO_TypeDef *chip_select_port;
    uint8_t chip_select_pin;
} PHAL_SPI_InternalConfig_t;

bool PHAL_SPI_internalValidateConfig(const PHAL_SPI_InternalConfig_t *config);
bool PHAL_SPI_internalConfigureRegisters(const PHAL_SPI_InternalConfig_t *config);
bool PHAL_SPI_internalInitializeDma(PHAL_SPI_Handle_t *handle, SPI_TypeDef *instance);
void PHAL_SPI_internalInitializeHandle(
    PHAL_SPI_Handle_t *handle,
    const PHAL_SPI_Config_t *config
);

bool PHAL_SPI_internalValidateTransfer(
    const PHAL_SPI_Handle_t *handle,
    size_t length
);
bool PHAL_SPI_internalPrepareTransfer(PHAL_SPI_Handle_t *handle);
bool PHAL_SPI_internalStartReceiveDma(
    PHAL_SPI_Handle_t *handle,
    uint8_t *rx_data,
    size_t length
);
bool PHAL_SPI_internalStartTransmitDma(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *tx_data,
    size_t length
);
void PHAL_SPI_internalEnableTransfer(PHAL_SPI_Handle_t *handle);
void PHAL_SPI_internalCancelTransferSetup(PHAL_SPI_Handle_t *handle, bool abort_receive);

bool PHAL_SPI_internalValidateHandle(const PHAL_SPI_Handle_t *handle);
void PHAL_SPI_internalDisablePeripheral(PHAL_SPI_Handle_t *handle);
bool PHAL_SPI_internalTeardown(PHAL_SPI_Handle_t *handle, bool success);

#endif
