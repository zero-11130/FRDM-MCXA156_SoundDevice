#ifndef __COLLECT_HANDLE_H
#define __COLLECT_HANDLE_H

#include <rtdevice.h>
#include "drv_pin.h"
#include "stdio.h"
#include "string.h"
#include "board.h"
#include <rtthread.h>
#include "arm_math.h"

/* -------------------------- ADC Configuration --------------------------- */
#define ADC_DEV_NAME        "adc1"      /* ADC device name */
#define ADC_DEV_CHANNEL     0           /* ADC channel number */
#define REFER_VOLTAGE       330         /* Reference voltage 3.3V (¡Á100 for 2 decimal places) */
#define CONVERT_BITS        (1 << 16)   /* 16-bit conversion resolution */

/* -------------------------- FFT Configuration --------------------------- */
#define FFT_LENGTH      1024        /* FFT length (1024 points) */
#define NUM_PEAKS       9           /* Number of spectral peaks to detect */

/* ----------------------- Anomaly Detection Config ----------------------- */
#define SAMPLE_FREQ           			1000    /* Sampling frequency (Hz) - 1ms delay = 1000Hz */
#define POWER_THRESHOLD       			0.4f    /* Total power threshold (¡À20% from baseline) */
#define FEATURE_FREQ_RATIO_THRESHOLD 	0.15  /* Feature frequency power ratio threshold (15%) */
#define MOTOR_ROTATE_FREQ     			50      /* Motor rated rotation frequency (Hz) - 3000rpm = 50Hz */
#define CALIBRATE_TIMES       			3      /* Number of samples for baseline calibration */

/* -------------------------- Global Variables ---------------------------- */
extern rt_adc_device_t adc_dev;                     /* ADC device handle */
extern rt_uint32_t ADC_value[FFT_LENGTH], ADC_vol;  /* ADC sampling buffer */
extern rt_uint32_t sample_cnt;                  /* Sampling counter */

extern arm_cfft_radix4_instance_f32 scfft;          /* FFT structure */

extern float32_t totalPower;                        /* Total signal power (V2) */
extern rt_bool_t motor_status;            /* Motor status: RT_TRUE=Normal, RT_FALSE=Abnormal */

/* -------------------------- Function Prototypes ------------------------- */
void SignalAnalyzer_handler(void);
static inline void Amplitude_Convert(float32_t inputArray[], float32_t outputArray[]);
static inline void ADCdataToSpectrum(const uint32_t ADCdata[], float32_t fftArray[]);
static inline void find_peaks(const float32_t *fftOutput, uint16_t fftLength, uint16_t *peaks, uint32_t NumPeaks, uint16_t windowSize);
static inline float32_t powerCalculate(const float32_t *fft_Array, const uint16_t *peaks, uint16_t NumPeaks, float *power);
static float peak_index_to_freq(uint16_t peak_index);
void calibrate_normal_baseline(void);
static float calculate_feature_freq_ratio(void);
void motor_abnormal_detect(void);

#endif
