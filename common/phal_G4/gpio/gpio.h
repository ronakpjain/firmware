/**
 * @file gpio.h
 * @author Adam Busch (busch8@purdue.edu)
 * @brief GPIO driver for STM32G474 devices.
 * @version 0.1
 * @date 2021-09-20
 *
 *
 * @copyright Copyright (c) 2021
 *
 *
 */
#ifndef __PHAL_G4_GPIO_H__
#define __PHAL_G4_GPIO_H__

#include <stddef.h>

#include "common/phal_G4/phal_G4.h"

/**
 * @brief Configuration type for GPIO Pin
 */
typedef enum {
    GPIO_TYPE_INPUT  = 0b00, /**< Pin input mode. */
    GPIO_TYPE_OUTPUT = 0b01, /**< Pin output mode. */
    GPIO_TYPE_AF     = 0b10, /**< Pin alternate-function mode. */
    GPIO_TYPE_ANALOG = 0b11, /**< Pin analog mode. */
} GPIOPinType_t;

/**
 * @brief Slew rate control for output pins
 */
typedef enum {
    GPIO_OUTPUT_LOW_SPEED   = 0b00, /**< Low slew rate. */
    GPIO_OUTPUT_MED_SPEED   = 0b01, /**< Medium slew rate. */
    GPIO_OUTPUT_HIGH_SPEED  = 0b10, /**< High slew rate. */
    GPIO_OUTPUT_ULTRA_SPEED = 0b11, /**< Very-high slew rate. */
} GPIOOutputSpeed_t;

/**
 * @brief Output drive mode selection
 */
typedef enum {
    GPIO_OUTPUT_PUSH_PULL  = 0b0, /**< Drive the output actively high and low. */
    GPIO_OUTPUT_OPEN_DRAIN = 0b1, /**< Drive low or leave the output high impedance. */
} GPIOOutputPull_t;

/**
 * @brief Enable internal pullup/down resistors
 */
typedef enum {
    GPIO_INPUT_OPEN_DRAIN = 0b00, /**< Disable internal pull resistors. */
    GPIO_INPUT_PULL_UP    = 0b01, /**< Enable the weak internal pull-up. */
    GPIO_INPUT_PULL_DOWN  = 0b10, /**< Enable the weak internal pull-down. */
} GPIOInputPull_t;

/**
 * @brief Configuration entry for GPIO initialization.
 */
typedef struct {
    GPIO_TypeDef *bank; /**< GPIO register bank. */
    uint8_t pin;        /**< Pin number from 0 through 15. */
    GPIOPinType_t type; /**< Pin operating mode. */

    /** Mode-specific pin settings. */
    struct {
        GPIOInputPull_t pull;     /**< Input and alternate-function pull selection. */
        GPIOOutputSpeed_t ospeed; /**< Output and alternate-function slew rate. */
        GPIOOutputPull_t otype;   /**< Output and alternate-function driver type. */
        uint8_t af_num;           /**< Alternate-function number from 0 through 15. */
    } config;
} GPIOInitConfig_t;

/**
 * @brief Create GPIO Init struct to intilize a GPIO pin for input
 *
 * @param gpio_bank GPIO_TypeDef* reference to the GPIO bank for the pin
 * @param pin_num Pin number from GPIO bank to configure
 * @param input_pull_sel Input pullup/pulldown/high-z selection
 */
#define GPIO_INIT_INPUT(gpio_bank, pin_num, input_pull_sel) \
    { \
        .bank = gpio_bank, .pin = pin_num, .type = GPIO_TYPE_INPUT, .config = { \
            .pull = input_pull_sel \
        } \
    }

/**
 * @brief Create GPIO Init struct to intilize a GPIO pin for output
 *
 * @param gpio_bank GPIO_TypeDef* reference to the GPIO bank for the pin
 * @param pin_num Pin number from GPIO bank to configure
 * @param ospeed_sel Pin output speed selection
 */
#define GPIO_INIT_OUTPUT(gpio_bank, pin_num, ospeed_sel) \
    { \
        .bank = gpio_bank, .pin = pin_num, .type = GPIO_TYPE_OUTPUT, .config = { \
            .ospeed = ospeed_sel, \
            .otype  = GPIO_OUTPUT_PUSH_PULL \
        } \
    }

#define GPIO_INIT_OUTPUT_OPEN_DRAIN(gpio_bank, pin_num, ospeed_sel) \
    { \
        .bank = gpio_bank, .pin = pin_num, .type = GPIO_TYPE_OUTPUT, .config = { \
            .ospeed = ospeed_sel, \
            .otype  = GPIO_OUTPUT_OPEN_DRAIN \
        } \
    }
