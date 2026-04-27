#include <stdio.h> 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "driver/spi_master.h" 
 
#define PIN_MOSI  32 
#define PIN_CLK   25 
#define PIN_CS    33 
 
spi_device_handle_t display; 
 
// Envía 16 bits al MAX7219: [registro (8 bits)] [dato (8 bits)] 
void max7219_write(uint8_t reg, uint8_t dato) { 
    uint8_t buf[2]; 
    buf[0] = reg;   // byte alto: dirección del registro 
    buf[1] = dato;  // byte bajo: dato 
 
    spi_transaction_t t = { 
        .length    = 16, 
        .tx_buffer = buf, 
    }; 
    spi_device_transmit(display, &t); 
} 
 
void app_main(void) { 
    // Inicializar bus SPI 
    spi_bus_config_t bus = { 
        .mosi_io_num   = PIN_MOSI, 
        .miso_io_num   = -1, 
        .sclk_io_num   = PIN_CLK, 
        .quadwp_io_num = -1, 
        .quadhd_io_num = -1, 
    }; 
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO); 
 
    // Agregar el MAX7219 como dispositivo SPI 
    spi_device_interface_config_t dev = { 
        .clock_speed_hz = 1000000,  // 1 MHz 
        .mode           = 0,        // CPOL=0, CPHA=0 
        .spics_io_num   = PIN_CS, 
        .queue_size     = 1, 
    }; 
    spi_bus_add_device(SPI2_HOST, &dev, &display); 
    // Configuración mínima del MAX7219 
    max7219_write(0x0C, 0x01);  // Shutdown OFF: operación normal 
    max7219_write(0x0F, 0x01);  // Display test: todos los LEDs ON 
    vTaskDelay(pdMS_TO_TICKS(2000)); 
    max7219_write(0x0F, 0x00);  // Display test: No test 
    max7219_write(0x09, 0xFF);  // Decode mode: Code en los 4 digitps
    max7219_write(0x0B, 0x03);  // Scan limit    

    max7219_write(0x0A, 0x08);  // Intensity     

    max7219_write(0x01, 4);  // DIG 0 = "4" 
    max7219_write(0x02, 3);  // DIG 1 = "3" 
    max7219_write(0x03, 2);  // DIG 2 = "2" 
    max7219_write(0x04, 1);  // DIG 3 = "1" 
}