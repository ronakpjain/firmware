#include "g4_testing.h"
#include "stm32g474xx.h"
#if (G4_TESTING_CHOSEN == TEST_USART)

#include <string.h>

#include "common/freertos/freertos.h"
#include "common/phal/gpio.h"
#include "common/phal/rcc.h"
#include "common/phal/usart.h"
#include "common/utils/countof.h"

void HardFault_Handler();

#define TargetCoreClockrateHz 16000000U
PHAL_RCC_Config_t clock_config = {
    .clock_source           = CLOCK_SOURCE_HSI,
    .use_pll                = false,
    .system_clock_target_hz = TargetCoreClockrateHz,
    .ahb_clock_target_hz    = TargetCoreClockrateHz,
    .apb1_clock_target_hz   = TargetCoreClockrateHz,
    .apb2_clock_target_hz   = TargetCoreClockrateHz,
};

GPIOInitConfig_t gpio_config[] = {
    GPIO_INIT_USART2RX_PA3,
    GPIO_INIT_USART2TX_PA2,
};

#define RX_BUFFER_SIZE 12
#define TX_BUFFER_SIZE 12
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint8_t tx_buffer[TX_BUFFER_SIZE];

PHAL_USART_Handle_t usart_handle;
static const PHAL_USART_Config_t usart_config = {
    .instance     = USART2,
    .baud_rate    = 115200U,
    .word_length  = 8,
    .parity       = PHAL_USART_PARITY_NONE,
    .stop_bits    = PHAL_USART_STOP_BITS_1,
    .hardware_rts = false,
    .hardware_cts = false,
};

int main() {
    osKernelInitialize();
    if (!PHAL_RCC_configure(&clock_config))
        HardFault_Handler();
    if (!PHAL_initGPIO(gpio_config, countof(gpio_config)))
        HardFault_Handler();
    if (!PHAL_USART_init(&usart_handle, &usart_config))
        HardFault_Handler();
    if (!PHAL_USART_startIdleReceive(&usart_handle, rx_buffer, RX_BUFFER_SIZE))
        HardFault_Handler();
    osKernelStart();
    return 0;
}

void PHAL_USART_receiveCompleteCallback(
    PHAL_USART_Handle_t *handle,
    bool success,
    size_t received_length
) {
    if (handle != &usart_handle) {
        return;
    }
    if (success && received_length <= sizeof(tx_buffer) && !PHAL_USART_txBusy(handle)) {
        memcpy(tx_buffer, rx_buffer, received_length);
        (void)PHAL_USART_transmit(handle, tx_buffer, received_length);
    }
    if (!PHAL_USART_startIdleReceive(handle, rx_buffer, RX_BUFFER_SIZE)) {
        HardFault_Handler();
    }
}

void HardFault_Handler() {
    while (1) {
        __asm__("nop");
    }
}

#endif // G4_TESTING_CHOSEN == TEST_USART
