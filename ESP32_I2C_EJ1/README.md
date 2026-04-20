# ESP32: Protocolo de comunicación I2C

## Ejemplo 1:

1. Configurar la fecha y hora en un *Real-Time Clock* (RTC) **DS1307** mediante escritura en los **registros** internos empleando el protocolo I2C.
2. Leer la fecha y hora cada segundo y mostrarlas en el monitor serial del PC.

En este ejemplo se trabajará con el RTC DS1307, el cual permite comunicación mediante interfaz I2C con un *host*. Se hará la lectura y escritura mediante el manejo de bajo nivel de los registros del periférico, mediante manipulación de bits y codificación BCD.

> *Framework:* ESP-IDF
