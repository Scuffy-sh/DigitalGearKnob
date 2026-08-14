#include "ble.h"

#include <Arduino.h>
#include <cstring>
#include <NimBLEDevice.h>

#include "ble/ble_format.h"
#include "can/can.h"
#include "can/sniff.h"
#include "gears/gears.h"
#include "theme/theme.h"

//=====================================================
// UUIDs BLE
//=====================================================

#define SERVICE_UUID "8b7d0001-5d6b-4d5d-a7b5-6d4b4c000001"
#define COMMAND_UUID "8b7d0002-5d6b-4d5d-a7b5-6d4b4c000001"

static NimBLECharacteristic* commandCharacteristic;

//=====================================================
// DEBUG MODE
//=====================================================

static bool debugMode = false;

//=====================================================
// NOTIFICACIONES
//=====================================================

/// Envía una línea de texto por la característica de comando.
static void notifyLine(const char *line)
{
    if (commandCharacteristic == nullptr)
    {
        return;
    }

    commandCharacteristic->setValue(
        reinterpret_cast<const uint8_t*>(line),
        strlen(line));
    commandCharacteristic->notify();
}

//=====================================================
// RECHAZO DE COMANDOS REMOVIDOS (protocolo v2)
//=====================================================

/// Notifica `error:removed:<cmd>` para flujos que ya no existen
/// (spec ble-protocol: los comandos removidos deben rechazarse
/// explícitamente para que un cliente desactualizado no falle).
static void rejectRemovedCommand(const char *cmd)
{
    char buffer[48];
    snprintf(buffer, sizeof(buffer), "error:removed:%s", cmd);
    notifyLine(buffer);
}

//=====================================================
// CALLBACK RECEPCION
//=====================================================

class ServerCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic* pCharacteristic) override
    {
        std::string value = pCharacteristic->getValue();

        if (value.empty()) return;

        Serial.print("BLE RX: ");
        Serial.println(value.c_str());

        String msg = String(value.c_str());

        //=================================================
        // COLOR
        //=================================================

        if (msg.indexOf("set_color") >= 0)
        {
            int start = msg.indexOf("#");

            if (start > 0)
            {
                String color = msg.substring(start + 1, start + 7);

                Serial.print("Color recibido: #");
                Serial.println(color);

                uint32_t rgb = strtoul(color.c_str(), nullptr, 16);

                Serial.print("RGB: 0x");
                Serial.println(rgb, HEX);

                theme_set_primary(rgb);
                theme_save();
            }
        }

        //=================================================
        // TEMA
        //=================================================

        if (msg.indexOf("set_theme") >= 0)
        {
            if (msg.indexOf("r") >= 0)
            {
                Serial.println("Tema R");

                // Futuro:
                // theme_set(...)
            }
        }

        //=================================================
        // OTA
        //=================================================

        if (msg.indexOf("ota") >= 0)
        {
            Serial.println("OTA request");

            // Futuro:
            // ota_start();
        }

        //=================================================
        // MARCHA MANUAL (protocolo "gear:R" / "gear:1".."gear:5" / "gear:N")
        //=================================================

        if (msg.startsWith("gear:"))
        {
            char c = msg.charAt(5);
            int8_t gear = -1;

            if (c == 'R' || c == 'r') gear = 0;
            else if (c >= '1' && c <= '5') gear = c - '0';
            else if (c == 'N' || c == 'n') gear = 6;

            if (gear >= 0)
            {
                gears_set(static_cast<uint8_t>(gear));
            }
            else
            {
                Serial.print("Gear inválido: ");
                Serial.println(c);
            }
        }
        
        //=================================================
        // COMANDOS REMOVIDOS (protocolo v2 — spec ble-protocol)
        // Los flujos acoplados al BNO085 (calibrate:, stream:on/off y las
        // notificaciones quat:) ya no existen. Un cliente desactualizado
        // recibe una rechazo explícito para no malinterpretar respuestas.
        //=================================================

        if (msg.startsWith("calibrate:"))
        {
            Serial.println("calibrate: removido (v2)");
            rejectRemovedCommand("calibrate");
        }

        if (msg == "stream:on" || msg == "stream:off")
        {
            Serial.println("stream: removido (v2)");
            rejectRemovedCommand("stream");
        }

        //=================================================
        // DEBUG
        //=================================================

        if (msg == "debug:on")
        {
            debugMode = true;
            Serial.println("Debug mode activado");
        }

        if (msg == "debug:off")
        {
            debugMode = false;
            Serial.println("Debug mode desactivado");
        }

        //=================================================
        // SNIFF (protocolo v2 — design D6)
        // Activa/desactiva el volcado de tramas CAN. Las líneas
        // `sniff:<E|S><8hexid>:<datahex>` se notifican desde ble_update()
        // drenando la cola del módulo can.
        //=================================================

        if (msg == "sniff:on")
        {
            sniff_set_enabled(true);
            Serial.println("Sniff activado");
        }

        if (msg == "sniff:off")
        {
            sniff_set_enabled(false);
            Serial.println("Sniff desactivado");
        }

        //=================================================
        // GET STATE (sync estado a la app)
        //=================================================

        if (msg == "get_state")
        {
            Serial.println("Estado solicitado por la app");

            // v2 (design data flow get_state): can:ok|offline, theme:#...,
            // proto:2 — los flujos bno:/cal_status:/cal_data: fueron removidos.
            char buf[32];

            snprintf(buf, sizeof(buf), "%s", can_is_online() ? "can:ok" : "can:offline");
            notifyLine(buf);
            delay(50);

            snprintf(buf, sizeof(buf), "theme:#%06lX", (unsigned long)theme_get_primary());
            notifyLine(buf);
            delay(50);

            snprintf(buf, sizeof(buf), "proto:2");
            notifyLine(buf);
            delay(50);

            Serial.println("Estado enviado a la app");
        }
    }
};

