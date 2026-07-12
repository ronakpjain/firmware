#include "common/phal_G4/adc/adc_internal.h"

#include "common/phal_G4/rcc/rcc.h"

static bool supported_adc(ADC_TypeDef *instance) {
    return instance == ADC1 || instance == ADC2 || instance == ADC3 || instance == ADC4;
}

static ADC_Common_TypeDef *common_for_adc(ADC_TypeDef *instance) {
    return (instance == ADC1 || instance == ADC2) ? ADC12_COMMON : ADC345_COMMON;
}

static void enable_adc_clock(ADC_TypeDef *instance) {
    if (instance == ADC1 || instance == ADC2) {
        RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    } else {
        RCC->AHB2ENR |= RCC_AHB2ENR_ADC345EN;
    }
    (void)RCC->AHB2ENR;
}

static void configure_adc_clock(ADC_TypeDef *instance) {
    ADC_Common_TypeDef *common = common_for_adc(instance);
    uint32_t ccr = common->CCR;
    ccr &= ~(ADC_CCR_CKMODE_Msk | ADC_CCR_PRESC_Msk);
    ccr |= ADC_CCR_CKMODE_0 | ADC_CCR_CKMODE_1; /* synchronous HCLK divided by four */
    common->CCR = ccr;
}

static bool wait_register(volatile uint32_t *reg, uint32_t mask, bool set) {
    for (uint32_t timeout = PHAL_ADC_INTERNAL_TIMEOUT; timeout != 0U; --timeout) {
        const bool active = (*reg & mask) != 0U;
        if (active == set) {
            return true;
        }
    }
    return false;
}

static bool wait_adc_flag(ADC_TypeDef *adc, uint32_t mask, bool set) {
    return wait_register(&adc->ISR, mask, set);
}

static bool disable_adc(ADC_TypeDef *adc) {
    if ((adc->CR & ADC_CR_ADSTART) != 0U) {
        adc->CR |= ADC_CR_ADSTP;
        if (!wait_register(&adc->CR, ADC_CR_ADSTART, false)) {
            return false;
        }
    }
    if ((adc->CR & ADC_CR_ADEN) != 0U) {
        adc->CR |= ADC_CR_ADDIS;
        for (uint32_t timeout = PHAL_ADC_INTERNAL_TIMEOUT; timeout != 0U; --timeout) {
            if ((adc->CR & ADC_CR_ADEN) == 0U) {
                return true;
            }
        }
        return false;
    }
    return true;
}

static void regulator_delay(void) {
    uint32_t clock_hz = PHAL_RCC_systemClockHz();
    if (clock_hz == 0U) {
        clock_hz = HSI_CLOCK_RATE_HZ;
    }
    uint32_t cycles = clock_hz / 50000U; /* 20 us at the published core clock */
    if (cycles < 1U) {
        cycles = 1U;
    }
    for (volatile uint32_t i = 0U; i < cycles; ++i) {
        __NOP();
    }
}

static bool valid_channels(const PHAL_ADC_Config_t *config) {
    if (config->channels == NULL || config->channel_count == 0U
        || config->channel_count > 16U) {
        return false;
    }

    bool used_ranks[16] = {false};
    for (size_t i = 0U; i < config->channel_count; ++i) {
        const PHAL_ADC_ChannelConfig_t *channel = &config->channels[i];
        if (channel->channel > 18U || channel->rank < 1U
            || channel->rank > config->channel_count
            || channel->sample_time > PHAL_ADC_SAMPLE_480_CYCLES
            || used_ranks[channel->rank - 1U]) {
            return false;
        }
        used_ranks[channel->rank - 1U] = true;
    }
    for (size_t i = 0U; i < config->channel_count; ++i) {
        if (!used_ranks[i]) {
            return false;
        }
    }
    return true;
}

