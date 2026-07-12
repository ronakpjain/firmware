#include "g4_testing.h"
#if (G4_TESTING_CHOSEN == TEST_FDCAN)

#include <string.h>

#include "common/freertos/freertos.h"
#include "common/phal/can.h"
#include "common/phal/gpio.h"
#include "common/phal/rcc.h"
#include "common/utils/countof.h"
#include "main.h"

GPIOInitConfig_t gpio_config[] = {
    GPIO_INIT_FDCAN2RX_PB12,
    GPIO_INIT_FDCAN2TX_PB13,
    GPIO_INIT_FDCAN3RX_PA8,
    GPIO_INIT_FDCAN3TX_PB4,
};

#define TargetCoreClockrateHz 16000000U
PHAL_RCC_Config_t clock_config = {
    .clock_source = CLOCK_SOURCE_HSI, .use_pll = false,
    .system_clock_target_hz = TargetCoreClockrateHz,
    .ahb_clock_target_hz = TargetCoreClockrateHz,
    .apb1_clock_target_hz = TargetCoreClockrateHz,
    .apb2_clock_target_hz = TargetCoreClockrateHz,
};

static void can_tx_100hz(void);
static void can_rx_1khz(void);
void HardFault_Handler(void);
DEFINE_TASK(can_tx_100hz, 10, osPriorityHigh, 256);
DEFINE_TASK(can_rx_1khz, 1, osPriorityHigh, 256);
DEFINE_QUEUE(q_can_rx, PHAL_CAN_Message_t, 256);

int main() {
    osKernelInitialize();
    if (!PHAL_RCC_configure(&clock_config)
        || !PHAL_initGPIO(gpio_config, countof(gpio_config))
        || !PHAL_CAN_init(&PHAL_CAN2, &(PHAL_CAN_Config_t){.bit_rate = 500000U, .loopback = false})
        || !PHAL_CAN_init(&PHAL_CAN3, &(PHAL_CAN_Config_t){.bit_rate = 500000U, .loopback = false})) {
        HardFault_Handler();
    }
    const uint32_t sids[] = {0x300U, 0x301U};
    const uint32_t xids[] = {0x1ABCDE1U, 0x1ABCDE2U, 0x1ABCDE3U};
    const PHAL_CAN_FilterConfig_t filters = {
        .standard_ids = sids, .standard_id_count = 2,
        .extended_ids = xids, .extended_id_count = 3,
    };
    if (!PHAL_CAN_setFilters(&PHAL_CAN2, &filters)
        || !PHAL_CAN_setFilters(&PHAL_CAN3, &filters)) {
        HardFault_Handler();
    }
    START_TASK(can_tx_100hz);
    START_TASK(can_rx_1khz);
    INIT_QUEUE(q_can_rx, PHAL_CAN_Message_t, 256);
    NVIC_SetPriority(FDCAN2_IT0_IRQn, 6);
    NVIC_SetPriority(FDCAN3_IT0_IRQn, 7);
    NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
    NVIC_EnableIRQ(FDCAN3_IT0_IRQn);
    osKernelStart();
    return 0;
}

void PHAL_CAN_rxCallback(PHAL_CAN_Handle_t *handle, const PHAL_CAN_Message_t *message) {
    (void)handle;
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        BaseType_t higher = 0;
        xQueueSendFromISR(q_can_rx, message, &higher);
        portYIELD_FROM_ISR(higher);
    }
}

static void PHAL_CAN_testExtended(void) {
    const PHAL_CAN_Message_t message = {
        .id = 0x1ABCDE1U, .extended = true, .length = 8,
        .data = {'E', 'X', 'T', 'I', 'D', '_', 'T', 'X'},
    };
    (void)PHAL_CAN_send(&PHAL_CAN2, &message);
}

static void can_tx_100hz(void) { PHAL_CAN_testExtended(); }
volatile PHAL_CAN_Message_t rx_frame_0;
static void can_rx_1khz(void) {
    PHAL_CAN_Message_t frame;
    while (xQueueReceive(q_can_rx, &frame, (TickType_t)0) == pdTRUE) {
        rx_frame_0 = frame;
    }
}

void HardFault_Handler(void) { while (1) { __asm__("nop"); } }

#endif // G4_TESTING_CHOSEN == TEST_FDCAN
