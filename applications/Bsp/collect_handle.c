#include "collect_handle.h"


/* -------------------------- Global Variables ---------------------------- */
rt_adc_device_t adc_dev;                     /* ADC device handle */
rt_uint32_t ADC_value[FFT_LENGTH], ADC_vol;  /* ADC sampling buffer */
rt_uint32_t sample_cnt = 0;                  /* Sampling counter */

arm_cfft_radix4_instance_f32 scfft;          /* FFT structure */
float32_t FFT_InputBuff[FFT_LENGTH * 2];     /* FFT input buffer (complex) */
float32_t FFT_OutputBuf[FFT_LENGTH / 2];     /* FFT output amplitude buffer */

uint16_t peaks[NUM_PEAKS];                   /* Spectral peak indexes */
float power[NUM_PEAKS];                      /* Harmonic power values */
float32_t totalPower;                        /* Total signal power (V2) */

float32_t normal_power_baseline;             /* Calibrated normal total power baseline */
float32_t normal_feature_ratio;              /* Calibrated feature frequency power ratio baseline */
rt_bool_t motor_status = RT_TRUE;            /* Motor status: RT_TRUE=Normal, RT_FALSE=Abnormal */


/* -------------------------- Core Functions ------------------------------ */
/**
 * @brief Convert FFT complex spectrum to amplitude spectrum
 * @param inputArray FFT complex output array
 * @param outputArray Amplitude spectrum output array
 */
static inline void Amplitude_Convert(float32_t inputArray[], float32_t outputArray[])
{
    arm_cmplx_mag_f32(inputArray, outputArray, FFT_LENGTH / 2);
}

/**
 * @brief Convert ADC sampling data to frequency spectrum via FFT
 * @param ADCdata Raw ADC sampling data
 * @param fftArray FFT complex input buffer
 */
static inline void ADCdataToSpectrum(const uint32_t ADCdata[], float32_t fftArray[])
{
    for (uint32_t i = 0; i < FFT_LENGTH; ++i)
    {
        fftArray[i * 2] = (float)ADCdata[i] * 3.3 / 65535;  /* Convert to actual voltage (V) */
        fftArray[i * 2 + 1] = 0;                           /* Imaginary part = 0 for real signal */
    }
    arm_cfft_radix4_f32(&scfft, fftArray);                 /* 4-radix FFT calculation */
}

/**
 * @brief Find spectral peaks in amplitude spectrum
 * @param fftOutput Amplitude spectrum array
 * @param fftLength Length of amplitude spectrum
 * @param peaks Buffer to store peak indexes
 * @param NumPeaks Maximum number of peaks to detect
 * @param windowSize Sliding window size (odd number)
 */
static inline void find_peaks(const float32_t *fftOutput, uint16_t fftLength, uint16_t *peaks, uint32_t NumPeaks, uint16_t windowSize)
{
    float32_t maxVal;
    uint16_t maxIdx;
    uint16_t halfWindowSize = windowSize / 2;
    uint16_t numPeaks = 1;  /* Start from 1st harmonic (skip DC component) */

    /* Reset peak buffer */
    for (uint16_t i = 0; i < NumPeaks; ++i)
    {
        peaks[i] = 0;
    }

    /* Sliding window peak detection */
    for (uint16_t i = halfWindowSize; i < 512 - halfWindowSize; i++)
    {
        maxVal = fftOutput[i];
        maxIdx = i;

        for (uint16_t j = i - halfWindowSize; j <= i + halfWindowSize; j++)
        {
            if (fftOutput[j] > maxVal)
            {
                maxVal = fftOutput[j];
                maxIdx = j;
            }
        }

        /* Check if current index is the window maximum */
        if (maxIdx == i)
        {
            if (peaks[numPeaks] == 0 || (i - peaks[numPeaks - 1]) > windowSize)
            {
                peaks[numPeaks] = i;
                ++numPeaks;

                if (numPeaks == NumPeaks)
                    return;
            }
        }
    }
}

/**
 * @brief Calculate total power from spectral peaks (exclude DC component)
 * @param fft_Array Amplitude spectrum array
 * @param peaks Spectral peak indexes
 * @param NumPeaks Number of peaks
 * @param power Buffer to store harmonic power
 * @return Total signal power (V2)
 */
static inline float32_t powerCalculate(const float32_t *fft_Array, const uint16_t *peaks, uint16_t NumPeaks, float *power)
{
    float sum = 0;

    /* Reset power buffer */
    for (uint16_t i = 0; i < NumPeaks; ++i)
    {
        power[i] = 0;
    }

    /* Calculate power for each harmonic (skip DC component at index 0) */
    for (uint16_t i = 1; i < NumPeaks; i++)
    {
        if (peaks[i] == 0)
        {
            break;  /* No more harmonics */
        }
        power[i] = fft_Array[peaks[i]] * fft_Array[peaks[i]] * 2 / FFT_LENGTH;
        sum += power[i];
    }
    return sum;
}

