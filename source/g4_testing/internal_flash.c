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

static const uint8_t g_flash_patterns[][13] = {
    {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x0F, 0xED, 0xCB, 0xA9, 0x87},
    {0xA5, 0x5A, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB},
    {0xDE, 0xAD, 0xBE, 0xEF, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90},
    {0xF0, 0x0D, 0xCA, 0xFE, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x5A},
};
uint8_t g_flash_readback[sizeof(g_flash_patterns[0])] = {0};
static volatile uint32_t g_flash_breakpoint = 0U;

#define DEFINE_FLASH_BREAKPOINT(site) \
    __attribute__((noinline)) static void flash_breakpoint_##site(void) { \
        g_flash_breakpoint = site; \
        __asm__ volatile("nop"); \
    }

DEFINE_FLASH_BREAKPOINT(1)
DEFINE_FLASH_BREAKPOINT(2)
DEFINE_FLASH_BREAKPOINT(3)
DEFINE_FLASH_BREAKPOINT(4)

int main() {
    if (PHAL_configureClockRates(&clock_config)) {
        HardFault_Handler();
    }
    if (!PHAL_initGPIO(gpio_config, countof(gpio_config))) {
        HardFault_Handler();
    }

    bool match = true;

    for (uint32_t cycle = 0U; cycle < countof(g_flash_patterns); cycle++) {
        const uint8_t *pattern = g_flash_patterns[cycle];
        bool cycle_match       = PHAL_FLASH_erase(FLASH_TEST_PAGE, sizeof(g_flash_patterns[0]));
        cycle_match = PHAL_FLASH_write(FLASH_TEST_PAGE, pattern, sizeof(g_flash_patterns[0]))
            && cycle_match;

        switch (cycle) {
            case 0U:
                flash_breakpoint_1();
                break;
            case 1U:
                flash_breakpoint_2();
                break;
            case 2U:
                flash_breakpoint_3();
                break;
            default:
                flash_breakpoint_4();
                break;
        }

        cycle_match = PHAL_FLASH_read(FLASH_TEST_PAGE, g_flash_readback, sizeof(g_flash_readback))
            && cycle_match;
        for (uint32_t i = 0U; cycle_match && i < sizeof(g_flash_patterns[0]); i++) {
            cycle_match = g_flash_readback[i] == pattern[i];
        }
        match = cycle_match && match;
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
