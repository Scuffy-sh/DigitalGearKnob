# Indicador de Marchas Digital (Digital Gear Knob)

**Leer en inglés → [README.md](README.md)**

Un indicador digital de marchas para coches con caja manual. Un ESP32-S3 montado en la palanca de cambios lee la orientación 3D de la palanca mediante un IMU BNO085, detecta cuál de las siete posiciones (R, 1–5, N) ha seleccionado el conductor y lo muestra en una pantalla AMOLED con una animación de arco realizada en LVGL. El mismo dispositivo expone un servicio Bluetooth Low Energy que una app complementaria en Flutter ("scuffy") utiliza para depuración en vivo, personalización de tema y calibración.

El problema que resuelve es sencillo: en muchos coches antiguos no existe indicador de marcha, y en los modernos suele estar lejos de la mano del conductor. Este proyecto muestra la marcha justo donde está tu mano, utilizando únicamente fusión de sensores — sin ningún contacto mecánico con la caja de cambios.

## Características principales

- **Detección del patrón H de 7 marchas** (R, 1–5, N) con un único IMU de 9 grados de libertad montado en la palanca.
- **Pantalla AMOLED con animación de arco en LVGL** — la marcha seleccionada se muestra con una animación de "carga" de 2 segundos en lugar de un cambio brusco de valor.
- **Conectividad BLE con una app Flutter** — la app puede conectarse, recibir datos en streaming, cambiar el color de acento y ejecutar la calibración por marcha.
- **Modo de depuración BLE en vivo** — enviando `debug:on` el dispositivo notifica `debug:ROLL,PITCH,GEAR` en cada ciclo de detección, lo que permite ajustar los umbrales en tiempo real.
- **Recaptura automática de la referencia neutral para compensar pendientes** — si la palanca permanece cerca del punto muerto, el cuaternión de referencia se vuelve a capturar, re-centrando todo el sistema de coordenadas cuando cambia la pendiente del coche.
- **Calibración de neutral al arrancar** — la referencia neutral se captura con la primera lectura válida del sensor tras el arranque, sin necesidad de configuración manual.

## Visión general de la arquitectura

```
┌────────────────────────────────────────────────────────────────┐
│                    ESP32-S3 · T-Display S3                     │
│                    C++ / Arduino / PlatformIO                  │
│                                                                │
│  ┌──────────┐   I2C (100 kHz)   ┌─────────────┐                │
│  │  BNO085  │ ─────────────────▶│  gears      │                │
│  │  IMU     │   rotation vector │  (detection │                │
│  └──────────┘   SH2_RV @ 200 Hz │   + zones)  │                │
│                                  └──────┬──────┘               │
│                                         │ LVGL 8.3.11          │
│                                         ▼                      │
│                                  ┌──────────────┐              │
│                                  │ 1.43" AMOLED │              │
│                                  │ arc + gear   │              │
│                                  └──────────────┘              │
│                                         │                      │
│                                    NimBLE "SCUFFY"             │
└────────────────────────────────────────┬───────────────────────┘
                                         │ BLE
                                         ▼
                               ┌────────────────────┐
                               │  Flutter app        │
                               │  "scuffy"           │
                               │  flutter_blue_plus  │
                               │  scan · connect ·   │
                               │  debug · calibrate  │
                               └────────────────────┘
```

En la placa convergen dos flujos de datos: el cuaternión del sensor llega por I2C, la lógica de detección lo convierte en marcha y se renderiza en la AMOLED mediante LVGL; en paralelo, la misma placa ejecuta un servidor GATT NimBLE para que la app del teléfono pueda observar y configurar el dispositivo.

## Componentes

### ESP32-S3 T-Display AMOLED

<img src="images/t-display-s3-amoled.jpg" width="400" alt="ESP32-S3 T-Display AMOLED">

**Tecnologías:** ESP32-S3 (doble núcleo, 240 MHz) · Wi-Fi/BLE · AMOLED de 1.43" · QSPI · LVGL 8.3.11

El cerebro y la pantalla en una sola placa. Esta placa LilyGO T-Display S3 ejecuta todo el firmware — manejo de la fusión del BNO085, detección de marcha, renderizado LVGL y el servidor BLE — y su AMOLED de 1.43" muestra la marcha actual con el arco animado de carga. El firmware también soporta otras pantallas AMOLED de la misma familia de placas; el panel activo se selecciona con un `define` en `include/pin_config.h`.

### IMU BNO085 de 9 grados de libertad

<img src="images/bno085-imu.jpg" width="400" alt="IMU BNO085">

**Tecnologías:** breakout Adafruit BNO085 · acelerómetro + giroscopio + magnetómetro · fusión de sensores integrada (rotation vector) · I2C/SPI · conector STEMMA QT / Qwiic