static void set_sequence_slot(ADC_TypeDef *adc, uint8_t rank, uint8_t channel) {
    volatile uint32_t *sequence_register;
    uint32_t shift;
    if (rank <= 4U) {
        sequence_register = &adc->SQR1;
        shift = 6U + (6U * (rank - 1U));
    } else if (rank <= 9U) {
        sequence_register = &adc->SQR2;
        shift = 6U * (rank - 5U);
    } else if (rank <= 14U) {
        sequence_register = &adc->SQR3;
        shift = 6U * (rank - 10U);
    } else {
        sequence_register = &adc->SQR4;
        shift = 6U * (rank - 15U);
    }
    *sequence_register = (*sequence_register & ~(0x1FU << shift))
        | (((uint32_t)channel & 0x1FU) << shift);
}

static void configure_sequence(ADC_TypeDef *adc, const PHAL_ADC_Config_t *config) {
    adc->SQR1 = ((uint32_t)(config->channel_count - 1U) << ADC_SQR1_L_Pos)
        & ADC_SQR1_L_Msk;
    adc->SQR2 = 0U;
    adc->SQR3 = 0U;
    adc->SQR4 = 0U;
    adc->SMPR1 = 0U;
    adc->SMPR2 = 0U;

    for (size_t i = 0U; i < config->channel_count; ++i) {
        const PHAL_ADC_ChannelConfig_t *channel = &config->channels[i];
        set_sequence_slot(adc, channel->rank, channel->channel);
        if (channel->channel <= 9U) {
            const uint32_t shift = 3U * channel->channel;
            adc->SMPR1 = (adc->SMPR1 & ~(0x7U << shift))
                | ((uint32_t)channel->sample_time << shift);
        } else {
            const uint32_t shift = 3U * (channel->channel - 10U);
            adc->SMPR2 = (adc->SMPR2 & ~(0x7U << shift))
                | ((uint32_t)channel->sample_time << shift);
        }
    }
}

static bool configure_oversampling(ADC_TypeDef *adc, PHAL_ADC_Oversampling_t count) {
    uint32_t cfgr2 = adc->CFGR2;
    cfgr2 &= ~(ADC_CFGR2_ROVSE | ADC_CFGR2_OVSR_Msk | ADC_CFGR2_OVSS_Msk);
    if (count == PHAL_ADC_OVERSAMPLING_NONE) {
        adc->CFGR2 = cfgr2;
        return true;
    }
    if (count < PHAL_ADC_OVERSAMPLING_2 || count > PHAL_ADC_OVERSAMPLING_256
        || (((uint32_t)count & ((uint32_t)count - 1U)) != 0U)) {
        return false;
    }

    uint8_t ratio_encoding = 0U;
    uint32_t ratio = (uint32_t)count;
    while (ratio > 2U) {
        ratio >>= 1U;
        ++ratio_encoding;
    }
    uint8_t shift = 0U;
    ratio = (uint32_t)count;
    while (ratio > 1U) {
        ratio >>= 1U;
        ++shift;
    }
    cfgr2 |= ADC_CFGR2_ROVSE;
    cfgr2 |= ((uint32_t)ratio_encoding << ADC_CFGR2_OVSR_Pos) & ADC_CFGR2_OVSR_Msk;
    cfgr2 |= ((uint32_t)shift << ADC_CFGR2_OVSS_Pos) & ADC_CFGR2_OVSS_Msk;
    adc->CFGR2 = cfgr2;
    return true;
}

static void adc_dma_complete(void *context, bool success) {
    PHAL_ADC_Handle_t *handle = (PHAL_ADC_Handle_t *)context;
    if (handle == NULL || !handle->initialized) {
        return;
    }
    handle->success = success;
    if ((handle->instance->CFGR & ADC_CFGR_CONT) == 0U || !success) {
        handle->busy = false;
    }
}

bool PHAL_ADC_internalValidateConfig(const PHAL_ADC_Config_t *config) {
    return config != NULL && supported_adc(config->instance)
        && config->resolution <= PHAL_ADC_RESOLUTION_6_BIT
        && config->alignment <= PHAL_ADC_ALIGNMENT_LEFT
        && valid_channels(config);
}

