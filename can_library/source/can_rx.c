#include "can_library/can_common.h"
#include "can_library/generated/can_router.h"

volatile can_data_t can_data;
volatile uint32_t last_can_rx_time_ms;

#if defined(STM32F407xx)
#define CAN_RX_MESSAGE_TYPE CanMsgTypeDef_t
typedef CanMsgTypeDef_t can_rx_item_t;
#elif defined(STM32G474xx)
#define CAN_RX_MESSAGE_TYPE PHAL_CAN_Message_t
typedef struct {
    PHAL_CAN_Handle_t *bus;
    PHAL_CAN_Message_t message;
} can_rx_item_t;
#else
#error "Unsupported CAN architecture"
#endif

DEFINE_QUEUE(can_rx_queue, can_rx_item_t, CAN_RX_QUEUE_LENGTH);

void CAN_rx_init(void) {
    INIT_QUEUE(can_rx_queue, can_rx_item_t, CAN_RX_QUEUE_LENGTH);
}

void CAN_rx_update(void) {
    can_rx_item_t item;
    if (xQueueReceive(can_rx_queue, &item, portMAX_DELAY) != pdPASS) {
        return;
    }
    last_can_rx_time_ms = OS_TICKS;
#if defined(STM32F407xx)
    CAN_rx_dispatcher(
        item.IDE == 0U ? item.StdId : item.ExtId,
        item.Data,
        item.DLC,
        item.Bus
    );
#else
    CAN_rx_dispatcher(
        item.message.id,
        item.message.data,
        item.message.length,
        item.bus
    );
#endif
}

#if defined(STM32F407xx)
void PHAL_CAN_rxCallback(CanMsgTypeDef_t *message) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xQueueSendFromISR(can_rx_queue, message, &xHigherPriorityTaskWoken) != pdPASS) {
        can_stats.rx_overflow++;
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#else
void PHAL_CAN_rxCallback(
    PHAL_CAN_Handle_t *handle,
    const PHAL_CAN_Message_t *message
) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    const can_rx_item_t item = {.bus = handle, .message = *message};
    if (xQueueSendFromISR(can_rx_queue, &item, &xHigherPriorityTaskWoken) != pdPASS) {
        can_stats.rx_overflow++;
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif
