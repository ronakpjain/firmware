#ifndef __PHAL_G4_FDCAN_H__
#define __PHAL_G4_FDCAN_H__

#include "common/phal_G4/phal_G4.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Classic CAN frame exchanged with the public FDCAN API. */
typedef struct {
    uint32_t id;       /**< 11-bit standard or 29-bit extended identifier. */
    bool extended;     /**< Use the extended identifier format when true. */
    uint8_t length;    /**< Payload length from zero through eight bytes. */
    uint8_t data[8];   /**< Payload bytes; entries beyond length are ignored. */
} PHAL_CAN_Message_t;

/** Semantic configuration for one FDCAN controller. */
typedef struct {
    uint32_t bit_rate; /**< Nominal arbitration bit rate in bits per second. */
    bool loopback;     /**< Enable internal silent loopback when true. */
} PHAL_CAN_Config_t;

/** Exact-match acceptance-filter lists for FIFO0. */
typedef struct {
    const uint32_t *standard_ids; /**< Array of 11-bit identifiers. */
    size_t standard_id_count;     /**< Number of standard IDs, up to MAX_NUM_SID_FILTER. */
    const uint32_t *extended_ids; /**< Array of 29-bit identifiers. */
    size_t extended_id_count;     /**< Number of extended IDs, up to MAX_NUM_XID_FILTER. */
} PHAL_CAN_FilterConfig_t;

/** Stable runtime identity for one hardware FDCAN controller. */
typedef struct {
    FDCAN_GlobalTypeDef *instance; /**< Controller register block assigned during initialization. */
    bool initialized;              /**< Whether initialization completed successfully. */
} PHAL_CAN_Handle_t;

/** Fixed public handle for FDCAN1. */
extern PHAL_CAN_Handle_t PHAL_CAN1;
/** Fixed public handle for FDCAN2. */
extern PHAL_CAN_Handle_t PHAL_CAN2;
/** Fixed public handle for FDCAN3. */
extern PHAL_CAN_Handle_t PHAL_CAN3;

/**
 * @brief Initialize one FDCAN controller with its fixed message-RAM partition.
 *
 * The FDCAN kernel clock is queried from RCC, nominal timing is derived from
 * the requested bit rate, and the classic-CAN FIFO0/TX-FIFO layout, interrupt
 * routing, and default accept-all policy are configured during INIT/CCE. The
 * public handle is the stable identity used by callbacks and CAN queues.
 *
 * @param handle One of the fixed controller handles: PHAL_CAN1, PHAL_CAN2, or PHAL_CAN3.
 * @param config Nominal bit rate and optional internal loopback. The peripheral
 *               instance and message-RAM partition are derived from the handle.
 * @return true The controller left INIT and is ready.
 * @return false Invalid argument, unsupported clock/timing, or bounded state timeout.
 * @note This function blocks for bounded INIT/CCE transitions.
 * @note GPIO and NVIC ownership remains with the board/CAN library.
 */
bool PHAL_CAN_init(PHAL_CAN_Handle_t *handle, const PHAL_CAN_Config_t *config);

/**
 * @brief Install exact-match standard and extended FIFO0 filters.
 * @param handle Initialized CAN handle.
 * @param filters Filter ID lists; a null list is valid only with count zero.
 * @return true Filters were written and the controller returned to running state.
 * @return false Invalid arguments or bounded INIT transition failure.
 * @note This function blocks while changing protected filter state.
 */
bool PHAL_CAN_setFilters(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_FilterConfig_t *filters
);

/**
 * @brief Queue one classic CAN frame directly into the hardware TX FIFO.
 * @param handle Initialized CAN handle.
 * @param message Frame with an 11- or 29-bit ID and at most 8 data bytes.
 * @return true The frame was copied and its TX request was issued.
 * @return false Invalid message/handle or hardware TX FIFO full.
 * @note This function does not block and never silently drops a full-FIFO frame.
 */
bool PHAL_CAN_send(PHAL_CAN_Handle_t *handle, const PHAL_CAN_Message_t *message);

