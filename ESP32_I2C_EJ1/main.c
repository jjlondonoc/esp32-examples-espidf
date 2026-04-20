#include <stdio.h> 
#include "driver/i2c.h" 
#include "esp_log.h" 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
 
// Dirección I2C del DS1307: fija en 0x68 (7 bits: 1101000) 

#define DS1307_ADDR 0x68 
 
// Dirección del primer registro de tiempo (segundos) 
#define DS1307_REG_SEC  0x00 
 
// Pines I2C del ESP32
#define I2C_SDA_PIN 32 
#define I2C_SCL_PIN 33 
 
#define I2C_PORT I2C_NUM_0 
 
// Frecuencia del bus: 100000 Hz = 100 kHz 
#define I2C_FREQ_HZ 100000 

// TAG para los logs
static const char *TAG = "DS1307"; 
 
// Convertimos un número decimal a BCD 
// Separamos decenas y unidades y las ponemos cada una en 4 bits 
uint8_t decimal_a_bcd(uint8_t decimal) { 
    // (decimal / 10) nos da las decenas: van en los 4 bits altos 
    // (decimal % 10) nos da las unidades: van en los 4 bits bajos 
    // << 4 desplaza las decenas a la posición correcta 
    return ((decimal / 10) << 4) | (decimal % 10); 
} 
 
// Convertimos un valor BCD a decimal 
uint8_t bcd_a_decimal(uint8_t bcd) { 
    // >> 4 desplaza los 4 bits altos hacia abajo: obtenemos las decenas 
    // & 0x0F enmascara solo los 4 bits bajos: obtenemos las unidades 
    return ((bcd >> 4) * 10) + (bcd & 0x0F); 
} 
 
void i2c_inicializar(void) { 
 
    // Creamos la estructura de configuración del driver I2C de ESP-IDF 
    i2c_config_t config = { 
 
        // El ESP32 va a ser el MAESTRO: genera SCL, START, STOP 
        .mode = I2C_MODE_MASTER, 
        .sda_io_num = I2C_SDA_PIN,   // Pin físico para la línea de datos 
        .scl_io_num = I2C_SCL_PIN,   // Pin físico para la línea de reloj 
        .sda_pullup_en = GPIO_PULLUP_ENABLE, 
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ, 
    }; 
 
    // Aplicamos la configuración al puerto I2C 
    i2c_param_config(I2C_PORT, &config); 
 
    // Instalamos el driver I2C en el puerto configurado 
    // Los dos ceros finales son tamaños de buffer para modo esclavo, 
    // en modo maestro no se usan, por eso van en 0 
    i2c_driver_install(I2C_PORT, config.mode, 0, 0, 0); 
 
    ESP_LOGI(TAG, "Bus I2C inicializado a 100kHz en SDA=%d SCL=%d", 
             I2C_SDA_PIN, I2C_SCL_PIN); 
} 
 
void ds1307_escribir_hora(void) { 
 
    // Vamos a escribir: segundos, minutos, horas, día, fecha, mes, año 
    // Hora que queremos configurar: Lunes 13 de Abril 2026, 10:30:00 
    uint8_t segundos = 0; 
    uint8_t minutos  = 30; 
    uint8_t horas    = 10;  // Modo 24 horas — bit 6 del registro 0x02 = 0 
    uint8_t dia      = 2;
    uint8_t fecha    = 13; 
    uint8_t mes      = 4; 
    uint8_t anio     = 26;  // El DS1307 guarda 00-99, representa 2000-2099 
 
    // Creamos el "comando" I2C que es la estructura que acumula toda la 
    // transacción antes de enviarla. Funciona como armar el paquete completo
    i2c_cmd_handle_t cmd = i2c_cmd_link_create(); 
 
    // Paso 1: Condición START
    i2c_master_start(cmd); 
 
    // Paso 2: Dirección del esclavo + bit de ESCRITURA
    // Enviamos los 7 bits de dirección (0x68) seguidos del bit R/W = 0 (Write)
    i2c_master_write_byte(cmd, (DS1307_ADDR << 1) | I2C_MASTER_WRITE, true); 
    // DS1307_ADDR << 1 desplaza la dirección 1 bit a la izquierda para 
    // dejar espacio al bit R/W en la posición 0 
    // | I2C_MASTER_WRITE pone ese bit en 0 (escribir) 
    // El 'true' le dice al driver que espere el ACK del DS1307 
 
    // Paso 3: Dirección del registro desde donde empezamos a escribir
    // Le decimos al DS1307 quiero escribir desde el registro 0x00 (segundos) 
    // Después de esto, el puntero interno del DS1307 se autoincrementa con 
    // cada byte que enviemos, así escribimos todos los registros seguidos 
    i2c_master_write_byte(cmd, DS1307_REG_SEC, true); 
 
    // Paso 4: Escribimos los 7 registros de tiempo en BCD
 
    // Registro 0x00 — Segundos 
    i2c_master_write_byte(cmd, decimal_a_bcd(segundos), true); 
 
    // Registro 0x01 — Minutos 
    i2c_master_write_byte(cmd, decimal_a_bcd(minutos), true); 
 
    // Registro 0x02 — Horas en modo 24h 
    i2c_master_write_byte(cmd, decimal_a_bcd(horas), true); 
 
    // Registro 0x03 — Día de la semana 
    // Nosotros decidimos: 1=Dom, 2=Lun, 3=Mar, 4=Mie, 5=Jue, 6=Vie, 7=Sab 
    i2c_master_write_byte(cmd, decimal_a_bcd(dia), true); 
 
    // Registro 0x04 — Fecha (día del mes) 
    i2c_master_write_byte(cmd, decimal_a_bcd(fecha), true); 
 
    // Registro 0x05 — Mes 
    i2c_master_write_byte(cmd, decimal_a_bcd(mes), true); 
 
    // Registro 0x06 — Año (solo los dos últimos dígitos) 
    // Datasheet Tabla 2: rango 00-99. 26 representa el año 2026 
    i2c_master_write_byte(cmd, decimal_a_bcd(anio), true); 
 
    // Paso 5: Condición STOP
    // Le indicamos al DS1307 que la transacción terminó 
    i2c_master_stop(cmd); 
 
    // Paso 6: Enviamos todo lo que armamos
    // Hasta aquí solo construimos el paquete, aquí realmente se transmite 
    esp_err_t resultado = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000)); 
 
    // Liberamos la memoria del comando
    i2c_cmd_link_delete(cmd); 
 
    if (resultado == ESP_OK) { 
        ESP_LOGI(TAG, "Hora escrita correctamente en el DS1307"); 
    } 
    else { 
        ESP_LOGE(TAG, "Error al escribir en el DS1307: %s", 
                 esp_err_to_name(resultado)); 
    } 
}
 
