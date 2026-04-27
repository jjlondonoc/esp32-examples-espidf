#include <stdio.h> 
#include <string.h> 
#include "driver/uart.h" 
#include "driver/gpio.h" 
 
#define UART_PORT UART_NUM_0 
#define LED_PIN 2 
 
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
    uart_driver_install(UART_PORT, 1024, 1024, 0, NULL, 0); 
 
    gpio_config_t io_conf = { 
        .pin_bit_mask = (1ULL << LED_PIN), 
        .mode = GPIO_MODE_OUTPUT 
    }; 
 
    gpio_config(&io_conf); 
 
    char *mensaje_inicio = 
        "\nControl de LED por UART\n" 
        "Presione:\n" 
        "E -> Encender LED\n" 
        "A -> Apagar LED\n\n"; 
 
    uart_write_bytes(UART_PORT, mensaje_inicio, strlen(mensaje_inicio)); 
 
    uint8_t data; 
 
    while (1) { 
 
        int len = uart_read_bytes(UART_PORT, &data, 1, portMAX_DELAY); 
 
        if (len > 0) {
            if (data == 'E') { 
                gpio_set_level(LED_PIN, 1); 
                char *msg = "LED encendido\n"; 
                uart_write_bytes(UART_PORT, msg, strlen(msg)); 
            } 
            if (data == 'A') { 
                gpio_set_level(LED_PIN, 0); 
                char *msg = "LED apagado\n"; 
                uart_write_bytes(UART_PORT, msg, strlen(msg)); 
            } 
        } 
    } 
}