/**
 * @brief Convert FFT peak index to actual frequency (Hz)
 * @param peak_index FFT peak index
 * @return Corresponding frequency (Hz)
 */
static float peak_index_to_freq(uint16_t peak_index)
{
    return (float)peak_index * SAMPLE_FREQ / FFT_LENGTH;
}

/**
 * @brief Calibrate normal motor baseline (total power + feature frequency ratio)
 * @note Call once when motor is running normally
 */
void calibrate_normal_baseline(void)
{
    rt_kprintf("Start motor baseline calibration... Please ensure motor is running normally.\n");
    rt_thread_mdelay(3000);  /* Wait for motor to stabilize */

    float total_power_avg = 0;
    float feature_ratio_avg = 0;

    /* Average multiple samples for stable baseline */
    for (int i = 0; i < CALIBRATE_TIMES; i++)
    {
        /* Collect ADC data */
        for (sample_cnt = 0; sample_cnt < FFT_LENGTH; sample_cnt++)
        {
            ADC_value[sample_cnt] = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
            rt_thread_mdelay(1);
        }

        /* Calculate spectrum and power */
        SignalAnalyzer_handler();
        total_power_avg += totalPower;

        /* Calculate feature frequency (motor rotation freq) power ratio */
        float feature_power = 0;
        for (int j = 1; j < NUM_PEAKS; j++)
        {
            if (peaks[j] == 0) break;
            float freq = peak_index_to_freq(peaks[j]);
            /* Match rotation frequency ¡À5Hz (tolerance) */
            if ((freq > MOTOR_ROTATE_FREQ - 5) && (freq < MOTOR_ROTATE_FREQ + 5))
            {
                feature_power = power[j];
                break;
            }
        }
        feature_ratio_avg += (feature_power / totalPower);
    }

    /* Calculate average baseline */
    normal_power_baseline = total_power_avg / CALIBRATE_TIMES;
    normal_feature_ratio = feature_ratio_avg / CALIBRATE_TIMES;

    rt_kprintf("Baseline calibration completed: Normal total power = %.4f V2, Feature ratio = %.2f%%\n",
               normal_power_baseline, normal_feature_ratio * 100);
}

/**
 * @brief Calculate real-time feature frequency power ratio
 * @return Feature frequency power ratio (0~1)
 */
static float calculate_feature_freq_ratio(void)
{
    float feature_power = 0;
    for (int j = 1; j < NUM_PEAKS; j++)
    {
        if (peaks[j] == 0) break;
        float freq = peak_index_to_freq(peaks[j]);
        /* Match rotation frequency ¡À5Hz */
        if ((freq > MOTOR_ROTATE_FREQ - 5) && (freq < MOTOR_ROTATE_FREQ + 5))
        {
            feature_power = power[j];
            break;
        }
    }
    if (totalPower < 0.0001) return 0;  /* Avoid division by zero */
    return feature_power / totalPower;
}

/**
 * @brief Motor anomaly detection core logic
 * @note Combine total power threshold and feature frequency ratio verification
 */
void motor_abnormal_detect(void)
{
    /* Step 1: Check total power deviation from baseline */
    float power_diff = fabs(totalPower - normal_power_baseline) / normal_power_baseline;
    if (power_diff > POWER_THRESHOLD)
    {
        /* Step 2: Verify feature frequency ratio for false alarm filtering */
        float feature_ratio = calculate_feature_freq_ratio();
        float ratio_diff = fabs(feature_ratio - normal_feature_ratio) / normal_feature_ratio;

        if (ratio_diff > FEATURE_FREQ_RATIO_THRESHOLD)
        {
            motor_status = RT_FALSE;
            rt_kprintf("[ANOMALY] Total power = %.4f V2 (Baseline: %.4f), Feature ratio = %.2f%% (Baseline: %.2f%%)\n",
                       totalPower, normal_power_baseline, feature_ratio * 100, normal_feature_ratio * 100);
        }
        else
        {
            motor_status = RT_TRUE;
            rt_kprintf("[NORMAL FLUCTUATION] Power changed but feature ratio normal, Ratio deviation = %.2f%%\n", ratio_diff * 100);
        }
    }
    else
    {
        motor_status = RT_TRUE;
        // Uncomment below for detailed normal log
        rt_kprintf("[NORMAL] Total power = %.4f V2, Feature ratio = %.2f%%\n", totalPower, calculate_feature_freq_ratio() * 100);
    }
}

/**
 * @brief Signal analysis pipeline (ADC ¡ú FFT ¡ú Power calculation)
 */
void SignalAnalyzer_handler(void)
{
    ADCdataToSpectrum(ADC_value, FFT_InputBuff);   /* Convert ADC data to spectrum */
    Amplitude_Convert(FFT_InputBuff, FFT_OutputBuf);/* Convert to amplitude spectrum */
    find_peaks(FFT_OutputBuf, FFT_LENGTH / 2, peaks, NUM_PEAKS, 41); /* Detect spectral peaks */
    totalPower = powerCalculate(FFT_OutputBuf, peaks, NUM_PEAKS, power); /* Calculate total power */
}

