#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"

#include "driver/gpio.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"

#define LED_GPIO GPIO_NUM_26

static const char *TAG = "BT_SPP_LED";
static const char *DEVICE_NAME = "ESP32_SPP_LED";
static const char *SPP_SERVER_NAME = "SPP_SERVER";

static uint32_t spp_handle = 0;

static void spp_send(const char *msg) {
    if (spp_handle != 0) {
        esp_spp_write(spp_handle, strlen(msg), (uint8_t *)msg);
    }
}

static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            ESP_LOGI(TAG, "Autenticacion Bluetooth completada");
            break;

        default:
            break;
    }
}

static void spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            ESP_LOGI(TAG, "SPP inicializado");

            esp_bt_gap_set_device_name(DEVICE_NAME);
            esp_bt_gap_set_scan_mode(
                ESP_BT_CONNECTABLE,
                ESP_BT_GENERAL_DISCOVERABLE
            );

            esp_spp_start_srv(
                ESP_SPP_SEC_NONE,
                ESP_SPP_ROLE_SLAVE,
                0,
                SPP_SERVER_NAME
            );
            break;

        case ESP_SPP_START_EVT:
            ESP_LOGI(TAG, "Servidor SPP iniciado");
            break;

        case ESP_SPP_SRV_OPEN_EVT:
            spp_handle = param->srv_open.handle;
            ESP_LOGI(TAG, "Cliente SPP conectado");
            break;

        case ESP_SPP_CLOSE_EVT:
            spp_handle = 0;
            ESP_LOGI(TAG, "Cliente SPP desconectado");
            break;

        case ESP_SPP_DATA_IND_EVT:
            if (param->data_ind.len > 0) {
                char cmd = (char)param->data_ind.data[0];

                if (cmd == 'E') {
                    gpio_set_level(LED_GPIO, 1);
                    spp_send("LED encendido\r\n");
                }
                else if (cmd == 'A') {
                    gpio_set_level(LED_GPIO, 0);
                    spp_send("LED apagado\r\n");
                }
                else {
                    spp_send("Error: comando desconocido\r\n");
                }
            }
            break;

        default:
            break;
    }
}

void app_main(void) {
    esp_err_t ret;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_callback));
    ESP_ERROR_CHECK(esp_spp_register_callback(spp_callback));

    esp_spp_cfg_t spp_cfg = {
        .mode = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = true,
        .tx_buffer_size = 0
    };

    ESP_ERROR_CHECK(esp_spp_enhanced_init(&spp_cfg));

    ESP_LOGI(TAG, "Bluetooth listo. Nombre: %s", DEVICE_NAME);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}