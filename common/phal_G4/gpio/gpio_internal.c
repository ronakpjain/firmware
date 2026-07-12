#include "common/phal_G4/gpio/gpio_internal.h"

#include <stddef.h>

static bool supported_port(GPIO_TypeDef *port) {
    return port == GPIOA || port == GPIOB || port == GPIOC || port == GPIOD
        || port == GPIOE || port == GPIOF || port == GPIOG;
}

static void enable_port_clock(GPIO_TypeDef *port) {
    if (port == GPIOA) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    if (port == GPIOB) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    if (port == GPIOC) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    if (port == GPIOD) RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    if (port == GPIOE) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
    if (port == GPIOF) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOFEN;
    if (port == GPIOG) RCC->AHB2ENR |= RCC_AHB2ENR_GPIOGEN;
    (void)RCC->AHB2ENR;
}

static bool valid_config(const GPIOInitConfig_t *entry) {
    if (entry == NULL || !supported_port(entry->bank) || entry->pin >= 16U
        || entry->type > GPIO_TYPE_ANALOG) {
        return false;
    }

    switch (entry->type) {
        case GPIO_TYPE_INPUT:
        case GPIO_TYPE_ANALOG:
            return entry->config.pull <= GPIO_INPUT_PULL_DOWN;
        case GPIO_TYPE_OUTPUT:
            return entry->config.ospeed <= GPIO_OUTPUT_ULTRA_SPEED
                && entry->config.otype <= GPIO_OUTPUT_OPEN_DRAIN;
        case GPIO_TYPE_AF:
            return entry->config.af_num < 16U
                && entry->config.ospeed <= GPIO_OUTPUT_ULTRA_SPEED
                && entry->config.otype <= GPIO_OUTPUT_OPEN_DRAIN
                && entry->config.pull <= GPIO_INPUT_PULL_DOWN;
        default:
            return false;
    }
}

static void configure_pull(GPIO_TypeDef *port, uint8_t pin, GPIOInputPull_t pull) {
    const uint32_t shift = 2U * pin;
    port->PUPDR = (port->PUPDR & ~(GPIO_PUPDR_PUPD0_Msk << shift))
        | (((uint32_t)pull & GPIO_PUPDR_PUPD0_Msk) << shift);
}

static void configure_output_fields(
    GPIO_TypeDef *port,
    uint8_t pin,
    GPIOOutputSpeed_t speed,
    GPIOOutputPull_t type
) {
    const uint32_t shift = 2U * pin;
    port->OSPEEDR = (port->OSPEEDR & ~(GPIO_OSPEEDR_OSPEED0_Msk << shift))
        | (((uint32_t)speed & GPIO_OSPEEDR_OSPEED0_Msk) << shift);
    port->OTYPER = (port->OTYPER & ~(GPIO_OTYPER_OT0_Msk << pin))
        | (((uint32_t)type & GPIO_OTYPER_OT0_Msk) << pin);
}

static void configure_mode(GPIO_TypeDef *port, uint8_t pin, GPIOPinType_t type) {
    const uint32_t shift = 2U * pin;
    port->MODER = (port->MODER & ~(GPIO_MODER_MODE0_Msk << shift))
        | (((uint32_t)type & GPIO_MODER_MODE0_Msk) << shift);
}

bool PHAL_GPIO_internalConfigure(const GPIOInitConfig_t *config, uint8_t config_len) {
    if (config == NULL && config_len != 0U) {
        return false;
    }

    for (uint8_t i = 0U; i < config_len; ++i) {
        if (!valid_config(&config[i])) {
            return false;
        }
    }

    for (uint8_t i = 0U; i < config_len; ++i) {
        const GPIOInitConfig_t *entry = &config[i];
        GPIO_TypeDef *port = entry->bank;
        const uint8_t pin = entry->pin;
        enable_port_clock(port);

        switch (entry->type) {
            case GPIO_TYPE_OUTPUT:
                configure_output_fields(port, pin, entry->config.ospeed, entry->config.otype);
                configure_pull(port, pin, GPIO_INPUT_OPEN_DRAIN);
                configure_mode(port, pin, GPIO_TYPE_OUTPUT);
                break;
            case GPIO_TYPE_INPUT:
                configure_pull(port, pin, entry->config.pull);
                configure_mode(port, pin, GPIO_TYPE_INPUT);
                break;
            case GPIO_TYPE_AF: {
                configure_output_fields(port, pin, entry->config.ospeed, entry->config.otype);
                configure_pull(port, pin, entry->config.pull);
                const uint8_t afr_index = pin / 8U;
                const uint8_t afr_shift = (uint8_t)(4U * (pin % 8U));
                port->AFR[afr_index] = (port->AFR[afr_index]
                    & ~(GPIO_AFRL_AFSEL0_Msk << afr_shift))
                    | (((uint32_t)entry->config.af_num & GPIO_AFRL_AFSEL0_Msk) << afr_shift);
                configure_mode(port, pin, GPIO_TYPE_AF);
                break;
            }
            case GPIO_TYPE_ANALOG:
                configure_pull(port, pin, GPIO_INPUT_OPEN_DRAIN);
                configure_mode(port, pin, GPIO_TYPE_ANALOG);
                break;
            default:
                return false;
        }
    }
    return true;
}

bool PHAL_GPIO_internalRead(GPIO_TypeDef *port, uint8_t pin, bool *value) {
    if (!supported_port(port) || pin >= 16U || value == NULL) {
        return false;
    }
    *value = ((port->IDR >> pin) & 1U) != 0U;
    return true;
}

bool PHAL_GPIO_internalWrite(GPIO_TypeDef *port, uint8_t pin, bool value) {
    if (!supported_port(port) || pin >= 16U) {
        return false;
    }
    port->BSRR = value ? (1UL << pin) : (1UL << (pin + 16U));
    return true;
}

bool PHAL_GPIO_internalToggle(GPIO_TypeDef *port, uint8_t pin) {
    if (!supported_port(port) || pin >= 16U) {
        return false;
    }
    const bool current = ((port->ODR >> pin) & 1U) != 0U;
    port->BSRR = current ? (1UL << (pin + 16U)) : (1UL << pin);
    return true;
}
