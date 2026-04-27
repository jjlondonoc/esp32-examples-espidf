#include <stdio.h> 
#include <string.h> 
#include "driver/uart.h" 
#include "esp_adc/adc_oneshot.h" 
#include "esp_adc/adc_cali.h" 
#include "esp_adc/adc_cali_scheme.h" 
 
#define UART_PORT UART_NUM_0 
 
#define ADC_UNIT       ADC_UNIT_2 
#define ADC_CHANNEL    ADC_CHANNEL_0    // GPIO4 en ESP32 
#define ADC_ATTEN      ADC_ATTEN_DB_12 
#define ADC_BITWIDTH   ADC_BITWIDTH_12 
 
void app_main(void) { 
    uart_config_t uart_config = { 
        .baud_rate = 9600, 
        .data_bits = UART_DATA_8_BITS, 
        .parity = UART_PARITY_DISABLE, 
        .stop_bits = UART_STOP_BITS_1, 
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, 
        .source_clk = UART_SCLK_DEFAULT, 
    }; 
 
    uart_param_config(UART_PORT, &uart_config); 
    uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0); 
 
    adc_oneshot_unit_handle_t adc_handle; 
    adc_oneshot_unit_init_cfg_t init_config = { 
        .unit_id = ADC_UNIT, 
    }; 
    adc_oneshot_new_unit(&init_config, &adc_handle); 
 
    adc_oneshot_chan_cfg_t config = { 
        .atten = ADC_ATTEN, 
        .bitwidth = ADC_BITWIDTH, 
    }; 
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config); 
 
    adc_cali_handle_t adc_cali_handle; 
    adc_cali_line_fitting_config_t cali_config = { 
        .unit_id = ADC_UNIT, 
        .atten = ADC_ATTEN, 
        .bitwidth = ADC_BITWIDTH, 
    }; 
    adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle); 

    char buffer[64]; 
    int raw; 
    int voltage_mv; 

    while (1) { 
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw); 
        adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_mv); 
        snprintf(buffer, sizeof(buffer), "V: %d mV\r\n", voltage_mv); 
        uart_write_bytes(UART_PORT, buffer, strlen(buffer)); 
        vTaskDelay(pdMS_TO_TICKS(500)); 
    } 
}
