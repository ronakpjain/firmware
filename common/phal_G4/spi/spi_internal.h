#ifndef PHAL_G4_SPI_INTERNAL_H
#define PHAL_G4_SPI_INTERNAL_H

#include "common/phal_G4/spi/spi.h"
#include "common/phal_G4/dma/dma_internal.h"

#define PHAL_SPI_INTERNAL_TIMEOUT 100000U

bool PHAL_SPI_internalConfigure(
    PHAL_SPI_Handle_t *handle,
    const PHAL_SPI_Config_t *config
);

bool PHAL_SPI_internalStart(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t length
);

bool PHAL_SPI_internalTeardown(PHAL_SPI_Handle_t *handle, bool success);

#endif
