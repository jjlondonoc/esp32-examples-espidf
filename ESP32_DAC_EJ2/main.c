#include <stdint.h> 
#include "driver/dac_oneshot.h" 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
 
dac_oneshot_handle_t dac_handle; 
 
void app_main(void) 
{ 
    dac_oneshot_config_t config = { 
        .chan_id = DAC_CHAN_0, 
    }; 
 
    dac_oneshot_new_channel(&config, &dac_handle); 
 
    while (1) { 
        // Escribe un valor entre 0 y 255 
        // 0 = 0V, 255 = 3.3V, 128 = ~1.65V 
        dac_oneshot_output_voltage(dac_handle, 128); 
        vTaskDelay(pdMS_TO_TICKS(1000)); 
 
        dac_oneshot_output_voltage(dac_handle, 255); 
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
} 