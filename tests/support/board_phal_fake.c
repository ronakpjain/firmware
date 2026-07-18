#include "board_phal_fake.h"

#include <string.h>

#define MAX_GPIO_WRITES 128

static test_gpio_write_t gpio_writes[MAX_GPIO_WRITES];
static uint32_t gpio_write_count;

void test_board_phal_reset(void) {
    memset(gpio_writes, 0, sizeof(gpio_writes));
    gpio_write_count = 0;
}

uint32_t test_board_gpio_write_count(void) {
    return gpio_write_count;
}

test_gpio_write_t test_board_gpio_write_at(uint32_t index) {
    if (index >= gpio_write_count || index >= MAX_GPIO_WRITES) {
        return (test_gpio_write_t){0};
    }
    return gpio_writes[index];
}

bool PHAL_readGPIO(GPIO_TypeDef *bank, uint8_t pin) {
    return ((bank->IDR >> pin) & 0x1U) != 0;
}

void PHAL_writeGPIO(GPIO_TypeDef *bank, uint8_t pin, bool value) {
    if (gpio_write_count < MAX_GPIO_WRITES) {
        gpio_writes[gpio_write_count] = (test_gpio_write_t){
            .bank = bank,
            .pin = pin,
            .value = value,
        };
    }
    gpio_write_count++;
}
