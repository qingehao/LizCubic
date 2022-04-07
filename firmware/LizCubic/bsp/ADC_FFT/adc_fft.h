#ifndef _ADC_FFT_H_
#define _ADC_FFT_H_
#include "stdint.h"
#include "arm_math.h"
void adc_fft_init(void (*cb)(float *fft_data, int32_t fft_len));
void adc_fft_start();
void adc_fft_stop();

#endif
