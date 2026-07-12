#ifndef PHAL_G4_SPI_H
#define PHAL_G4_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/phal_G4/dma/dma.h"
#include "common/phal_G4/gpio/gpio.h"
#include "common/phal_G4/phal_G4.h"

/** SPI controller operating mode. */
typedef enum {
    PHAL_SPI_MODE_MASTER,
    PHAL_SPI_MODE_SLAVE
} PHAL_SPI_Mode_t;

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
} PHAL_SPI_Config_t;

typedef struct {
    SPI_TypeDef *instance;
    PHAL_DMA_Handle_t tx_dma;
    PHAL_DMA_Handle_t rx_dma;
    GPIO_TypeDef *chip_select_port;
    uint8_t chip_select_pin;
    bool software_chip_select;
    bool initialized;
    volatile bool busy;
    volatile bool transfer_success;
} PHAL_SPI_Handle_t;

/**
 * @brief Initialize one G4 SPI instance for byte-wide full-duplex DMA transfers.
 *
 * The SPI input clock is obtained from PHAL_RCC_apb1ClockHz() or
 * PHAL_RCC_apb2ClockHz(), and the nearest supported divider not exceeding the
 * requested rate is programmed. The fixed DMA controller, channel, request,
 * and IRQ are selected privately from the SPI instance. PHAL owns the SPI
 * peripheral registers and optionally the software chip-select GPIO.
 *
 * @param handle Storage-backed SPI handle that remains valid after initialization.
 * @param config Semantic SPI configuration; frame_size_bits must currently be 8.
 * @return true The peripheral and both fixed DMA routes were initialized.
 * @return false Invalid argument, unsupported instance/rate, or occupied DMA route.
 *
 * @note This function blocks for register configuration but performs no transfer.
 * @note GPIO pins must be configured by PHAL_GPIO_initGPIO before use.
 * @note DMA IRQ handlers are installed centrally by the G4 DMA module.
 */
bool PHAL_SPI_init(PHAL_SPI_Handle_t *handle, const PHAL_SPI_Config_t *config);

/**
 * @brief Start an asynchronous full-duplex SPI DMA transfer.
 *
 * @param handle Initialized, idle SPI handle.
 * @param tx_data Source bytes, or NULL to transmit zero bytes.
 * @param rx_data Destination bytes, or NULL to discard received bytes.
 * @param length Number of bytes; both buffers must remain valid until completion.
 * @return true The transfer was armed.
 * @return false Invalid state/argument, busy handle, or DMA setup failure.
 *
 * @note This function does not block. Completion is reflected by PHAL_SPI_busy()
 * and the saved result returned by PHAL_SPI_transferBlocking().
 * @note When software_chip_select is enabled, PHAL asserts/releases the GPIO
 * around the complete transfer.
 */
bool PHAL_SPI_transfer(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t length
);

/**
 * @brief Start a DMA transfer and wait for its bounded completion.
 *
 * @param handle Initialized SPI handle.
 * @param tx_data Source bytes, or NULL for zero-filled transmission.
 * @param rx_data Destination bytes, or NULL to discard reception.
 * @param length Number of bytes; buffers remain valid while this function runs.
 * @param timeout Maximum loop iterations spent waiting for completion.
 * @return true The DMA transfer completed and SPI BSY cleared.
 * @return false Invalid state, timeout, overrun, DMA failure, or SPI failure.
 *
 * @note This function blocks and aborts the transfer when timeout reaches zero.
 */
bool PHAL_SPI_transferBlocking(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *tx_data,
    uint8_t *rx_data,
    size_t length,
    uint32_t timeout
);

/**
 * @brief Return whether an SPI transfer is active.
 * @param handle SPI handle.
 * @return true A DMA transfer is active.
 * @return false The handle is invalid, uninitialized, or idle.
 * @note This function does not block.
 */
bool PHAL_SPI_busy(const PHAL_SPI_Handle_t *handle);

/**
 * @brief Abort an active SPI transfer and leave the peripheral disabled.
 * @param handle Initialized SPI handle.
 * @return true The DMA channels and SPI peripheral were stopped.
 * @return false Invalid handle or a bounded hardware wait failed.
 * @note This function blocks for bounded BSY/channel-disable waits and does not invoke callbacks.
 */
bool PHAL_SPI_abort(PHAL_SPI_Handle_t *handle);

/**
 * @brief Read one register through a blocking DMA transaction.
 * @param handle Initialized SPI handle.
 * @param address Seven-bit register address.
 * @param skip_dummy Whether the device requires one rather than two dummy bytes.
 * @param value Destination for the received register value.
 * @return true The transaction completed successfully.
 * @return false Invalid arguments, timeout, DMA failure, or SPI failure.
 * @note This function blocks and uses the handle's fixed DMA routes.
 */
bool PHAL_SPI_readRegister(
    PHAL_SPI_Handle_t *handle,
    uint8_t address,
    bool skip_dummy,
    uint8_t *value
);

/**
 * @brief Write one register through a blocking DMA transaction.
 * @param handle Initialized SPI handle.
 * @param address Seven-bit register address.
 * @param write_data Register value.
 * @return true The transaction completed successfully.
 * @return false Invalid arguments, timeout, DMA failure, or SPI failure.
 * @note This function blocks and uses the handle's fixed DMA routes.
 */
bool PHAL_SPI_writeRegister(
    PHAL_SPI_Handle_t *handle,
    uint8_t address,
    uint8_t write_data
);

/**
 * @brief Reset an SPI peripheral through its RCC reset bit.
 * @param handle Initialized SPI handle identifying the instance.
 * @return true DMA was idle or aborted and the peripheral was reset.
 * @return false The handle was invalid or an active transfer could not be aborted.
 * @note GPIO configuration is not changed.
 */
bool PHAL_SPI_forceReset(PHAL_SPI_Handle_t *handle);

/** Deprecated ambiguous-value compatibility wrappers. */
uint8_t PHAL_SPI_readByte(PHAL_SPI_Handle_t *handle, uint8_t address, bool skip_dummy);
uint8_t PHAL_SPI_writeByte(PHAL_SPI_Handle_t *handle, uint8_t address, uint8_t write_data);
void PHAL_SPI_ForceReset(PHAL_SPI_Handle_t *handle);

/**
 * @brief Deprecated command-plus-dummy compatibility wrapper implemented through DMA.
 * @param handle Initialized SPI handle.
 * @param out_data Bytes transmitted during the command phase, or NULL to transmit zeroes.
 * @param tx_length Number of command-phase bytes.
 * @param rx_length Number of additional zero-filled clocks after the command phase.
 * @param in_data Optional buffer for all `tx_length + rx_length` received bytes.
 * @return true when the DMA transfer completed successfully; false on invalid input, timeout,
 *         hardware failure, or a total transfer longer than 256 bytes.
 * @note This function blocks. Migrate callers to PHAL_SPI_transferBlocking().
 */
bool PHAL_SPI_transfer_noDMA(
    PHAL_SPI_Handle_t *handle,
    const uint8_t *out_data,
    uint32_t tx_length,
    uint32_t rx_length,
    uint8_t *in_data
);

#endif