Un IMU de 9 grados de libertad con un motor de fusión integrado. En lugar de fusionar los datos brutos de acelerómetro/giroscopio/magnetómetro en el MCU, el BNO085 produce internamente un rotation vector (cuaternión) filtrado, lo que resulta más preciso y más sencillo de usar. Está montado en la palanca de cambios para que su orientación refleje la posición de la palanca. El firmware lo lee por I2C a 100 kHz con el reporte de rotation vector habilitado a intervalos de 5 ms, mientras que el bucle de detección lo muestrea a 5 Hz (cada 200 ms) y aplica un control de estabilidad.

### Pomo de cambios

<img src="images/gear-knob.avif" width="400" alt="Pomo de cambios">

El pomo físico que sustituye al original. El IMU va embebido en el interior del pomo, que constituye la integración mecánica de todo el proyecto en el coche: la alimentación y la electrónica viven dentro del pomo, y la pantalla queda orientada hacia el conductor.

#### Ensamblaje

La vista abierta muestra el BNO085 y el ESP32-S3 conectados dentro del pomo antes de pegarlos; el producto final mantiene la pantalla orientada hacia el conductor, mostrando la marcha actual cuando está encendido.

<img src="images/knob-open.jpeg" width="280" alt="Pomo abierto: BNO085 y ESP32-S3 conectados dentro">

<img src="images/knob-final-off.jpeg" width="280" alt="Pomo final, pantalla apagada">

<img src="images/knob-final-on.jpeg" width="280" alt="Pomo final, pantalla encendida">

### App Flutter (scuffy)

<img src="images/flutter-app.jpeg" width="260" alt="App complementaria Flutter">

**Tecnologías:** Flutter · Dart · flutter_blue_plus · permission_handler · shared_preferences

La app cliente BLE complementaria. Escanea el dispositivo (anunciado como `SCUFFY`), se conecta a la característica de comandos y muestra los valores de depuración en vivo (roll, pitch y marcha detectada) en un panel cuando el toggle de DEBUG está activado. También permite cambiar el color de acento de la pantalla y ejecutar la calibración por marcha desde el teléfono.

## Cómo funciona

### Algoritmo de detección

1. **Captura de la referencia neutral.** Con la primera lectura válida del sensor tras el arranque (la palanca está en N en ese momento), el cuaternión bruto se almacena como `q_neutral`.
2. **Cálculo de la rotación relativa.** Cada nueva muestra se expresa en relación con la neutral: `q_rel = conj(q_neutral) * q_current`. Esto elimina la orientación absoluta de montaje y deja únicamente el movimiento de la palanca.
3. **Extracción de ángulos de inclinación.** `q_rel` se descompone en dos ángulos en grados: **pitch** (inclinación adelante/atrás) y **roll** (inclinación izquierda/derecha). Esto separa las columnas y filas del patrón H.
4. **Mapeo a zonas 2D.** El patrón H se divide según el pitch: las marchas delanteras tienen `pitch < 0`, y la marcha atrás y las pares tienen `pitch > 0`. El roll separa entonces R / 1 / 3 / 5 en la fila delantera y 4 / 2 en la fila trasera. El punto muerto es una pequeña zona muerta alrededor del centro.
5. **Anti-rebote con contador de estabilidad.** Un cambio de marcha solo se acepta tras **3 detecciones idénticas consecutivas** (la detección se ejecuta cada 200 ms), lo que descarta lecturas transitorias mientras la palanca está a medio camino.
6. **Animación del arco.** Al aceptarse, se reproduce una animación de arco LVGL de 2 segundos con easing hacia el valor objetivo, mientras la etiqueta muestra la marcha destino durante todo el proceso — nunca cuenta marchas intermedias al bajar (sin el parpadeo N-5-4-3-2-1).

### Recaptura automática de neutral (compensación de pendientes)

El BNO085 mide la orientación absoluta contra la gravedad. Si la pendiente del coche cambia, la referencia neutral capturada al arrancar ya no coincide con la posición real de la palanca y todas las marchas se leen mal.

La solución es volver a capturar la referencia en tiempo de ejecución: cuando la palanca permanece **dentro de ±8° del punto muerto durante ~1 s**, el firmware recaptura `q_neutral` como la media normalizada de las muestras tomadas en esa ventana. Como las zonas se definen como desplazamientos relativos a N, re-centrar N re-centra todo el sistema.

Existe una **puerta de seguridad crítica**: la recaptura solo se activa cuando la marcha *detectada* es N o ambigua. Sin ella, la 5ª marcha (pitch −9.2°) entraría en la ventana de ±8° con una pendiente modesta y corrompería la referencia neutral mientras se conduce.

**Limitación conocida.** Con un único IMU en la palanca, los desniveles sostenidos de más de ~3° no pueden distinguirse por completo de una posición real de marcha — una pendiente de esa magnitud desplaza todas las zonas en la misma cantidad. La solución definitiva es un segundo IMU montado en la carrocería, usando su orientación como referencia en lugar de una captura fija.

## Stack tecnológico

