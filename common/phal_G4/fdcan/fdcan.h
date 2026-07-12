#ifndef __PHAL_G4_FDCAN_H__
#define __PHAL_G4_FDCAN_H__

#include "common/phal_G4/phal_G4.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t id;
    bool extended;
    uint8_t length;
    uint8_t data[8];
} PHAL_CAN_Message_t;

typedef struct {
    uint32_t bit_rate;
    bool loopback;
} PHAL_CAN_Config_t;

typedef struct {
    const uint32_t *standard_ids;
    size_t standard_id_count;
    const uint32_t *extended_ids;
    size_t extended_id_count;
} PHAL_CAN_FilterConfig_t;

typedef struct {
    FDCAN_GlobalTypeDef *instance;
    bool initialized;
} PHAL_CAN_Handle_t;

extern PHAL_CAN_Handle_t PHAL_CAN1;
extern PHAL_CAN_Handle_t PHAL_CAN2;
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

#define MAX_NUM_XID_FILTER (8)
#define MAX_NUM_SID_FILTER (28)

#define AF_NUM_FDCAN1 (9)
#define AF_NUM_FDCAN2 (9)
#define AF_NUM_FDCAN3 (11)

// FDCAN1 GPIO definitions (PA11/PA12 or PB8/PB9)
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

// !!! double check the AF number for PB8/PB9, not validated yet
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

// FDCAN2 GPIO definitions (PB12/PB13 or PB5/PB6)
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

// FDCAN3 GPIO definitions (PA8/PB4 or PA15/PB3)
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

#endif // __PHAL_G4_FDCAN_H__