/**
 * @brief Create GPIO Init struct to intilize a GPIO pin for analog
 *
 * @param gpio_bank GPIO_TypeDef* reference to the GPIO bank for the pin
 * @param pin_num Pin number from GPIO bank to configure
 */
#define GPIO_INIT_ANALOG(gpio_bank, pin_num) \
    {.bank = gpio_bank, .pin = pin_num, .type = GPIO_TYPE_ANALOG}

/**
 * @brief Create GPIO Init struct to intilize a GPIO pin for alternate function
 *
 * @param gpio_bank GPIO_TypeDef* reference to the GPIO bank for the pin
 * @param pin_num Pin number from GPIO bank to configure
 * @param alt_func_num Alternate function selection
 * @param ospeed_sel Pin output speed selection
 * @param otype_sel Pin output type selection
 * @param input_pull_sel Input pullup/pulldown/high-z selection
 */
#define GPIO_INIT_AF(gpio_bank, pin_num, alt_func_num, ospeed_sel, otype_sel, input_pull_sel) \
    { \
        .bank = gpio_bank, .pin = pin_num, .type = GPIO_TYPE_AF, .config = { \
            .af_num = alt_func_num, \
            .ospeed = ospeed_sel, \
            .otype  = otype_sel, \
            .pull   = input_pull_sel \
        } \
    }

/*
    Useful defines for GPIO Init struct with commonly used peripheral/pin mappings.
    If you find yourself adding the same pin mappings to multiple devices, add a macro below
    to cut down on duplication.
*/

/**
 * @brief Initialize GPIO pins from a list of semantic configurations.
 *
 * Enables each referenced GPIO port clock and programs the pin mode, pull,
 * output type, speed, and alternate-function fields selected by the entry.
 *
 * @param config A list of GPIOs to config
 * @param config_len Number of GPIOs in the config list
 * @return true All GPIOs were a valid configuration format
 * @return false Some of the GPIOs had an invalid configuration format
 */
bool PHAL_GPIO_init(const GPIOInitConfig_t config[], uint8_t config_len);

/**
 * @brief Read one GPIO input value.
 * @param port GPIO port register block.
 * @param pin Pin number in the range 0..15.
 * @param value Destination for the sampled IDR value.
 * @return true The arguments were valid.
 * @return false A pointer was NULL or the pin was outside 0..15.
 */
static inline bool PHAL_GPIO_read(GPIO_TypeDef *port, uint8_t pin, bool *value) {
    if (port == NULL || value == NULL || pin > 15U) {
        return false;
    }
    *value = ((port->IDR >> pin) & 1U) != 0U;
    return true;
}

/**
 * @brief Atomically set or reset one GPIO output.
 * @param port GPIO port register block.
 * @param pin Pin number in the range 0..15.
 * @param value true sets the pin; false resets it.
 * @return true The arguments were valid.
 * @return false The port was NULL or the pin was outside 0..15.
 */
static inline bool PHAL_GPIO_write(GPIO_TypeDef *port, uint8_t pin, bool value) {
    if (port == NULL || pin > 15U) {
        return false;
    }
    port->BSRR = 1UL << (value ? pin : pin + 16U);
    return true;
}

/**
 * @brief Atomically toggle one GPIO output latch.
 * @param port GPIO port register block.
 * @param pin Pin number in the range 0..15.
 * @return true The arguments were valid.
 * @return false The port was NULL or the pin was outside 0..15.
 * @note The ODR latch, rather than the external IDR level, determines the next value.
 */
static inline bool PHAL_GPIO_toggle(GPIO_TypeDef *port, uint8_t pin) {
    if (port == NULL || pin > 15U) {
        return false;
    }
    return PHAL_GPIO_write(port, pin, ((port->ODR >> pin) & 1U) == 0U);
}

/** @deprecated Shared F4/G4 compatibility API; prefer PHAL_GPIO_read(). */
static inline bool PHAL_readGPIO(GPIO_TypeDef *port, uint8_t pin) {
    bool value = false;
    return PHAL_GPIO_read(port, pin, &value) && value;
}

/** @deprecated Shared F4/G4 compatibility API; prefer PHAL_GPIO_write(). */
static inline void PHAL_writeGPIO(GPIO_TypeDef *port, uint8_t pin, bool value) {
    (void)PHAL_GPIO_write(port, pin, value);
}

