#include "g4_testing.h"
#if (G4_TESTING_CHOSEN == TEST_FLASH)

#include <stdint.h>

#include "common/phal/flash.h"
#include "common/phal/gpio.h"
#include "common/phal/rcc.h"
#include "common/utils/countof.h"
#include "main.h"

void HardFault_Handler();

static constexpr uint32_t kTargetCoreClockrateHz = 16'000'000;
ClockRateConfig_t clock_config = {
    .clock_source           = CLOCK_SOURCE_HSI,
    .system_clock_target_hz = kTargetCoreClockrateHz,
    .ahb_clock_target_hz    = (kTargetCoreClockrateHz / 1),
    .apb1_clock_target_hz   = (kTargetCoreClockrateHz / (1)),
    .apb2_clock_target_hz   = (kTargetCoreClockrateHz / (1)),
};

#define FLASH_TEST_PAGE (FLASH_BASE + (508U * 1024U))

GPIOInitConfig_t gpio_config[] = {
    GPIO_INIT_OUTPUT(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_OUTPUT_LOW_SPEED),
    GPIO_INIT_OUTPUT(LED_RED_PORT, LED_RED_PIN, GPIO_OUTPUT_LOW_SPEED),
};

static const uint8_t g_flash_pattern[] = {
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
    0x0F, 0xED, 0xCB, 0xA9, 0x87,
};
uint8_t g_flash_readback[16] = {0};

int main() {
    if (PHAL_configureClockRates(&clock_config)) {
        HardFault_Handler();
    }
    if (!PHAL_initGPIO(gpio_config, countof(gpio_config))) {
        HardFault_Handler();
    }

    bool match = true;

    if (!PHAL_FLASH_erase(FLASH_TEST_PAGE, sizeof(g_flash_pattern))) {
        match = false;
    }
    if (!PHAL_FLASH_write(FLASH_TEST_PAGE, g_flash_pattern, sizeof(g_flash_pattern))) {
        match = false;
    }
    if (!PHAL_FLASH_read(FLASH_TEST_PAGE, g_flash_readback, sizeof(g_flash_readback))) {
        match = false;
    }

    if (match) {
        for (uint32_t i = 0U; i < sizeof(g_flash_pattern); i++) {
            if (g_flash_readback[i] != g_flash_pattern[i]) {
                match = false;
                break;
            }
        }
    }

    PHAL_writeGPIO(LED_GREEN_PORT, LED_GREEN_PIN, match ? 1 : 0);
    PHAL_writeGPIO(LED_RED_PORT, LED_RED_PIN, match ? 0 : 1);

    return 0;
}

void HardFault_Handler() {
    while (1) {
        __asm__("nop");
    }
}

#endif // G4_TESTING_CHOSEN == TEST_FLASH
