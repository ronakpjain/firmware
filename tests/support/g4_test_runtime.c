#include "g4_test_runtime.h"

#include <string.h>

#include "common/phal/gpio.h"

GPIO_TypeDef test_gpioa;
GPIO_TypeDef test_gpiob;
GPIO_TypeDef test_gpioc;
GPIO_TypeDef test_gpiod;
GPIO_TypeDef test_gpioe;
GPIO_TypeDef test_gpiof;
GPIO_TypeDef test_gpiog;
RCC_TypeDef test_rcc;

void test_runtime_reset(void) {
    memset(&test_gpioa, 0, sizeof(test_gpioa));
    memset(&test_gpiob, 0, sizeof(test_gpiob));
    memset(&test_gpioc, 0, sizeof(test_gpioc));
    memset(&test_gpiod, 0, sizeof(test_gpiod));
    memset(&test_gpioe, 0, sizeof(test_gpioe));
    memset(&test_gpiof, 0, sizeof(test_gpiof));
    memset(&test_gpiog, 0, sizeof(test_gpiog));
    memset(&test_rcc, 0, sizeof(test_rcc));
}

bool test_g4_read_gpio(GPIO_TypeDef *bank, uint8_t pin) {
    return PHAL_readGPIO(bank, pin);
}

void test_g4_write_gpio(GPIO_TypeDef *bank, uint8_t pin, bool value) {
    PHAL_writeGPIO(bank, pin, value);
}

void test_g4_toggle_gpio(GPIO_TypeDef *bank, uint8_t pin) {
    PHAL_toggleGPIO(bank, pin);
}
