#include <math.h> 
#include <stdint.h> 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "driver/dac_continuous.h" 
 
#define SAMPLES  100 
#define FREQ_HZ  20000 
// Frecuencia de la senoidal = FREQ_HZ / SAMPLES = 200 Hz 
 
dac_continuous_handle_t dac_handle; 
uint8_t sine_table[SAMPLES]; 
 
void app_main(void) 
{ 
    for (int i = 0; i < SAMPLES; i++) { 
        float angle = 2.0f * 3.14159265f * i / SAMPLES; 
        sine_table[i] = (uint8_t)((sinf(angle) + 1.0f) * 127.5f); 
    } 
 
    dac_continuous_config_t config = { 
        .chan_mask = DAC_CHANNEL_MASK_CH0, 
        .desc_num = 4, 
        .buf_size = 256, 
        .freq_hz = FREQ_HZ, 
        .offset = 0, 
        .clk_src = DAC_DIGI_CLK_SRC_DEFAULT, 
        .chan_mode = DAC_CHANNEL_MODE_ALTER, 
    }; 
 
    dac_continuous_new_channels(&config, &dac_handle); 
    dac_continuous_enable(dac_handle); 
    dac_continuous_write_cyclically(dac_handle, sine_table, sizeof(sine_table), NULL); 
 
    while (1) { 
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
} 
 