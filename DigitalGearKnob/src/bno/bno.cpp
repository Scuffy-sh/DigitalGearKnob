#include "bno.h"

#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include "pin_config.h"

//=====================================================
// SENSOR
//=====================================================

static Adafruit_BNO08x bno08x;
static bool bno_available = false;

//=====================================================
// CONTADOR DE ERRORES CONSECUTIVOS
//=====================================================

static uint8_t consecutiveErrors = 0;
static const uint8_t MAX_CONSECUTIVE_ERRORS = 5;

static uint8_t resetAttempts = 0;
static const uint8_t MAX_RESET_ATTEMPTS = 3;
static uint32_t cooldownUntil = 0;

//=====================================================
// RECUPERACIÓN DEL BUS I2C
//=====================================================

/// Genera pulsos de reloj en SCL para liberar SDA si el esclavo
/// lo tiene bloqueado en bajo (lock-up clásico del BNO085).
static void i2c_busRecover()
{
    pinMode(IIC_SCL, OUTPUT_OPEN_DRAIN);
    pinMode(IIC_SDA, INPUT_PULLUP);

    // Generar hasta 9 pulsos de reloj para liberar SDA
    for (int i = 0; i < 9; i++)
    {
        digitalWrite(IIC_SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(IIC_SCL, HIGH);
        delayMicroseconds(5);
    }

    // Generar condición STOP: SDA de LOW a HIGH mientras SCL está HIGH
    pinMode(IIC_SDA, OUTPUT_OPEN_DRAIN);
    digitalWrite(IIC_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(IIC_SDA, HIGH);
    delayMicroseconds(5);

    // Restaurar pines a modo I2C
    pinMode(IIC_SDA, INPUT_PULLUP);
    pinMode(IIC_SCL, INPUT_PULLUP);
}

//=====================================================
// INIT
//=====================================================

bool bno_init()
{
    Wire.begin(IIC_SDA, IIC_SCL);
    Wire.setClock(100000);

    if (!bno08x.begin_I2C())
    {
        Serial.println("ERROR: No se ha encontrado el BNO085");
        bno_available = false;
        return false;
    }

    if (!bno08x.enableReport(SH2_ROTATION_VECTOR, 5000))
    {
        Serial.println("ERROR: No se pudo habilitar Rotation Vector");
        bno_available = false;
        return false;
    }

    Serial.println("BNO085 inicializado correctamente");
    bno_available = true;
    consecutiveErrors = 0;

    return true;
}

//=====================================================
// RESET / RECUPERACIÓN
//=====================================================

bool bno_reset()
{
    if (!bno_available) return false;

    if (millis() < cooldownUntil)
    {
        return false;
    }

    if (resetAttempts >= MAX_RESET_ATTEMPTS)
    {
        Serial.println("BNO085: máximos reintentos de reset alcanzados, cooldown 5s");
        cooldownUntil = millis() + 5000;
        resetAttempts = 0;
        return false;
    }

    resetAttempts++;
    Serial.printf("BNO085: intentando recuperación (intento %d/%d)...\n", resetAttempts, MAX_RESET_ATTEMPTS);

    i2c_busRecover();
    delay(50);

    Wire.end();
    delay(100);
    Wire.begin(IIC_SDA, IIC_SCL);
    Wire.setClock(100000);
    delay(100);

    // Full reinit — verifies chip ID, not just sends a command
    if (!bno08x.begin_I2C(BNO08x_I2CADDR_DEFAULT, &Wire))
    {
        Serial.println("BNO085: fallo al reiniciar comunicación I2C");
        return false;
    }

    if (!bno08x.enableReport(SH2_ROTATION_VECTOR, 5000))
    {
        Serial.println("BNO085: fallo al re-habilitar reporte");
        return false;
    }

    consecutiveErrors = 0;
    resetAttempts = 0;
    Serial.println("BNO085: recuperado correctamente");
    return true;
}

//=====================================================
// LEER QUATERNION
//=====================================================

bool bno_readQuaternion(Quaternion &quat)
{
    if (!bno_available) return false;

    sh2_SensorValue_t sensorValue;

    if (!bno08x.getSensorEvent(&sensorValue))
    {
        consecutiveErrors++;

        if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
        {
            Serial.printf("BNO085: %d errores consecutivos, intentando reset...\n", consecutiveErrors);
            bno_reset();
        }

        return false;
    }

    if (sensorValue.sensorId != SH2_ROTATION_VECTOR)
    {
        return false;
    }

    consecutiveErrors = 0;
    resetAttempts = 0;

    quat.w = sensorValue.un.rotationVector.real;
    quat.x = sensorValue.un.rotationVector.i;
    quat.y = sensorValue.un.rotationVector.j;
    quat.z = sensorValue.un.rotationVector.k;

    return true;
}

//=====================================================
// ESTADO DEL SENSOR
//=====================================================

bool bno_is_available()
{
    return bno_available;
}