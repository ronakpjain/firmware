#include "g4_testing.h"
#include "stm32g474xx.h"
#if (G4_TESTING_CHOSEN == TEST_SPI)

#include "common/phal/gpio.h"
#include "common/phal/rcc.h"
#include "common/phal/spi.h"
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
    GPIO_INIT_SPI1SCK_PA5,
    GPIO_INIT_SPI1MOSI_PA7,
    GPIO_INIT_SPI1MISO_PA6,
    GPIO_INIT_OUTPUT(GPIOA, 4, GPIO_OUTPUT_ULTRA_SPEED),
    GPIO_INIT_SPI2SCK_RET_PB13,
    GPIO_INIT_SPI2MOSI_RET_PB15,
    GPIO_INIT_SPI2MISO_RET_PB14,
    GPIO_INIT_SPI2NSS_RET_PB12,
};

#define XFER_LEN 8
static uint8_t master_tx[XFER_LEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
static uint8_t master_rx[XFER_LEN];
static uint8_t slave_tx[XFER_LEN] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x12, 0x34};
static uint8_t slave_rx[XFER_LEN];

static PHAL_SPI_Handle_t spi1;
static PHAL_SPI_Handle_t spi2;
static const PHAL_SPI_Config_t spi1_config = {
    .instance = SPI1, .mode = PHAL_SPI_MODE_MASTER, .data_rate_hz = 1000000U,
    .software_chip_select = true, .chip_select_port = GPIOA, .chip_select_pin = 4,
};
static const PHAL_SPI_Config_t spi2_config = {
    .instance = SPI2, .mode = PHAL_SPI_MODE_SLAVE, .data_rate_hz = 1000000U,
    .software_chip_select = false, .chip_select_port = GPIOB, .chip_select_pin = 12,
};

int main() {
    if (!PHAL_RCC_configure(&clock_config))
        HardFault_Handler();
    if (!PHAL_GPIO_init(gpio_config, countof(gpio_config)))
        HardFault_Handler();
    if (!PHAL_SPI_init(&spi1, &spi1_config) || !PHAL_SPI_init(&spi2, &spi2_config))
        HardFault_Handler();

    if (!PHAL_SPI_transfer(&spi2, slave_tx, slave_rx, XFER_LEN)
        || !PHAL_SPI_transfer(&spi1, master_tx, master_rx, XFER_LEN))
        HardFault_Handler();
    while (PHAL_SPI_busy(&spi1) || PHAL_SPI_busy(&spi2)) {
    }

    if (!PHAL_SPI_transfer_noDMA(&spi1, master_tx, master_rx, XFER_LEN, 1000000U))
        HardFault_Handler();
    return 0;
}

void HardFault_Handler() {
    while (1) {
        __asm__("nop");
    }
}

#endif // G4_TESTING_CHOSEN == TEST_SPI
