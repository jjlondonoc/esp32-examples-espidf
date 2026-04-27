#include <stdint.h> 
#include "driver/timer.h"           
#include "esp_adc/adc_oneshot.h" 
#include "driver/ledc.h" 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
 
// ===================== PINES Y PARÁMETROS ===================== 
#define POT_ADC_CHANNEL     ADC_CHANNEL_6    // GPIO34 potenciómetro
#define LED_GPIO            18               // GPIO18 salida PWM del LED 
 
// PWM 
#define LEDC_TIMER_NUM      LEDC_TIMER_0 
#define LEDC_CHANNEL_NUM    LEDC_CHANNEL_0 
#define LEDC_FREQ_HZ        5000             // Frecuencia PWM: 5 kHz (imperceptible al ojo) 
#define LEDC_RESOLUTION     LEDC_TIMER_12_BIT // Resolución: 0 a 4095 
 
// Timer
#define TIMER_GROUP         TIMER_GROUP_0 
#define TIMER_IDX           TIMER_0 
#define TIMER_DIVIDER       80               // 80 MHz / 80 = 1 MHz (1 tick = 1 µs) 
#define SAMPLE_FREQ_HZ      100              // Frecuencia de muestreo: 100 Hz 
#define TIMER_ALARM_US      (1000000 / SAMPLE_FREQ_HZ) // 10000 µs = 10 ms 
 
static adc_oneshot_unit_handle_t adc_handle; 
static volatile bool sample_ready = false; 

// Se ejecuta por interrupción cada 10 ms 
static bool IRAM_ATTR timer_callback(void *args) { 
    sample_ready = true; // Señala al loop principal que es momento de muestrear 
    return false;
} 
 
// ===================== APP MAIN ===================== 
void app_main(void) { 
    adc_oneshot_unit_init_cfg_t adc_init_cfg = { 
        .unit_id  = ADC_UNIT_1, 
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    }; 
    adc_oneshot_new_unit(&adc_init_cfg, &adc_handle); 
 
    adc_oneshot_chan_cfg_t adc_chan_cfg = { 
        .atten    = ADC_ATTEN_DB_12,      // Atenuación 12 dB: rango 0 a ~3.3V 
        .bitwidth = ADC_BITWIDTH_12,      // Resolución 12 bits: 0 a 4095 
    }; 
    adc_oneshot_config_channel(adc_handle, POT_ADC_CHANNEL, &adc_chan_cfg); 
 
    timer_config_t timer_cfg = { 
        .clk_src     = TIMER_SRC_CLK_DEFAULT, 
        .divider     = TIMER_DIVIDER,       // 1 tick = 1 µs 
        .counter_dir = TIMER_COUNT_UP, 
        .counter_en  = TIMER_PAUSE,         // Inicia pausado hasta timer_start() 
        .alarm_en    = TIMER_ALARM_EN,      // Habilita la alarma 
        .auto_reload = TIMER_AUTORELOAD_EN, // Se reinicia solo en cada alarma 
    }; 
    timer_init(TIMER_GROUP, TIMER_IDX, &timer_cfg); 
    timer_set_counter_value(TIMER_GROUP, TIMER_IDX, 0); 
    timer_set_alarm_value(TIMER_GROUP, TIMER_IDX, TIMER_ALARM_US); // Alarma a los 10000 µs 
    timer_enable_intr(TIMER_GROUP, TIMER_IDX); 
    timer_isr_callback_add(TIMER_GROUP, TIMER_IDX, timer_callback, NULL, 0); 
    timer_start(TIMER_GROUP, TIMER_IDX); 
 
    ledc_timer_config_t ledc_timer = { 
        .speed_mode      = LEDC_LOW_SPEED_MODE, // Compatible con todos los GPIO 
        .timer_num       = LEDC_TIMER_0, 
        .duty_resolution = LEDC_TIMER_12_BIT,     // 12 bits: duty entre 0 y 4095 
        .freq_hz         = 5000,        // 5 kHz: sin parpadeo visible 
        .clk_cfg         = LEDC_AUTO_CLK,       // Selección automática del reloj 
    }; 
    ledc_timer_config(&ledc_timer); 
 
    ledc_channel_config_t ledc_channel = { 
        .speed_mode = LEDC_LOW_SPEED_MODE, 
        .channel    = LEDC_CHANNEL_0, 
        .timer_sel  = LEDC_TIMER_0, 
        .intr_type  = LEDC_INTR_DISABLE,  // Sin interrupción al cambiar duty 
        .gpio_num   = 18, 
        .duty       = 0,                   // Duty inicial: LED apagado 
        .hpoint     = 0,                   // Punto de inicio del pulso: 0 (normal) 
    }; 
    ledc_channel_config(&ledc_channel); 
 
    int adc_raw = 0; 
 
    while (1) { 
        if (sample_ready) {                 // El timer indicó que es momento de muestrear 
            sample_ready = false; 
 
            adc_oneshot_read(adc_handle, POT_ADC_CHANNEL, &adc_raw); 
 
            // Mapeo directo: ADC 12 bits (0-4095): PWM 12 bits (0-4095) 
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (uint32_t)adc_raw); 
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); 
        } 
        vTaskDelay(pdMS_TO_TICKS(1));
    } 
} 