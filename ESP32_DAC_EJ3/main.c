#include "driver/dac_cosine.h" 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
 
dac_cosine_handle_t dac_handle; 
 
void app_main(void) 
{ 
    dac_cosine_config_t config = { 
        .chan_id    = DAC_CHAN_0, 
        .freq_hz    = 1000,      // Frecuencia de la onda: 1 kHz 
        .clk_src    = DAC_COSINE_CLK_SRC_DEFAULT, 
        .offset     = 0,         // Offset DC: -128 a 127 
        .phase      = DAC_COSINE_PHASE_0, 
        .atten      = DAC_COSINE_ATTEN_DEFAULT,  // Amplitud máxima 
        .flags.force_set_freq = false, 
    }; 
 
    dac_cosine_new_channel(&config, &dac_handle); 
    dac_cosine_start(dac_handle); 
 
    while (1) { 
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
} 