#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define ALARM_PIN 2
#define SENSOR_A_PIN 32
#define SENSOR_B_PIN 33
#define SENSOR_C_PIN 25

static inline bool btn_pressed(gpio_num_t pin) {
    if(gpio_get_level(pin) == 0) {
        return true;
    }
    else {
        return false;
    }
}

void app_main() {

    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << ALARM_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&out_cfg);
    gpio_set_level(ALARM_PIN, 0);

    gpio_config_t in_cfg = {
        .pin_bit_mask = (1ULL << SENSOR_A_PIN) | (1ULL << SENSOR_B_PIN) | (1ULL << SENSOR_C_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&in_cfg);

    while(1) {
        bool door = btn_pressed(SENSOR_A_PIN);
        bool temperature = btn_pressed(SENSOR_B_PIN);
        bool humidity = btn_pressed(SENSOR_C_PIN);

        bool z = (door && (humidity ^ temperature)) || (humidity && temperature);
        gpio_set_level(ALARM_PIN, z);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}