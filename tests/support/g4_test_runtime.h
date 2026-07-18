#ifndef G4_TEST_RUNTIME_H
#define G4_TEST_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g474xx.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_runtime_reset(void);

bool test_g4_read_gpio(GPIO_TypeDef *bank, uint8_t pin);
void test_g4_write_gpio(GPIO_TypeDef *bank, uint8_t pin, bool value);
void test_g4_toggle_gpio(GPIO_TypeDef *bank, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif // G4_TEST_RUNTIME_H