void ds1307_leer_hora(void) { 
 
    // Buffer donde guardaremos los 7 bytes leídos (un byte por registro) 
    uint8_t datos[7]; 
 
    i2c_cmd_handle_t cmd = i2c_cmd_link_create(); 
 
    // Le decimos al DS1307 "quiero leer desde la dirección 0x00" 
 
    // START 
    i2c_master_start(cmd); 
 
    // Dirección + bit de escritura (R/W = 0) 
    i2c_master_write_byte(cmd, (DS1307_ADDR << 1) | I2C_MASTER_WRITE, true); 
 
    // Le enviamos la dirección del registro desde donde queremos leer 
    i2c_master_write_byte(cmd, DS1307_REG_SEC, true); 
 
    // Es un nuevo START sin haber hecho STOP, el bus no se libera 
    // Esto le indica al DS1307 que ahora vamos a leer 
    i2c_master_start(cmd); 
 
    // Dirección + bit de LECTURA (R/W = 1) 
    // El DS1307 ahora sabe que debe transmitir datos 
    i2c_master_write_byte(cmd, (DS1307_ADDR << 1) | I2C_MASTER_READ, true); 
    // | I2C_MASTER_READ pone el bit R/W en 1 (leer) 
 
    // Leemos bytes 0 al 5 enviando ACK después de cada uno 
    // ACK le dice al DS1307 "recibí el byte, mándame el siguiente" 
    i2c_master_read(cmd, datos, 6, I2C_MASTER_ACK); 
    // Lee 6 bytes (seg, min, hrs, dia, fecha, mes) con ACK en cada uno 
 
    // El último byte (año) va con NACK 
    // NACK le dice al DS1307 "ya no quiero más datos, hace quebse detenghma" 
    i2c_master_read_byte(cmd, &datos[6], I2C_MASTER_NACK); 
 
    // STOP — terminamos la transacción 
    i2c_master_stop(cmd); 
 
    // Enviamos todo 
    esp_err_t resultado = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000)); 
    i2c_cmd_link_delete(cmd); 
 
    if (resultado != ESP_OK) { 
        ESP_LOGE(TAG, "Error al leer del DS1307: %s", 
                 esp_err_to_name(resultado)); 
        return; 
    } 

    // Cada byte viene en BCD, lo convertimos a decimal para mostrarlo 
 
    // Debemos enmascarar, si no hacemos esto y CH=1, la conversión BCD daría un valor incorrecto 
    uint8_t segundos = bcd_a_decimal(datos[0] & 0x7F); 
 
    uint8_t minutos  = bcd_a_decimal(datos[1] & 0x7F); 
 
    uint8_t horas    = bcd_a_decimal(datos[2] & 0x3F); 
 
    uint8_t dia      = bcd_a_decimal(datos[3] & 0x07); 
 
    uint8_t fecha    = bcd_a_decimal(datos[4] & 0x3F); 

    uint8_t mes      = bcd_a_decimal(datos[5] & 0x1F); 

    uint8_t anio     = bcd_a_decimal(datos[6]); 
 
    // Imprimimos la hora como LOG en consola serial
    ESP_LOGI(TAG, "Fecha: %02d/%02d/20%02d  Hora: %02d:%02d:%02d  Dia semana: %d", 
             fecha, mes, anio, horas, minutos, segundos, dia); 
} 
 
void app_main(void) { 
 
    // 1. Inicializamos el bus I2C del ESP32 
    i2c_inicializar(); 
 
    // 2. Escribimos la hora una sola vez al arrancar 
    ds1307_escribir_hora(); 
 
    // 3. Leemos y mostramos la hora cada segundo en un ciclo infinito 
    while (1) { 
        ds1307_leer_hora(); 
 
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
} 