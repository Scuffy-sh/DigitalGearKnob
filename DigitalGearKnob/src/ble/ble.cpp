#include "ble.h"

#include <Arduino.h>
#include <cstring>
#include <NimBLEDevice.h>

#include "gears/gears.h"
#include "theme/theme.h"

#include "bno/bno.h"
#include "calibration/calibration.h"

//=====================================================
// UUIDs BLE
//=====================================================

#define SERVICE_UUID "8b7d0001-5d6b-4d5d-a7b5-6d4b4c000001"
#define COMMAND_UUID "8b7d0002-5d6b-4d5d-a7b5-6d4b4c000001"

static NimBLECharacteristic* commandCharacteristic;

//=====================================================
// CALIBRACIÓN
//=====================================================

static bool calibrationPending = false;
static GearPosition calibrationGear;
static uint32_t calibrationStartTime = 0;

static Quaternion lastQuaternion;
static bool quaternionReceived = false;

//=====================================================
// STREAM BNO
//=====================================================

static bool streamEnabled = false;
static uint32_t lastStreamTime = 0;

//=====================================================
// DEBUG MODE
//=====================================================

static bool debugMode = false;

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
        // CALIBRACIÓN
        //=================================================

        if (msg.startsWith("calibrate:"))
        {
            String gearText = msg.substring(10);

            GearPosition gear;

            if (calibration_fromString(gearText, gear))
            {
                Serial.print("Iniciando calibración de ");
                Serial.println(gearText);

                calibrationGear = gear;
                calibrationPending = true;
                calibrationStartTime = millis();
                quaternionReceived = false;

                if (streamEnabled)
                {
                    streamEnabled = false;
                    Serial.println("Stream desactivado (calibración en curso)");

                    static constexpr char streamOff[] = "stream:off";
                    commandCharacteristic->setValue(
                        reinterpret_cast<const uint8_t*>(streamOff),
                        sizeof(streamOff) - 1);
                    commandCharacteristic->notify();
                }
            }
            else
            {
                Serial.println("Marcha no válida");
            }
        }

        //=================================================
        // STREAM
        //=================================================

        if (msg == "stream:on")
        {
            if (calibrationPending)
            {
                Serial.println("Stream rechazado (calibración en curso)");

                static constexpr char rejected[] = "stream:rejected";
                commandCharacteristic->setValue(
                    reinterpret_cast<const uint8_t*>(rejected),
                    sizeof(rejected) - 1);
                commandCharacteristic->notify();
            }
            else
            {
                streamEnabled = true;
                Serial.println("Stream activado");
            }
        }

        if (msg == "stream:off")
        {
            streamEnabled = false;
            Serial.println("Stream desactivado");
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

            // Estado del sensor BNO085
            const char* bnoStatus = bno_is_available() ? "bno:ok" : "bno:error";
            commandCharacteristic->setValue(
                reinterpret_cast<const uint8_t*>(bnoStatus),
                strlen(bnoStatus));
            commandCharacteristic->notify();
            delay(50);

            char themeBuf[20];
            snprintf(themeBuf, sizeof(themeBuf), "theme:#%06lX", (unsigned long)theme_get_primary());

            commandCharacteristic->setValue(
                reinterpret_cast<const uint8_t*>(themeBuf),
                strlen(themeBuf));
            commandCharacteristic->notify();
            delay(50);

            char calStatus[GEAR_COUNT + 1];
            for (uint8_t i = 0; i < GEAR_COUNT; i++)
            {
                calStatus[i] = calibration_is_valid((GearPosition)i) ? '1' : '0';
            }
            calStatus[GEAR_COUNT] = '\0';

            char calBuf[30];
            snprintf(calBuf, sizeof(calBuf), "cal_status:%s", calStatus);

            commandCharacteristic->setValue(
                reinterpret_cast<const uint8_t*>(calBuf),
                strlen(calBuf));
            commandCharacteristic->notify();
            delay(50);

            static const char gearNames[] = "R12345N";

            for (uint8_t i = 0; i < GEAR_COUNT; i++)
            {
                if (!calibration_is_valid((GearPosition)i))
                {
                    continue;
                }

                GearCalibration cal = calibration_get((GearPosition)i);

                char calData[80];
                snprintf(
                    calData,
                    sizeof(calData),
                    "cal_data:%c:%.4f,%.4f,%.4f,%.4f",
                    gearNames[i],
                    cal.w, cal.x, cal.y, cal.z);

                commandCharacteristic->setValue(
                    reinterpret_cast<const uint8_t*>(calData),
                    strlen(calData));
                commandCharacteristic->notify();
                delay(50);
            }

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
    if (calibrationPending)
    {
        Quaternion q;

        // Vamos guardando siempre la última lectura válida
        if (bno_readQuaternion(q))
        {
            lastQuaternion = q;
            quaternionReceived = true;
        }

        // Esperamos aproximadamente 100 ms
        if (millis() - calibrationStartTime < 100)
        {
            return;
        }

        calibrationPending = false;

        if (!quaternionReceived)
        {
            // Intentar una recuperación del sensor antes de fallar
            Serial.println("BNO no respondió, intentando reset para calibración...");
            if (bno_reset())
            {
                // Reintentar una lectura tras el reset
                delay(100);
                if (bno_readQuaternion(q))
                {
                    lastQuaternion = q;
                    quaternionReceived = true;
                }
            }

            if (!quaternionReceived)
            {
                Serial.println("No se recibió ningún quaternion tras recuperación");
                return;
            }
        }

        calibration_save(
            calibrationGear,
            lastQuaternion.w,
            lastQuaternion.x,
            lastQuaternion.y,
            lastQuaternion.z);

        Serial.println("--------------------------------");

        Serial.println("Calibración guardada");
        static constexpr char calibrationOk[] = "calibration_ok";
        commandCharacteristic->setValue(
            reinterpret_cast<const uint8_t*>(calibrationOk),
            sizeof(calibrationOk) - 1);
        commandCharacteristic->notify();

        Serial.printf(
            "W: %.4f  X: %.4f  Y: %.4f  Z: %.4f\n",
            lastQuaternion.w,
            lastQuaternion.x,
            lastQuaternion.y,
            lastQuaternion.z);

        Serial.println("--------------------------------");

        return;
    }

    if (streamEnabled)
    {
        if (millis() - lastStreamTime >= 50)
        {
            lastStreamTime = millis();

            Quaternion q;

            if (bno_readQuaternion(q))
            {
                char buffer[80];

                snprintf(
                    buffer,
                    sizeof(buffer),
                    "quat:%.4f,%.4f,%.4f,%.4f",
                    q.w,
                    q.x,
                    q.y,
                    q.z);

                commandCharacteristic->setValue(
                    reinterpret_cast<const uint8_t*>(buffer),
                    strlen(buffer));
                commandCharacteristic->notify();
            }
        }
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
