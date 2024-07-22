#include "temperature.h"
#include <zephyr/drivers/adc.h>
#include <zephyr/random/random.h>

static const uint16_t V25_mV = 1430;
static const float Avg_Slope = 4.3;

void temp_sensor_init(void)
{
#ifdef CONFIG_BOARD_NUCLEO_F103RB

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC1->SMPR1 |= ADC_SMPR1_SMP16;
    ADC1->CR2 |= ADC_CR2_EXTSEL | ADC_CR2_TSVREFE;
    ADC1->CR2 |= ADC_CR2_ADON;

    uint32_t timeout = 256;
    while(timeout--);

    ADC1->CR2 |= ADC_CR2_CAL;
    while (!(ADC1->CR2 & ADC_CR2_CAL));

#else   /* CONFIG_BOARD_NUCLEO_F103RB */
    srand(0);
#endif /* CONFIG_BOARD_NUCLEO_F103RB */
}

int16_t temp_sensor_read(void)
{
#ifdef CONFIG_BOARD_NUCLEO_F103RB

    ADC1->SQR3 = ADC_CHANNEL_16;
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while(!(ADC1->SR & ADC_SR_EOC));

    uint16_t adc_value_mV = ADC1->DR * 3300 / 4096;

    return ((V25_mV - adc_value_mV) / Avg_Slope) + 25;

#else /* CONFIG_BOARD_NUCLEO_F103RB */
    return (rand() % 35);
#endif /* CONFIG_BOARD_NUCLEO_F103RB */
}