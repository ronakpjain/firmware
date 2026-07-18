#ifndef BOARD_PHAL_FAKE_H
#define BOARD_PHAL_FAKE_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g474xx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GPIO_TypeDef *bank;
    uint8_t pin;
    bool value;
} test_gpio_write_t;

void test_board_phal_reset(void);
uint32_t test_board_gpio_write_count(void);
test_gpio_write_t test_board_gpio_write_at(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif // BOARD_PHAL_FAKE_H
