#include "common/phal_G4/gpio/gpio_internal.h"

#include <stddef.h>

bool PHAL_initGPIO(const GPIOInitConfig_t config[], uint8_t config_len) {
    if (!PHAL_GPIO_internalValidateConfig(config, config_len)) {
        return false;
    }

    for (uint8_t i = 0U; i < config_len; ++i) {
        const GPIOInitConfig_t *entry = &config[i];
        PHAL_GPIO_internalEnablePortClock(entry->bank);

        switch (entry->type) {
            case GPIO_TYPE_OUTPUT:
                PHAL_GPIO_internalConfigureOutput(entry);
                break;
            case GPIO_TYPE_INPUT:
                PHAL_GPIO_internalConfigureInput(entry);
                break;
            case GPIO_TYPE_AF:
                PHAL_GPIO_internalConfigureAlternateFunction(entry);
                break;
            case GPIO_TYPE_ANALOG:
                PHAL_GPIO_internalConfigureAnalog(entry);
                break;
            default:
                return false;
        }
    }
    return true;
}

bool PHAL_GPIO_read(GPIO_TypeDef *port, uint8_t pin, bool *value) {
    if (value == NULL || !PHAL_GPIO_internalValidatePin(port, pin)) {
        return false;
    }
    PHAL_GPIO_internalRead(port, pin, value);
    return true;
}

bool PHAL_GPIO_write(GPIO_TypeDef *port, uint8_t pin, bool value) {
    if (!PHAL_GPIO_internalValidatePin(port, pin)) {
        return false;
    }
    PHAL_GPIO_internalWrite(port, pin, value);
    return true;
}

bool PHAL_GPIO_toggle(GPIO_TypeDef *port, uint8_t pin) {
    if (!PHAL_GPIO_internalValidatePin(port, pin)) {
        return false;
    }
    PHAL_GPIO_internalToggle(port, pin);
    return true;
}

bool PHAL_readGPIO(GPIO_TypeDef *port, uint8_t pin) {
    bool value = false;
    return PHAL_GPIO_read(port, pin, &value) && value;
}

void PHAL_writeGPIO(GPIO_TypeDef *port, uint8_t pin, bool value) {
    (void)PHAL_GPIO_write(port, pin, value);
}

void PHAL_toggleGPIO(GPIO_TypeDef *port, uint8_t pin) {
    (void)PHAL_GPIO_toggle(port, pin);
}
