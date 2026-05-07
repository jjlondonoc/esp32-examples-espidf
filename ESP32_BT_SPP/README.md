# ESP32 - Comunicación inalámbrica: Bluetooth Classic usando perfil SPP

En este ejemplo se muestra cómo configurar el perfil **SPP (Serial Port Profile)** sobre Bluetooth Classic (BR/EDR) en el ESP32 utilizando el stack **Bluedroid** incluido en ESP-IDF. El perfil SPP permite crear un canal de comunicación serial inalámbrico que emula una UART convencional mediante el protocolo RFCOMM.

Gracias a esto, el ESP32 puede comportarse como un puerto COM virtual inalámbrico, permitiendo enviar y recibir datos de manera similar a una comunicación serial tradicional, pero utilizando Bluetooth Classic.

Es necesario programar el ESP32 con este código luego de configurar las dependencias modificando el archivo *sdkconfig.defaults* y el archivo ubicado en *src/CMakeLists.txt* para habilitar el stack Bluetooth Classic, el cual viene deshabilitado por defecto en PlatformIO.

Una vez programado el ESP32, el dispositivo puede emparejarse con un computador o con un smartphone Android para establecer la comunicación SPP. En computadores normalmente aparecerá como un puerto COM virtual, mientras que en Android pueden utilizarse aplicaciones compatibles con Bluetooth SPP para enviar y recibir datos.

Es importante tener en cuenta que Apple no permite acceso al perfil SPP clásico en dispositivos iPhone/iPad. Por esta razón, cuando se desea compatibilidad con dispositivos Apple, normalmente se utiliza Bluetooth Low Energy (BLE) junto con servicios GATT específicos como el servicio NUS (Nordic UART Service).

En esta implementación, cuando se envía por el puerto serial Bluetooth la letra `'E'`, el ESP32 enciende un LED conectado al GPIO26 y responde enviando un mensaje indicando el estado del LED:

```text id="z8o5ci"
LED encendido
```

Cuando se recibe la letra `'A'`, el LED se apaga y el ESP32 responde:

```text id="9x9sq2"
LED apagado
```

En caso de recibir cualquier otro comando diferente a estas dos letras, el ESP32 enviará:

```text id="7kp8mx"
Comando desconocido
```

De esta manera se implementa una comunicación serial inalámbrica simple utilizando Bluetooth Classic y el perfil SPP.

> *Framework:* ESP-IDF
> *Stack Bluetooth utilizado:* Bluedroid
> *Perfil Bluetooth implementado:* SPP (Serial Port Profile)
