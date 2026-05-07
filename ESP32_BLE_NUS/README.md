# ESP32 - Comunicación inalámbrica: Bluetooth Low Energy (BLE) usando servicio NUS

En este ejemplo se muestra cómo implementar una comunicación inalámbrica BLE (Bluetooth Low Energy) en el ESP32 utilizando el stack **NimBLE** incluido en ESP-IDF. Para ello se implementa el servicio **Nordic UART Service (NUS)**, un servicio GATT ampliamente utilizado para emular una comunicación serial tipo UART sobre BLE.

A diferencia del perfil SPP clásico de Bluetooth BR/EDR, BLE sí es soportado por múltiples plataformas modernas como computadores, dispositivos Android e incluso dispositivos Apple (iPhone/iPad), ya que Apple no permite acceso al perfil SPP clásico pero sí permite el uso de servicios GATT BLE estándar o personalizados. Por esta razón, para aplicaciones modernas de comunicación serial inalámbrica se suele utilizar BLE junto con NUS.

El servicio NUS fue originalmente definido por Nordic Semiconductor y establece un conjunto de UUIDs específicos para implementar una comunicación bidireccional tipo UART sobre BLE. En esta implementación se utilizaron exactamente los UUIDs definidos por Nordic para garantizar compatibilidad con aplicaciones y herramientas BLE existentes.

Los UUIDs utilizados son:

* **Servicio NUS**

  * `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`

* **Característica RX (escritura hacia el ESP32)**

  * `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`

* **Característica TX (notificaciones desde el ESP32)**

  * `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`

La característica RX es utilizada por el dispositivo central (smartphone o PC) para enviar datos hacia el ESP32, mientras que la característica TX es utilizada por el ESP32 para enviar notificaciones al dispositivo central.

Es necesario programar el ESP32 con este código luego de configurar las dependencias modificando el archivo *sdkconfig.defaults* y el archivo ubicado en *src/CMakeLists.txt* para habilitar el stack BLE que viene deshabilitado por defecto en PlatformIO.

Una vez programado el ESP32, puede utilizarse la aplicación móvil **nRF Connect** para conectarse al dispositivo BLE. Esta aplicación permite explorar servicios GATT, escribir datos sobre la característica RX y recibir notificaciones desde la característica TX. La aplicación está disponible tanto para Android como para iPhone, además de existir herramientas similares para computadores.

En esta implementación, cuando se envía la letra `'E'` hacia la característica RX del servicio NUS, el ESP32 enciende un LED conectado al GPIO2 y responde mediante una notificación BLE con el texto:

```text
LED encendido
```

Cuando se recibe la letra `'A'`, el LED se apaga y el ESP32 envía:

```text
LED apagado
```

En caso de recibir cualquier otro comando diferente a estas dos letras, el ESP32 responde:

```text
Error: comando desconocido
```

De esta manera se implementa una comunicación inalámbrica bidireccional simple utilizando BLE y servicios GATT.

> *Framework:* ESP-IDF
> *Stack BLE utilizado:* NimBLE
> *Servicio BLE implementado:* Nordic UART Service (NUS)
