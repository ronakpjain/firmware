#include "g4_testing.h"
#if (G4_TESTING_CHOSEN == TEST_BLINKY)

#include <stdint.h>

#include "common/freertos/freertos.h"
#include "common/phal/gpio.h"
#include "common/phal/rcc.h"
#include "common/utils/countof.h"
#include "main.h"

GPIOInitConfig_t gpio_config[] = {
    GPIO_INIT_OUTPUT(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_OUTPUT_LOW_SPEED),
    GPIO_INIT_OUTPUT(LED_RED_PORT, LED_RED_PIN, GPIO_OUTPUT_LOW_SPEED),
    GPIO_INIT_OUTPUT(LED_BLUE_PORT, LED_BLUE_PIN, GPIO_OUTPUT_LOW_SPEED),
    GPIO_INIT_OUTPUT(LED_ORANGE_PORT, LED_ORANGE_PIN, GPIO_OUTPUT_LOW_SPEED),
};

static constexpr uint32_t TargetCoreClockrateHz = 16'000'000;
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

static void ledblink1(void);
static void ledblink2(void);
static void ledblink3(void);
static void ledblink4(void);

DEFINE_TASK(ledblink1, 250, osPriorityNormal, 64);
DEFINE_TASK(ledblink2, 300, osPriorityNormal, 64);
DEFINE_TASK(ledblink3, 500, osPriorityNormal, 64);
DEFINE_TASK(ledblink4, 1000, osPriorityNormal, 64);

int main() {
    if (!PHAL_RCC_configure(&clock_config)) {
        HardFault_Handler();
    }

    if (!PHAL_initGPIO(gpio_config, countof(gpio_config))) {
        HardFault_Handler();
    }

    PHAL_GPIO_write(LED_GREEN_PORT, LED_GREEN_PIN, 1);
    PHAL_GPIO_write(LED_RED_PORT, LED_RED_PIN, 1);
    PHAL_GPIO_write(LED_BLUE_PORT, LED_BLUE_PIN, 1);
    PHAL_GPIO_write(LED_ORANGE_PORT, LED_ORANGE_PIN, 1);

    osKernelInitialize();

    // Create threads
    START_TASK(ledblink1);
    START_TASK(ledblink2);
    START_TASK(ledblink3);
    START_TASK(ledblink4);

    osKernelStart(); // Go!

    return 0;
}

static void ledblink1(void) {
    PHAL_GPIO_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
}

static void ledblink2(void) {
    PHAL_GPIO_toggle(LED_RED_PORT, LED_RED_PIN);
}

static void ledblink3(void) {
    PHAL_GPIO_toggle(LED_BLUE_PORT, LED_BLUE_PIN);
}

static void ledblink4(void) {
    PHAL_GPIO_toggle(LED_ORANGE_PORT, LED_ORANGE_PIN);
}

void HardFault_Handler() {
    while (1) {
        __asm__("nop");
    }
}

#endif // G4_TESTING_CHOSEN == TEST_BLINKY