/**
 * @brief Return whether the hardware TX FIFO has a free element.
 * @param handle Initialized CAN handle.
 * @return true A TX FIFO element is available.
 * @return false Invalid handle or FIFO full.
 * @note This function does not block.
 */
bool PHAL_CAN_txAvailable(const PHAL_CAN_Handle_t *handle);

/**
 * @brief Receive one frame from a CAN interrupt.
 * @param handle Stable handle identifying the controller.
 * @param message Received frame storage valid for the callback duration.
 * @note Weak default callback executes from FDCAN IT0 context.
 */
void PHAL_CAN_rxCallback(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_Message_t *message
);

/**
 * @brief Notify the CAN library that a frame completed transmission.
 * @param handle Stable handle identifying the controller.
 * @note Weak default callback executes from FDCAN IT1 context.
 */
void PHAL_CAN_txCallback(PHAL_CAN_Handle_t *handle);

/** Maximum number of extended-ID filter elements per controller. */
#define MAX_NUM_XID_FILTER (8)
/** Maximum number of standard-ID filter elements per controller. */
#define MAX_NUM_SID_FILTER (28)

/** Alternate-function number used by FDCAN1 pins. */
#define AF_NUM_FDCAN1 (9)
/** Alternate-function number used by FDCAN2 pins. */
#define AF_NUM_FDCAN2 (9)
/** Alternate-function number used by FDCAN3 pins. */
#define AF_NUM_FDCAN3 (11)

/** @name FDCAN1 GPIO initialization constants
 *  PA11/PA12 and PB8/PB9 pin alternatives. @{ */
#define GPIO_INIT_FDCAN1RX_PA11 \
    GPIO_INIT_AF(GPIOA, \
                 11, \
                 AF_NUM_FDCAN1, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_FDCAN1TX_PA12 \
    GPIO_INIT_AF(GPIOA, \
                 12, \
                 AF_NUM_FDCAN1, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)

/** @} */

/** @warning The PB8/PB9 alternate-function mapping has not been hardware validated.
 *  @name Alternate FDCAN1 PB8/PB9 GPIO initialization constants
 *  @{ */
#define GPIO_INIT_FDCAN1RX_PB8 \
    GPIO_INIT_AF(GPIOB, \
                 8, \
                 AF_NUM_FDCAN1, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_FDCAN1TX_PB9 \
    GPIO_INIT_AF(GPIOB, \
                 9, \
                 AF_NUM_FDCAN1, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)

/** @} */

/** @name FDCAN2 GPIO initialization constants
 *  PB12/PB13 and PB5/PB6 pin alternatives. @{ */
#define GPIO_INIT_FDCAN2RX_PB12 \
    GPIO_INIT_AF(GPIOB, \
                 12, \
                 AF_NUM_FDCAN2, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_FDCAN2TX_PB13 \
    GPIO_INIT_AF(GPIOB, \
                 13, \
                 AF_NUM_FDCAN2, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_FDCAN2RX_PB5 \
    GPIO_INIT_AF(GPIOB, \
                 5, \
                 AF_NUM_FDCAN2, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_FDCAN2TX_PB6 \
    GPIO_INIT_AF(GPIOB, \
                 6, \
                 AF_NUM_FDCAN2, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)

/** @} */

/** @name FDCAN3 GPIO initialization constants
 *  PA8/PB4 and PA15/PB3 pin alternatives. @{ */
#define GPIO_INIT_FDCAN3RX_PA8 \
    GPIO_INIT_AF(GPIOA, \
                 8, \
                 AF_NUM_FDCAN3, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_FDCAN3TX_PB4 \
    GPIO_INIT_AF(GPIOB, \
                 4, \
                 AF_NUM_FDCAN3, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_FDCAN3TX_PA15 \
    GPIO_INIT_AF(GPIOA, \
                 15, \
                 AF_NUM_FDCAN3, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_FDCAN3TX_PB3 \
    GPIO_INIT_AF(GPIOB, \
                 3, \
                 AF_NUM_FDCAN3, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)
/** @} */

#endif // __PHAL_G4_FDCAN_H__