bool PHAL_ADC_internalConfigureHardware(const PHAL_ADC_Config_t *config) {
    ADC_TypeDef *adc = config->instance;
    enable_adc_clock(adc);
    configure_adc_clock(adc);

    if (!disable_adc(adc)) {
        return false;
    }

    adc->CR &= ~ADC_CR_DEEPPWD;
    adc->CR |= ADC_CR_ADVREGEN;
    regulator_delay();

    adc->CR &= ~ADC_CR_ADCALDIF;
    adc->CR |= ADC_CR_ADCAL;
    if (!wait_register(&adc->CR, ADC_CR_ADCAL, false)) {
        return false;
    }

    uint32_t cfgr = adc->CFGR;
    cfgr &= ~(ADC_CFGR_CONT | ADC_CFGR_DISCEN | ADC_CFGR_DMAEN
              | ADC_CFGR_DMACFG | ADC_CFGR_RES_Msk | ADC_CFGR_ALIGN);
    cfgr |= ((uint32_t)config->resolution << ADC_CFGR_RES_Pos) & ADC_CFGR_RES_Msk;
    if (config->alignment == PHAL_ADC_ALIGNMENT_LEFT) {
        cfgr |= ADC_CFGR_ALIGN;
    }
    cfgr |= ADC_CFGR_DMAEN;
    if (config->continuous) {
        cfgr |= ADC_CFGR_CONT | ADC_CFGR_DMACFG;
    }
    adc->CFGR = cfgr;

    configure_sequence(adc, config);
    return configure_oversampling(adc, config->oversampling);
}

bool PHAL_ADC_internalInitializeDma(PHAL_ADC_Handle_t *handle, ADC_TypeDef *adc) {
    const PHAL_DMA_Route_t *route = PHAL_DMA_internalAdcRoute(adc);
    return route != NULL && PHAL_DMA_internalInit(&handle->dma, route);
}

bool PHAL_ADC_internalEnable(PHAL_ADC_Handle_t *handle, const PHAL_ADC_Config_t *config) {
    ADC_TypeDef *adc = config->instance;
    adc->ISR = ADC_ISR_ADRDY | ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    adc->CR |= ADC_CR_ADEN;
    if (!wait_adc_flag(adc, ADC_ISR_ADRDY, true)) {
        (void)PHAL_DMA_abort(&handle->dma);
        (void)disable_adc(adc);
        return false;
    }

    handle->instance = adc;
    handle->sequence_length = config->channel_count;
    handle->busy = false;
    handle->success = false;
    handle->initialized = true;
    return true;
}

bool PHAL_ADC_internalValidateStart(
    const PHAL_ADC_Handle_t *handle,
    const uint16_t *samples,
    size_t sample_count
) {
    return handle != NULL && handle->initialized && handle->instance != NULL
        && samples != NULL && sample_count != 0U && !handle->busy
        && handle->sequence_length != 0U && sample_count % handle->sequence_length == 0U
        && (handle->instance->CR & ADC_CR_ADEN) != 0U
        && (handle->instance->ISR & ADC_ISR_ADRDY) != 0U;
}

bool PHAL_ADC_internalStartDma(
    PHAL_ADC_Handle_t *handle,
    uint16_t *samples,
    size_t sample_count
) {
    handle->busy = true;
    handle->success = false;
    handle->instance->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    const bool circular = (handle->instance->CFGR & ADC_CFGR_CONT) != 0U;
    if (!PHAL_DMA_internalStart(
            &handle->dma,
            (volatile void *)&handle->instance->DR,
            samples,
            sample_count,
            true,
            circular,
            adc_dma_complete,
            handle)) {
        handle->busy = false;
        return false;
    }

    handle->instance->CR |= ADC_CR_ADSTART;
    return true;
}

bool PHAL_ADC_internalValidateHandle(const PHAL_ADC_Handle_t *handle) {
    return handle != NULL && handle->initialized && handle->instance != NULL;
}

bool PHAL_ADC_internalStopConversion(PHAL_ADC_Handle_t *handle) {
    if ((handle->instance->CR & ADC_CR_ADSTART) == 0U) {
        return true;
    }

    handle->instance->CR |= ADC_CR_ADSTP;
    return wait_register(&handle->instance->CR, ADC_CR_ADSTART, false);
}

void PHAL_ADC_internalCompleteStop(PHAL_ADC_Handle_t *handle, bool success) {
    handle->busy = false;
    handle->success = success;
}
