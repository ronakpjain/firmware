#ifndef PHAL_G4_USART_H
#define PHAL_G4_USART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/phal_G4/dma/dma.h"
#include "common/phal_G4/phal_G4.h"

typedef enum {
    PHAL_USART_PARITY_NONE,
    PHAL_USART_PARITY_EVEN,
    PHAL_USART_PARITY_ODD
} PHAL_USART_Parity_t;

typedef enum {
    PHAL_USART_STOP_BITS_1,
    PHAL_USART_STOP_BITS_2
} PHAL_USART_StopBits_t;

typedef struct {
    USART_TypeDef *instance;
    uint32_t baud_rate;
    uint8_t word_length;
    PHAL_USART_Parity_t parity;
    PHAL_USART_StopBits_t stop_bits;
    bool hardware_rts;
    bool hardware_cts;
} PHAL_USART_Config_t;

typedef struct {
    USART_TypeDef *instance;
    PHAL_DMA_Handle_t tx_dma;
    PHAL_DMA_Handle_t rx_dma;
    volatile bool tx_busy;
    volatile bool rx_busy;
    volatile bool tx_success;
    volatile bool rx_success;
    bool initialized;
} PHAL_USART_Handle_t;

/**
 * @brief Initialize USART1, USART2, or USART3 for byte-wide DMA transfers.
 *
 * The APB clock is obtained from PHAL_RCC_apb1ClockHz() or
 * PHAL_RCC_apb2ClockHz(), and BRR is calculated with the STM32 oversampling-by
 * 16 formula. Fixed DMA channels, requests, and IRQs are selected privately
 * from the USART instance. Hardware RTS/CTS are configured from the semantic
 * flags; receive and idle interrupts remain disabled until an operation starts.
 *
 * @param handle Storage-backed USART handle that remains valid after initialization.
 * @param config USART settings; word_length must be 7, 8, or 9.
 * @return true The peripheral and both fixed DMA routes were initialized.
 * @return false Invalid configuration, unsupported instance, baud, or DMA collision.
 * @note This function blocks for register setup but does not transmit/receive.
 * @note GPIO alternate-function pins must be configured by the caller.
 */
bool PHAL_USART_init(PHAL_USART_Handle_t *handle, const PHAL_USART_Config_t *config);

/**
 * @brief Start an asynchronous DMA transmission.
 * @param handle Initialized, idle USART handle.
 * @param data Source bytes retained until completion.
 * @param length Number of bytes, 1..65535.
 * @return true DMA transmission was armed.
 * @return false Invalid state/argument or DMA setup failure.
 * @note This function does not block; completion waits for USART TC after DMA.
 */
bool PHAL_USART_transmit(PHAL_USART_Handle_t *handle, const uint8_t *data, size_t length);

/**
 * @brief Transmit through DMA and wait for wire completion.
 * @param handle Initialized USART handle.
 * @param data Source buffer retained while this function runs.
 * @param length Number of bytes.
 * @param timeout Maximum loop iterations spent waiting.
 * @return true DMA and USART TC completed.
 * @return false Invalid state, timeout, or DMA/USART failure.
 * @note This function blocks and aborts on timeout.
 */
bool PHAL_USART_transmitBlocking(
    PHAL_USART_Handle_t *handle,
    const uint8_t *data,
    size_t length,
    uint32_t timeout
);

/**
 * @brief Start an asynchronous fixed-length DMA reception.
 * @param handle Initialized, idle USART handle.
 * @param data Destination buffer retained until completion.
 * @param length Exact number of bytes to receive.
 * @return true DMA reception was armed.
 * @return false Invalid state/argument or DMA setup failure.
 * @note This function does not block and does not use RXNE interrupts.
 */
bool PHAL_USART_receive(PHAL_USART_Handle_t *handle, uint8_t *data, size_t length);

/**
 * @brief Receive a fixed-length DMA buffer and wait for completion.
 * @param handle Initialized USART handle.
 * @param data Destination retained while this function runs.
 * @param length Exact number of bytes.
 * @param timeout Maximum loop iterations spent waiting.
 * @return true The requested bytes were received.
 * @return false Invalid state, timeout, or DMA/USART failure.
 * @note This function blocks.
 */
bool PHAL_USART_receiveBlocking(
    PHAL_USART_Handle_t *handle,
    uint8_t *data,
    size_t length,
    uint32_t timeout
);

/**
 * @brief Arm DMA reception terminated by an IDLE line.
 * @param handle Initialized, idle USART handle.
 * @param data Destination buffer retained until callback/stop.
 * @param capacity Maximum number of bytes to receive.
 * @return true DMA and IDLE interrupt were armed.
 * @return false Invalid state/argument or DMA setup failure.
 * @note This function does not block. The weak receive callback runs in USART IRQ
 * context with the number of received bytes.
 */
bool PHAL_USART_startIdleReceive(PHAL_USART_Handle_t *handle, uint8_t *data, size_t capacity);

/**
 * @brief Stop an active receive or idle receive operation.
 * @param handle Initialized USART handle.
 * @return true DMA/request state was stopped.
 * @return false Invalid handle or bounded DMA stop failure.
 * @note This function blocks for a bounded DMA-disable wait and does not call the callback.
 */
bool PHAL_USART_stopReceive(PHAL_USART_Handle_t *handle);

/**
 * @brief Return whether a USART transmission is active.
 * @param handle USART handle.
 * @return true DMA or final TC completion is pending.
 * @return false Invalid, uninitialized, or idle handle.
 * @note This function does not block.
 */
bool PHAL_USART_txBusy(const PHAL_USART_Handle_t *handle);

/**
 * @brief Return whether a USART reception is active.
 * @param handle USART handle.
 * @return true Fixed-length or idle DMA reception is active.
 * @return false Invalid, uninitialized, or idle handle.
 * @note This function does not block.
 */
bool PHAL_USART_rxBusy(const PHAL_USART_Handle_t *handle);

/**
 * @brief Report completion of a DMA transmission after USART TC.
 * @param handle Completed USART handle.
 * @param success Whether DMA and the wire transfer succeeded.
 * @note Weak default; executes from interrupt context.
 */
void PHAL_USART_transmitCompleteCallback(PHAL_USART_Handle_t *handle, bool success);

/**
 * @brief Report completion of a receive operation.
 * @param handle Completed USART handle.
 * @param success Whether DMA and USART status succeeded.
 * @param received_length Bytes received before DMA/IDLE completion.
 * @note Weak default; executes from DMA or USART interrupt context.
 */
void PHAL_USART_receiveCompleteCallback(
    PHAL_USART_Handle_t *handle,
    bool success,
    size_t received_length
);

#endif
