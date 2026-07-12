#ifndef PHAL_G4_GPIO_INTERNAL_H
#define PHAL_G4_GPIO_INTERNAL_H

#include "common/phal_G4/gpio/gpio.h"

bool PHAL_GPIO_internalValidateConfig(const GPIOInitConfig_t *config, uint8_t config_len);
bool PHAL_GPIO_internalValidatePin(GPIO_TypeDef *port, uint8_t pin);
void PHAL_GPIO_internalEnablePortClock(GPIO_TypeDef *port);
void PHAL_GPIO_internalConfigureOutput(const GPIOInitConfig_t *entry);
void PHAL_GPIO_internalConfigureInput(const GPIOInitConfig_t *entry);
void PHAL_GPIO_internalConfigureAlternateFunction(const GPIOInitConfig_t *entry);
void PHAL_GPIO_internalConfigureAnalog(const GPIOInitConfig_t *entry);

#endif
