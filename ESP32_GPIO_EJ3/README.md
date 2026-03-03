# ESP32: Manejo de pines GPIO

## Ejemplo 3:

Diseñar un sistema de alarma (Z) para una incubadora, que se active cuando dos o más de las siguientes condiciones sea detectada:

- Apertura de puerta (A = 0: Puerta cerrada, A = 1: Puerta abierta).
- Temperatura excesiva (B = 0: Temperatura baja, B = 1: Temperatura alta).
- Humedad relativa alta (C = 0: Humedad baja, C= 1: Humedad alta).

En este ejemplo se muestra la implementación empleando la expresión lógica obtenida de la tabla de verdad.

> *Framework:* ESP-IDF