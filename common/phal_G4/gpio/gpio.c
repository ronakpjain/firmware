#include "common/phal_G4/gpio/gpio_internal.h"

#include <stddef.h>

bool PHAL_GPIO_init(const GPIOInitConfig_t config[], uint8_t config_len) {
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