/** @deprecated Shared F4/G4 compatibility API; prefer PHAL_GPIO_toggle(). */
static inline void PHAL_toggleGPIO(GPIO_TypeDef *port, uint8_t pin) {
    (void)PHAL_GPIO_toggle(port, pin);
}

#define GPIO_INIT_USART3TX_PC10 \
    GPIO_INIT_AF(GPIOC, \
                 10, \
                 7, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_USART3RX_PC11 \
    GPIO_INIT_AF(GPIOC, \
                 11, \
                 7, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_USART3TX_PB10 \
    GPIO_INIT_AF(GPIOB, \
                 10, \
                 7, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_USART3RX_PB11 \
    GPIO_INIT_AF(GPIOB, \
                 11, \
                 7, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_USART2TX_PA2 \
    GPIO_INIT_AF(GPIOA, 2, 7, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_USART2RX_PA3 \
    GPIO_INIT_AF(GPIOA, \
                 3, \
                 7, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_USART1TX_PA9 \
    GPIO_INIT_AF(GPIOA, 9, 7, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_USART1RX_PA10 \
    GPIO_INIT_AF(GPIOA, \
                 10, \
                 7, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_USART2TX_PD5 \
    GPIO_INIT_AF(GPIOD, 5, 7, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_USART2RX_PD6 \
    GPIO_INIT_AF(GPIOD, \
                 6, \
                 7, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_UART4TX_PC10 \
    GPIO_INIT_AF(GPIOC, \
                 10, \
                 8, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_UART4RX_PC11 \
    GPIO_INIT_AF(GPIOC, \
                 11, \
                 8, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_LPUART1TX_PC0 \
    GPIO_INIT_AF(GPIOC, 0, 8, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)
#define GPIO_INIT_LPUART1RX_PC1 \
    GPIO_INIT_AF(GPIOC, \
                 1, \
                 8, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_OPEN_DRAIN, \
                 GPIO_INPUT_OPEN_DRAIN)

/* * SPI1 Pins (Standard for both RET and CET)
 * PA5=SCK, PA6=MISO, PA7=MOSI, PA4 or PA15=NSS (AF5) 
 */
#define GPIO_INIT_SPI1SCK_PA5 \
    GPIO_INIT_AF(GPIOA, 5, 5, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_SPI1MISO_PA6 \
    GPIO_INIT_AF(GPIOA, 6, 5, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_PULL_UP)

#define GPIO_INIT_SPI1MOSI_PA7 \
    GPIO_INIT_AF(GPIOA, 7, 5, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_SPI1NSS_PA4 \
    GPIO_INIT_AF(GPIOA, 4, 5, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_SPI1NSS_PA15 \
    GPIO_INIT_AF(GPIOA, \
                 15, \
                 5, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)

/* * SPI2 Pins - RET Package (64-pin)
 * PB13=SCK, PB14=MISO, PB15=MOSI, PB12=NSS (AF5)
 */
#define GPIO_INIT_SPI2SCK_RET_PB13 \
    GPIO_INIT_AF(GPIOB, \
                 13, \
                 5, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_SPI2MISO_RET_PB14 \
    GPIO_INIT_AF(GPIOB, 14, 5, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_PULL_UP)

#define GPIO_INIT_SPI2MOSI_RET_PB15 \
    GPIO_INIT_AF(GPIOB, \
                 15, \
                 5, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_SPI2NSS_RET_PB12 \
    GPIO_INIT_AF(GPIOB, \
                 12, \
                 5, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)

/* * SPI2 Pins - CET Package (48-pin)
 * PA9=SCK, PA10=MISO, PA11=MOSI, PA8=NSS (AF5)
 */
#define GPIO_INIT_SPI2SCK_CET_PA9 \
    GPIO_INIT_AF(GPIOA, 9, 5, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_SPI2MISO_CET_PA10 \
    GPIO_INIT_AF(GPIOA, 10, 5, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_PULL_UP)

#define GPIO_INIT_SPI2MOSI_CET_PA11 \
    GPIO_INIT_AF(GPIOA, \
                 11, \
                 5, \
                 GPIO_OUTPUT_ULTRA_SPEED, \
                 GPIO_OUTPUT_PUSH_PULL, \
                 GPIO_INPUT_OPEN_DRAIN)

#define GPIO_INIT_SPI2NSS_CET_PA8 \
    GPIO_INIT_AF(GPIOA, 8, 5, GPIO_OUTPUT_ULTRA_SPEED, GPIO_OUTPUT_PUSH_PULL, GPIO_INPUT_OPEN_DRAIN)

#endif // __PHAL_G4_GPIO_H__
