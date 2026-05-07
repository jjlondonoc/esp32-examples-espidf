#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_bt.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define LED_GPIO GPIO_NUM_26
#define DEVICE_NAME "ESP32-NUS"

static const char *TAG = "ESP32_NUS";

static uint8_t own_addr_type;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t nus_tx_val_handle;
static bool notify_enabled = false;

/*
 * UUIDs oficiales del Nordic UART Service:
 * Service: 6e400001-b5a3-f393-e0a9-e50e24dcca9e
 * RX:      6e400002-b5a3-f393-e0a9-e50e24dcca9e
 * TX:      6e400003-b5a3-f393-e0a9-e50e24dcca9e
 *
 * En NimBLE se escriben en orden little-endian.
 */
static const ble_uuid128_t nus_service_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);

static const ble_uuid128_t nus_rx_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e);

static const ble_uuid128_t nus_tx_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

static void nus_send_response(const char *msg)
{
    if (conn_handle == BLE_HS_CONN_HANDLE_NONE || !notify_enabled) {
        ESP_LOGW(TAG, "Cliente no conectado o no suscrito a TX Notify");
        return;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(msg, strlen(msg));

    if (om == NULL) {
        ESP_LOGE(TAG, "No se pudo crear buffer BLE");
        return;
    }

    int rc = ble_gatts_notify_custom(conn_handle, nus_tx_val_handle, om);

    if (rc != 0) {
        ESP_LOGE(TAG, "Error enviando notificacion: %d", rc);
    }
}

static int nus_rx_access_cb(uint16_t conn_handle_param,
                            uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt,
                            void *arg)
{
    uint8_t data[64];
    uint16_t len = 0;

    int rc = ble_hs_mbuf_to_flat(ctxt->om, data, sizeof(data) - 1, &len);

    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    data[len] = '\0';

    ESP_LOGI(TAG, "Dato recibido por BLE: %s", data);

    if (len > 0 && data[0] == 'E') {
        gpio_set_level(LED_GPIO, 1);
        nus_send_response("LED encendido");
    } else if (len > 0 && data[0] == 'A') {
        gpio_set_level(LED_GPIO, 0);
        nus_send_response("LED apagado");
    } else {
        nus_send_response("Error: comando desconocido");
    }

    return 0;
}

static int nus_tx_access_cb(uint16_t conn_handle_param,
                            uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt,
                            void *arg)
{
    return 0;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &nus_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &nus_rx_uuid.u,
                .access_cb = nus_rx_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid = &nus_tx_uuid.u,
                .access_cb = nus_tx_access_cb,
                .val_handle = &nus_tx_val_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            },
            {
                0,
            }
        },
    },
    {
        0,
    }
};

static void ble_app_advertise(void);

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "Cliente conectado");
        } else {
            ESP_LOGW(TAG, "Conexion fallida, reanudando advertising");
            ble_app_advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Cliente desconectado");
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
        notify_enabled = false;
        ble_app_advertise();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == nus_tx_val_handle) {
            notify_enabled = event->subscribe.cur_notify;
            ESP_LOGI(TAG, "TX Notify: %s", notify_enabled ? "ON" : "OFF");
        }
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ble_app_advertise();
        return 0;

    default:
        return 0;
    }
}

static void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;
    struct ble_gap_adv_params adv_params;

    int rc;

    // Advertising principal: solo flags + UUID del servicio
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    fields.uuids128 = (ble_uuid128_t *)&nus_service_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);

    if (rc != 0) {
        ESP_LOGE(TAG, "Error configurando advertising: %d", rc);
        return;
    }

    // Scan response: nombre del dispositivo
    memset(&rsp_fields, 0, sizeof(rsp_fields));

    const char *name = ble_svc_gap_device_name();

    rsp_fields.name = (uint8_t *)name;
    rsp_fields.name_len = strlen(name);
    rsp_fields.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);

    if (rc != 0) {
        ESP_LOGE(TAG, "Error configurando scan response: %d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(
        own_addr_type,
        NULL,
        BLE_HS_FOREVER,
        &adv_params,
        ble_gap_event_cb,
        NULL
    );

    if (rc != 0) {
        ESP_LOGE(TAG, "Error iniciando advertising: %d", rc);
    } else {
        ESP_LOGI(TAG, "Advertising iniciado como %s", DEVICE_NAME);
    }
}

static void ble_app_on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);

    if (rc != 0) {
        ESP_LOGE(TAG, "Error obteniendo direccion BLE: %d", rc);
        return;
    }

    ble_app_advertise();
}

static void ble_app_on_reset(int reason)
{
    ESP_LOGE(TAG, "Reset BLE, reason=%d", reason);
}

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(nimble_port_init());

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_svc_gap_device_name_set(DEVICE_NAME);

    ble_hs_cfg.sync_cb = ble_app_on_sync;
    ble_hs_cfg.reset_cb = ble_app_on_reset;

    ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_svr_svcs));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_svr_svcs));

    nimble_port_freertos_init(ble_host_task);
}