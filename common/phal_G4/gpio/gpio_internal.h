#ifndef PHAL_G4_GPIO_INTERNAL_H
#define PHAL_G4_GPIO_INTERNAL_H

#include "common/phal_G4/gpio/gpio.h"

bool PHAL_GPIO_internalConfigure(const GPIOInitConfig_t *config, uint8_t config_len);
bool PHAL_GPIO_internalRead(GPIO_TypeDef *port, uint8_t pin, bool *value);
bool PHAL_GPIO_internalWrite(GPIO_TypeDef *port, uint8_t pin, bool value);
bool PHAL_GPIO_internalToggle(GPIO_TypeDef *port, uint8_t pin);

#endif
