#include <gtest/gtest.h>

#include <cstdint>
#include <iterator>

extern "C" {
#include "common/phal/gpio.h"
}

#include "g4_test_runtime.h"

namespace {

class G4GpioTest : public testing::Test {
protected:
    void SetUp() override {
        test_runtime_reset();
    }
};

TEST_F(G4GpioTest, InitializesOutputAndEnablesItsPeripheralClock) {
    GPIOA->MODER = UINT32_MAX;
    GPIOA->OSPEEDR = UINT32_MAX;
    GPIOA->OTYPER = UINT32_MAX;

    GPIOInitConfig_t config{};
    config.bank = GPIOA;
    config.pin = 5;
    config.type = GPIO_TYPE_OUTPUT;
    config.config.ospeed = GPIO_OUTPUT_HIGH_SPEED;
    config.config.otype = GPIO_OUTPUT_OPEN_DRAIN;

    ASSERT_TRUE(PHAL_initGPIO(&config, 1));
    EXPECT_NE(RCC->AHB2ENR & RCC_AHB2ENR_GPIOAEN, 0U);
    EXPECT_EQ((GPIOA->MODER >> 10) & 0x3U, GPIO_TYPE_OUTPUT);
    EXPECT_EQ((GPIOA->OSPEEDR >> 10) & 0x3U, GPIO_OUTPUT_HIGH_SPEED);
    EXPECT_EQ((GPIOA->OTYPER >> 5) & 0x1U, GPIO_OUTPUT_OPEN_DRAIN);
}

TEST_F(G4GpioTest, InitializesInputAndAlternateFunctionPins) {
    GPIOInitConfig_t configs[2]{};

    configs[0].bank = GPIOB;
    configs[0].pin = 3;
    configs[0].type = GPIO_TYPE_INPUT;
    configs[0].config.pull = GPIO_INPUT_PULL_UP;

    configs[1].bank = GPIOC;
    configs[1].pin = 10;
    configs[1].type = GPIO_TYPE_AF;
    configs[1].config.af_num = 7;
    configs[1].config.ospeed = GPIO_OUTPUT_ULTRA_SPEED;
    configs[1].config.otype = GPIO_OUTPUT_OPEN_DRAIN;
    configs[1].config.pull = GPIO_INPUT_PULL_DOWN;

    ASSERT_TRUE(PHAL_initGPIO(configs, 2));

    EXPECT_NE(RCC->AHB2ENR & RCC_AHB2ENR_GPIOBEN, 0U);
    EXPECT_EQ((GPIOB->MODER >> 6) & 0x3U, GPIO_TYPE_INPUT);
    EXPECT_EQ((GPIOB->PUPDR >> 6) & 0x3U, GPIO_INPUT_PULL_UP);

    EXPECT_NE(RCC->AHB2ENR & RCC_AHB2ENR_GPIOCEN, 0U);
    EXPECT_EQ((GPIOC->MODER >> 20) & 0x3U, GPIO_TYPE_AF);
    EXPECT_EQ((GPIOC->AFR[1] >> 8) & 0xFU, 7U);
    EXPECT_EQ((GPIOC->OSPEEDR >> 20) & 0x3U, GPIO_OUTPUT_ULTRA_SPEED);
    EXPECT_EQ((GPIOC->OTYPER >> 10) & 0x1U, GPIO_OUTPUT_OPEN_DRAIN);
    EXPECT_EQ((GPIOC->PUPDR >> 20) & 0x3U, GPIO_INPUT_PULL_DOWN);
}

TEST_F(G4GpioTest, EnablesClocksForEverySupportedGpioBank) {
    GPIOInitConfig_t configs[7]{};
    GPIO_TypeDef *banks[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG};

    for (size_t i = 0; i < std::size(configs); ++i) {
        configs[i].bank = banks[i];
        configs[i].pin = 0;
        configs[i].type = GPIO_TYPE_ANALOG;
    }

    ASSERT_TRUE(PHAL_initGPIO(configs, std::size(configs)));
    constexpr uint32_t all_gpio_clocks =
        RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN |
        RCC_AHB2ENR_GPIODEN | RCC_AHB2ENR_GPIOEEN | RCC_AHB2ENR_GPIOFEN |
        RCC_AHB2ENR_GPIOGEN;
    EXPECT_EQ(RCC->AHB2ENR & all_gpio_clocks, all_gpio_clocks);
}

TEST_F(G4GpioTest, RejectsUnknownBanksAndPinTypes) {
    GPIO_TypeDef unknown_bank{};
    GPIOInitConfig_t config{};
    config.bank = &unknown_bank;
    config.type = GPIO_TYPE_INPUT;
    EXPECT_FALSE(PHAL_initGPIO(&config, 1));

    config.bank = GPIOA;
    config.type = static_cast<GPIOPinType_t>(4);
    EXPECT_FALSE(PHAL_initGPIO(&config, 1));
}

TEST_F(G4GpioTest, ReadsWritesAndTogglesPinsUsingTheG4RegisterLayout) {
    GPIOA->IDR = UINT32_C(1) << 4;
    EXPECT_TRUE(test_g4_read_gpio(GPIOA, 4));
    EXPECT_FALSE(test_g4_read_gpio(GPIOA, 3));

    test_g4_write_gpio(GPIOA, 4, true);
    EXPECT_EQ(GPIOA->BSRR, UINT32_C(1) << 4);

    GPIOA->BSRR = 0;
    test_g4_write_gpio(GPIOA, 4, false);
    EXPECT_EQ(GPIOA->BSRR, UINT32_C(1) << 20);

    GPIOA->BSRR = 0;
    test_g4_toggle_gpio(GPIOA, 4);
    EXPECT_EQ(GPIOA->BSRR, UINT32_C(1) << 20);
}

} // namespace
