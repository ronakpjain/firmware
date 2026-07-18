#ifndef TEST_FAKE_STM32G474XX_H
#define TEST_FAKE_STM32G474XX_H

#include <stdint.h>

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
    volatile uint32_t BRR;
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t AHB2ENR;
} RCC_TypeDef;

extern GPIO_TypeDef test_gpioa;
extern GPIO_TypeDef test_gpiob;
extern GPIO_TypeDef test_gpioc;
extern GPIO_TypeDef test_gpiod;
extern GPIO_TypeDef test_gpioe;
extern GPIO_TypeDef test_gpiof;
extern GPIO_TypeDef test_gpiog;
extern RCC_TypeDef test_rcc;

#define GPIOA (&test_gpioa)
#define GPIOB (&test_gpiob)
#define GPIOC (&test_gpioc)
#define GPIOD (&test_gpiod)
#define GPIOE (&test_gpioe)
#define GPIOF (&test_gpiof)
#define GPIOG (&test_gpiog)
#define RCC   (&test_rcc)

#define RCC_AHB2ENR_GPIOAEN (UINT32_C(1) << 0)
#define RCC_AHB2ENR_GPIOBEN (UINT32_C(1) << 1)
#define RCC_AHB2ENR_GPIOCEN (UINT32_C(1) << 2)
#define RCC_AHB2ENR_GPIODEN (UINT32_C(1) << 3)
#define RCC_AHB2ENR_GPIOEEN (UINT32_C(1) << 4)
#define RCC_AHB2ENR_GPIOFEN (UINT32_C(1) << 5)
#define RCC_AHB2ENR_GPIOGEN (UINT32_C(1) << 6)

#define GPIO_MODER_MODE0_Msk     UINT32_C(0x3)
#define GPIO_MODER_MODE1_Pos     2U
#define GPIO_OSPEEDR_OSPEED0_Msk UINT32_C(0x3)
#define GPIO_OSPEEDR_OSPEED1_Pos 2U
#define GPIO_OTYPER_OT0_Msk      UINT32_C(0x1)
#define GPIO_OTYPER_OT1_Pos      1U
#define GPIO_PUPDR_PUPD0_Msk     UINT32_C(0x3)
#define GPIO_PUPDR_PUPD1_Pos     2U
#define GPIO_AFRL_AFSEL0_Msk     UINT32_C(0xF)
#define GPIO_AFRL_AFSEL1_Pos     4U

#endif // TEST_FAKE_STM32G474XX_H
