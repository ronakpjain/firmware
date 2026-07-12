#include "g4_testing.h"
#if (G4_TESTING_CHOSEN == TEST_CANPILER)

#include <string.h>

#include "can_library/generated/G4_TESTING.h"
#include "common/phal/adc.h"
#include "common/phal/can.h"
#include "common/phal/dma.h"
#include "common/phal/gpio.h"
#include "common/phal/rcc.h"
#include "common/freertos/freertos.h"
#include "main.h"
#include "can_library/faults_common.h"
#include "common/utils/countof.h"

GPIOInitConfig_t gpio_config[] = {
    GPIO_INIT_FDCAN2RX_PB12,
    GPIO_INIT_FDCAN2TX_PB13,

};

#define TargetCoreClockrateHz 16'000'000
PHAL_RCC_Config_t clock_config = {
    .clock_source              = CLOCK_SOURCE_HSI,
    .use_pll                   = false,
    .vco_output_rate_target_hz = 16'000'000,
    .system_clock_target_hz    = TargetCoreClockrateHz,
    .ahb_clock_target_hz       = (TargetCoreClockrateHz / 1),
    .apb1_clock_target_hz      = (TargetCoreClockrateHz / (1)),
    .apb2_clock_target_hz      = (TargetCoreClockrateHz / (1)),
};


void HardFault_Handler();

// void send_periodic() {
//     CAN_SEND_ccan_test(0x3);
// }

void send_periodic() {
    CAN_SEND_IZZE_IMU_config(0U, 0U, 0U, 0U, 0U, 0U);
}

DEFINE_TASK(CAN_rx_update, 0, osPriorityHigh, STACK_2048);
DEFINE_TASK(CAN_tx_update, 2, osPriorityNormal, STACK_2048);
DEFINE_TASK(send_periodic, 10, osPriorityNormal, 1024);

int main() {
    if (!PHAL_RCC_configure(&clock_config)) {
        HardFault_Handler();
    }

    if (!PHAL_initGPIO(gpio_config, countof(gpio_config))) {
        HardFault_Handler();
    }

    if (!PHAL_CAN_init(&PHAL_CAN2, &(PHAL_CAN_Config_t){.bit_rate = GCAN_BAUD_RATE, .loopback = false})) {
        HardFault_Handler();
    }

    if (!CAN_init()) {
        HardFault_Handler();
    }

    // NVIC
    NVIC_SetPriority(FDCAN2_IT0_IRQn, 6);
    NVIC_EnableIRQ(FDCAN2_IT0_IRQn);

    osKernelInitialize();

    START_TASK(CAN_rx_update);
    START_TASK(CAN_tx_update);
    START_TASK(send_periodic);

    osKernelStart();

    return 0;
}

void HardFault_Handler() {
    while (1) {
        __asm__("nop");
    }
}

#endif // G4_TESTING_CHOSEN == TEST_CANPILER