//=====================================================
// INIT BLE
//=====================================================

void ble_init()
{
    Serial.println("Iniciando BLE...");

    NimBLEDevice::init("SCUFFY");

    NimBLEServer* server = NimBLEDevice::createServer();

    NimBLEService* service = server->createService(SERVICE_UUID);

    commandCharacteristic = service->createCharacteristic(
        COMMAND_UUID,
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::NOTIFY
    );

    commandCharacteristic->setCallbacks(new ServerCallbacks());

    service->start();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();

    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->start();

    Serial.println("BLE iniciado correctamente");
    Serial.println("Nombre: SCUFFY");
}

//=====================================================
// UPDATE
//=====================================================

void ble_update()
{
    // v2 (design D6): el módulo can enqueuea líneas ya formateadas
    // `sniff:<E|S><8hexid>:<datahex>` (limitadas a 20 Hz por el limiter) y
    // `can:no_frames` (watchdog de bus silencioso). Este loop las drena y
    // las notifica; toda la BLE notify queda acá, en el loop principal.
    if (commandCharacteristic == nullptr)
    {
        return;
    }

    char line[CAN_SNIFF_TEXT_MAX];

    while (can_sniff_take(line, sizeof(line)))
    {
        notifyLine(line);
        Serial.println(line);
    }
}

//=====================================================
// DEBUG API
//=====================================================

void ble_set_debug(bool enabled)
{
    debugMode = enabled;
}

bool ble_is_debug()
{
    return debugMode;
}

void ble_send_debug(int8_t gear, float rpm, float speed, bool can_online)
{
    if (!debugMode || !commandCharacteristic) return;

    // v2 (design data flow debug): debug:<GEAR>,<RPM>,<SPEED>,<CANSTAT>
    char buffer[64];

    if (!ble_format_debug(buffer, sizeof(buffer), gear, rpm, speed, can_online))
    {
        return;
    }

    notifyLine(buffer);
}
