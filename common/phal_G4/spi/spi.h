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
    PHAL_SPI_MODE_MASTER, /**< Generate the serial clock as bus controller. */
    PHAL_SPI_MODE_SLAVE   /**< Receive the serial clock as bus peripheral. */
} PHAL_SPI_Mode_t;

/** Semantic configuration used to initialize an SPI handle. */
typedef struct {
    SPI_TypeDef *instance;       /**< SPI1, SPI2, or SPI3 register block. */
    PHAL_SPI_Mode_t mode;        /**< Controller operating mode. */
    uint32_t data_rate_hz;       /**< Maximum requested serial clock frequency. */
    bool software_chip_select;   /**< Drive chip select through GPIO when true. */
    GPIO_TypeDef *chip_select_port; /**< Software chip-select GPIO port. */
    uint8_t chip_select_pin;     /**< Software chip-select pin number, 0 through 15. */
} PHAL_SPI_Config_t;

/** Runtime state for one initialized SPI controller and its DMA routes. */
typedef struct {
    SPI_TypeDef *instance;       /**< Configured SPI register block. */
    PHAL_DMA_Handle_t tx_dma;    /**< Privately managed transmit DMA handle. */
    PHAL_DMA_Handle_t rx_dma;    /**< Privately managed receive DMA handle. */
    GPIO_TypeDef *chip_select_port; /**< Software chip-select GPIO port. */
    uint8_t chip_select_pin;     /**< Software chip-select pin number. */
    bool software_chip_select;   /**< Whether PHAL controls chip select through GPIO. */
    bool initialized;            /**< Whether initialization completed successfully. */
    volatile bool busy;          /**< Whether a full-duplex transfer is active. */
    volatile bool transfer_success; /**< Result of the most recently completed transfer. */
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
 * @param config SPI instance, mode, rate, and chip-select ownership. PHAL fixes
 *               frames to eight bits and uses clock mode 0 (CPOL=0, CPHA=0).
 * @return true The peripheral and both fixed DMA routes were initialized.
 * @return false Invalid argument, unsupported instance/rate, or occupied DMA route.
 *
 * @note This function blocks for register configuration but performs no transfer.
 * @note GPIO pins must be configured by PHAL_GPIO_init() before use.
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
 * and the saved result returned by PHAL_SPI_transfer_noDMA().
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
 * @brief Perform a synchronous transfer by waiting for DMA completion.
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
bool PHAL_SPI_transfer_noDMA(
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

/**
 * @brief Report completion of an asynchronous SPI transfer.
 * @param handle SPI handle whose transfer completed.
 * @param success Whether both DMA directions and SPI teardown succeeded.
 * @note Weak default callback; override it in application code when asynchronous
 *       completion handling is required.
 * @note Executes from a DMA interrupt context. Keep the implementation bounded
 *       and use interrupt-safe synchronization primitives.
 * @note The callback is also invoked with false when an active transfer is aborted.
 */
void PHAL_SPI_transferCompleteCallback(PHAL_SPI_Handle_t *handle, bool success);

#endif
