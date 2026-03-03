#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define PIN_BUTTON 2
#define PIN_LED 32

void app_main() {

    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << PIN_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&out_cfg);
    gpio_set_level(PIN_LED, 0);

    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL << PIN_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&in_cfg);

    bool led_state = false;
    bool last_button_state = true;

    while(1) {

        bool current_button_state = gpio_get_level(PIN_BUTTON);

        if (last_button_state == 1 && current_button_state == 0) {
            led_state = !led_state;
            gpio_set_level(PIN_LED, led_state);

            vTaskDelay(pdMS_TO_TICKS(200));
        }

        last_button_state = current_button_state;
        
    }
}