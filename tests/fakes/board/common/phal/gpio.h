#ifndef TEST_FAKE_BOARD_GPIO_H
#define TEST_FAKE_BOARD_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g474xx.h"

bool PHAL_readGPIO(GPIO_TypeDef *bank, uint8_t pin);
void PHAL_writeGPIO(GPIO_TypeDef *bank, uint8_t pin, bool value);

#endif // TEST_FAKE_BOARD_GPIO_H