| Capa | Tecnología |
| --- | --- |
| Firmware | C++ · framework Arduino · PlatformIO |
| MCU | ESP32-S3 (LilyGO T-Display S3), doble núcleo 240 MHz |
| Sensor | Adafruit BNO085 (rotation vector integrado) · I2C 100 kHz |
| Pantalla | AMOLED 1.43" · LVGL 8.3.11 · GFX Library for Arduino 1.4.9 |
| BLE (firmware) | NimBLE-Arduino ^1.4.1 |
| App | Flutter · Dart |
| BLE (app) | flutter_blue_plus ^1.35.5 |
| Soporte app | permission_handler ^12.0.1 · flutter_colorpicker ^1.1.0 · shared_preferences ^2.2.0 |

## Estructura del proyecto

```
ChatGPT/
├── DigitalGearKnob/            # Firmware ESP32-S3 (PlatformIO)
│   ├── platformio.ini          # envs: lilygo-t-display-s3, native (tests host)
│   ├── include/
│   │   ├── pin_config.h        # Mapeo de pines + selección de panel AMOLED
│   │   └── lgfx_user.hpp       # Configuración LGFX
│   ├── src/
│   │   ├── main.cpp            # Secuencia de arranque + bucle principal
│   │   ├── display/            # Init de la AMOLED QSPI (GFX library)
│   │   ├── lvgl_port/          # Integración LVGL ↔ pantalla
│   │   ├── ui/                 # Pantallas LVGL generadas con SquareLine (arco + etiqueta)
│   │   ├── boot/               # Splash de logo + fundido a la pantalla de marcha
│   │   ├── theme/              # Color de acento / theming
│   │   ├── bno/                # Driver BNO085 + recuperación del bus I2C
│   │   ├── gears/              # Detección de marcha, recaptura de neutral, animación
│   │   ├── ble/                # Servidor GATT NimBLE + modo debug
│   │   └── calibration/        # Enum de marchas + persistencia de calibración (NVS)
│   └── test/test_calibration/  # Tests Unity en host para el módulo de calibración
├── scuffy/                     # App complementaria Flutter
│   └── lib/
│       ├── services/ble_service.dart   # Singleton cliente BLE (scan, notify, comandos)
│       ├── screens/                    # home, calibración de marchas, splash
│       ├── widgets/                    # tarjeta de marcha, tarjeta de tema, estado UI
│       ├── models/                     # marcha, cuaternión, tema, estado de calibración
│       └── data/                       # presets de tema
├── images/                         # Imágenes del producto usadas en este README
```

## Primeros pasos

### Firmware (ESP32-S3)

El entorno principal está definido en `platformio.ini` como `lilygo-t-display-s3` (framework Arduino). Con PlatformIO instalado:

```bash
cd DigitalGearKnob
pio run -e lilygo-t-display-s3 -t upload
pio device monitor -b 115200
```

El módulo de calibración también compila como entorno nativo de host para tests unitarios:

```bash
pio test -e native
```

### App Flutter

```bash
cd scuffy
flutter pub get
flutter run
```

La app escanea el dispositivo anunciado como `SCUFFY` y se conecta a él. Para activar la depuración en vivo, pulsa el toggle **DEBUG: OFF**: la app escribe `debug:on` en la característica de comandos BLE y el firmware comienza a notificar `debug:ROLL,PITCH,GEAR` una vez por ciclo de detección (~5 Hz). Los valores aparecen en el panel de depuración. Pulsar el toggle de nuevo envía `debug:off` y detiene el stream.

## Calibración

Las zonas de detección actuales se calibraron con mediciones reales tomadas dentro del coche, y los umbrales de zona de `gears.cpp` provienen directamente de esas mediciones:

| Marcha | Zona (relativa al punto muerto, en grados) |
| --- | --- |
| N | `|roll| < 2.5` y `|pitch| < 2.5` |
| R | `pitch < -3` y `roll > 18` |
| 1 | `pitch < -3` y `12 < roll <= 18` |
| 3 | `pitch < -3` y `7 < roll <= 12` |
| 5 | `pitch < -3` y `roll <= 7` |
| 4 | `pitch > 3` y `roll < -7.5` |
| 2 | `pitch > 3` y `roll >= -7.5` |

Puntos de anclaje medidos en el coche: R (20.4, −4.7), 1 (13.6, −9.7), 3 (10.0, −13.5), 5 (1.6, −9.2), N (0.9, 0.9), 2 (−5.1, 14.8), 4 (−9.6, 11.1) — mostrados como `(roll, pitch)`.

## Hoja de ruta

- **Notificación BLE dedicada `gear:X`** para que la app del teléfono muestre la marcha actual sin depuración ni sondeo.
- **Asistente de calibración guiado en la app** — un flujo estructurado alrededor de la captura por marcha existente.
- **Segundo IMU en la carrocería** para inmunidad total a las pendientes, eliminando por completo la limitación de ~3°.
- **Tests unitarios en host para la lógica de zonas** — extender el arnés de pruebas nativo de la calibración a la detección 2D de marchas.

## Licencia

Este proyecto está bajo la [Licencia MIT](LICENSE).
