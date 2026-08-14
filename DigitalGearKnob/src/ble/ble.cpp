#include "ble.h"

#include <Arduino.h>
#include <cstring>
#include <NimBLEDevice.h>

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
// RECHAZO DE COMANDOS REMOVIDOS (protocolo v2)
//=====================================================

/// Notifica `error:removed:<cmd>` para flujos que ya no existen
/// (spec ble-protocol: los comandos removidos deben rechazarse
/// explícitamente para que un cliente desactualizado no falle).
static void rejectRemovedCommand(const char *cmd)
{
    if (commandCharacteristic == nullptr)
    {
        return;
    }

    char buffer[48];
    snprintf(buffer, sizeof(buffer), "error:removed:%s", cmd);

    commandCharacteristic->setValue(
        reinterpret_cast<const uint8_t*>(buffer),
        strlen(buffer));
    commandCharacteristic->notify();
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
        // GET STATE (sync estado a la app)
        //=================================================

        if (msg == "get_state")
        {
            Serial.println("Estado solicitado por la app");

            // v2: sin bno:/cal_status:/cal_data: (flujos BNO removidos).
            // Phase 5 añade can:ok|offline + proto:2 aquí.
            char themeBuf[20];
            snprintf(themeBuf, sizeof(themeBuf), "theme:#%06lX", (unsigned long)theme_get_primary());

            commandCharacteristic->setValue(
                reinterpret_cast<const uint8_t*>(themeBuf),
                strlen(themeBuf));
            commandCharacteristic->notify();
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
    // v2: todos los flujos acoplados al BNO085 (captura de calibración,
    // stream de quaternions) fueron removidos en Phase 4. Phase 5 añade
    // aquí el drenaje de la cola de sniff y las notificaciones can:*.
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

void ble_send_debug(float roll, float pitch, int8_t gear)
{
    if (!debugMode || !commandCharacteristic) return;

    static const char* gearNames[] = {"R", "1", "2", "3", "4", "5", "N"};

    char buffer[64];
    const char* gearName = (gear >= 0 && gear <= 6) ? gearNames[gear] : "??";
    snprintf(buffer, sizeof(buffer), "debug:%.1f,%.1f,%s", roll, pitch, gearName);

    commandCharacteristic->setValue(
        reinterpret_cast<const uint8_t*>(buffer),
        strlen(buffer));
    commandCharacteristic->notify();
}
