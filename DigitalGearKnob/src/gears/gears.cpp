#include "gears.h"

#include <Arduino.h>
#include <math.h>
#include <lvgl.h>
#include "ui/ui.h"
#include "bno/bno.h"
#include "calibration/calibration.h"
#include "ble/ble.h"

//=====================================================
// ESTADO
//=====================================================

static uint8_t current_gear = 0;   // 0 = R
static uint32_t last_change = 0;

//=====================================================
// LÓGICA DE UI / ANIMACIÓN ARC
//=====================================================

static int last_displayed_gear = -1;
static int8_t anim_target_gear = -1;  // marcha destino de la animación

static void arc_anim_cb(void *var, int32_t v)
{
    lv_arc_set_value(ui_CargaMarchas, v);

    // La etiqueta muestra SIEMPRE la marcha destino durante la
    // animación; el arco solo es un efecto visual de "carga" y
    // no cuenta marchas intermedias (evita N-5-4-3-2-1 al bajar).
    int gear = anim_target_gear;

    if (gear < 0)
    {
        // Fallback defensivo (no debería ocurrir): mapeo por valor.
        if (v < 20) gear = 0;
        else if (v < 40) gear = 1;
        else if (v < 60) gear = 2;
        else if (v < 80) gear = 3;
        else if (v < 95) gear = 4;
        else gear = 5;
    }

    if (gear != last_displayed_gear)
    {
        last_displayed_gear = gear;

        char txt[4];

        if (gear == 0)
        {
            strcpy(txt, "R");
        }
        else if (gear == 6)
        {
            strcpy(txt, "N");
        }
        else
        {
            sprintf(txt, "%d", gear);
        }

        lv_label_set_text(ui_Marchas, txt);
    }
}

//=====================================================
// ANIMACIÓN HACIA NUEVA MARCHA
//=====================================================

static void animate_to_gear(uint8_t gear)
{
    if (!ui_CargaMarchas) return;

    int target;

    if (gear == 0) target = 0;
    else if (gear == 6) target = 100;  // N = arco completo (como el arranque)
    else target = gear * 20;

    anim_target_gear = gear;

    lv_anim_t a;
    lv_anim_init(&a);

    lv_anim_set_var(&a, ui_CargaMarchas);

    lv_anim_set_exec_cb(&a, arc_anim_cb);

    lv_anim_set_values(
        &a,
        lv_arc_get_value(ui_CargaMarchas),
        target);

    lv_anim_set_time(&a, 2000);

    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_start(&a);
}

//=====================================================
// MATEMÁTICAS DE CUATERNION
//=====================================================

/// Conjugado de un cuaternion (inversa para unit quaternions).
static Quaternion q_conjugate(Quaternion q)
{
    return { q.w, -q.x, -q.y, -q.z };
}

/// Multiplicación de cuaternions: a * b
static Quaternion q_multiply(Quaternion a, Quaternion b)
{
    return {
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
    };
}

/// Normaliza un cuaternion (lo deja con módulo 1).
static Quaternion q_normalize(Quaternion q)
{
    float mag = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (mag < 1e-6f) return {1, 0, 0, 0};
    return { q.w / mag, q.x / mag, q.y / mag, q.z / mag };
}

//=====================================================
// DETECCIÓN POR ZONAS 2D (PITCH + ROLL)
//=====================================================
//
// El cuaternion relativo a la neutral se descompone en ángulos
// de inclinación de la palanca:
//   pitch = inclinación adelante/atrás
//   roll  = inclinación izquierda/derecha
//
// Mapa del H-pattern (medidas reales del coche):
//
//          Adelante (pitch < 0)
//   R(20.4,-4.7)  1(13.6,-9.7)  3(10.0,-13.5)  5(1.6,-9.2)
//
//   ---------- N (0.9,0.9) ----------
//
//          Atrás (pitch > 0)
//   2(-5.1,14.8)  4(-9.6,11.1)
//
// Zonas definidas por los datos medidos reales.
//=====================================================

// Intervalo de detección (ms)
static const uint32_t DETECTION_INTERVAL_MS = 200;

// Zona muerta alrededor de N (grados): ambos valores deben estar
// dentro de este rango para marcar N. Se mantiene estricta para que
// ninguna marcha cercana (como 5, con pitch -9.2) caiga en N.
static const float NEUTRAL_ZONE = 2.5f;

// Umbral de stabilización: cuántos frames consecutivos
// deben coincidir para aceptar un cambio de marcha.
static const uint8_t STABILITY_FRAMES = 3;

static uint32_t last_detection_time = 0;
static uint8_t stability_counter = 0;
static int8_t pending_gear = -1;

//=====================================================
// REFERENCIA NEUTRAL (capturada al arrancar)
//=====================================================

static Quaternion neutral_quat;
static bool neutral_captured = false;

//=====================================================
// AUTO-RECAPTURA DE NEUTRAL
//=====================================================
//
// El BNO085 mide orientación absoluta (contra la gravedad).
// Cuando el coche cambia de pendiente, la referencia neutral
// capturada al boot queda corrida y todas las marchas se
// leen mal. Solución: cuando la palanca está quieta cerca
// de N durante ~1s, re-capturar la referencia. Como las
// zonas son desplazamientos relativos a N, re-centrar N
// re-centra todo el sistema de coordenadas.
//=====================================================

// Ventana (grados) para considerar que la palanca está "en N".
// Ninguna marcha medida cae dentro de ±8° (la más cercana es 5
// con pitch -9.2), así que es seguro como gatillo de recaptura.
static const float NEUTRAL_RECAPTURE_WINDOW = 8.0f;

// Tiempo estable cerca de N para re-capturar (ms)
static const uint32_t NEUTRAL_RECAPTURE_MS = 1000;

static Quaternion neutral_sum = {0, 0, 0, 0};
static uint8_t neutral_sample_count = 0;
static uint32_t neutral_stable_start = 0;
static bool neutral_recaptured = false;

//=====================================================
// EXTRAER PITCH Y ROLL DEL CUATERNION RELATIVO
//=====================================================

/// Extrae pitch y roll (en grados) de un cuaternion relativo.
/// pitch = inclinación adelante/atrás (>0 = atrás con estos datos)
/// roll  = inclinación izquierda/derecha (>0 = izquierda/adelante)
static void extract_angles(Quaternion rel, float &roll, float &pitch)
{
    float w = rel.w, x = rel.x, y = rel.y, z = rel.z;

    // Roll (rotación alrededor del eje X del sensor)
    roll = atan2f(2.0f * (w * x + y * z),
                  1.0f - 2.0f * (x * x + y * y)) * 180.0f / M_PI;

    // Pitch (rotación alrededor del eje Y del sensor)
    float sinArg = 2.0f * (w * y - x * z);
    sinArg = fmaxf(-1.0f, fminf(1.0f, sinArg));
    pitch = asinf(sinArg) * 180.0f / M_PI;
}

//=====================================================
// DETECCIÓN POR ZONAS
//=====================================================

/// Determina la marcha a partir de los ángulos pitch/roll.
/// Devuelve -1 si no se detecta ninguna marcha.
///
/// Zonas calibradas con medidas reales del coche:
///   N:  roll 0.9,  pitch 0.9
///   R:  roll 20.4, pitch -4.7
///   1:  roll 13.6, pitch -9.7
///   3:  roll 10.0, pitch -13.5
///   5:  roll 1.6,  pitch -9.2
///   2:  roll -5.1, pitch 14.8
///   4:  roll -9.6, pitch 11.1
///
/// Las marchas delanteras (1, 3, 5) tienen pitch < 0;
/// las traseras (2, 4) tienen pitch > 0. El roll separa
/// R (>18), 1 (12-18), 3 (7-12) y 5 (<7) en delanteras, y
/// 4 (< -7.5) de 2 (> -7.5) en traseras.
static int8_t detect_gear_from_angles(float roll, float pitch)
{
    // ── NEUTRAL: ambos valores cerca del centro ──
    if (fabsf(roll) < NEUTRAL_ZONE && fabsf(pitch) < NEUTRAL_ZONE)
    {
        return GEAR_N;
    }

    // ── MARCHAS ADELANTE (pitch negativo) ──
    if (pitch < -3.0f)
    {
        if (roll > 18.0f) return GEAR_R;
        if (roll > 12.0f) return GEAR_1;
        if (roll > 7.0f)  return GEAR_3;
        return GEAR_5;
    }

    // ── MARCHAS ATRÁS (pitch positivo) ──
    if (pitch > 3.0f)
    {
        if (roll < -7.5f) return GEAR_4;
        return GEAR_2;
    }

    // No se detectó claramente
    return -1;
}

//=====================================================
// API PUBLICA
//=====================================================

void gears_init()
{
    current_gear = 6;  // Neutral
    last_change = millis();

    if (ui_CargaMarchas)
    {
        lv_arc_set_range(ui_CargaMarchas, 0, 100);
        lv_arc_set_value(ui_CargaMarchas, 100);
    }

    if (ui_Marchas)
    {
        lv_label_set_text(ui_Marchas, "N");
    }

    last_displayed_gear = 6;
}

void gears_set(uint8_t gear)
{
    current_gear = gear;
    last_change = millis();

    animate_to_gear(gear);

    Serial.print("Gear changed to: ");
    Serial.println(gear);
}

uint8_t gears_get_current()
{
    return current_gear;
}

void gears_update()
{
    if (millis() - last_detection_time < DETECTION_INTERVAL_MS)
    {
        return;
    }
    last_detection_time = millis();

    Quaternion current;
    if (!bno_readQuaternion(current))
    {
        return;
    }

    // ── Capturar neutral en la primera lectura ──
    if (!neutral_captured)
    {
        neutral_quat = current;
        neutral_captured = true;
        Serial.println("Neutral capturada como referencia");
        return;
    }

    // ── Calcular rotación relativa a neutral ──
    Quaternion rel = q_multiply(q_conjugate(neutral_quat), current);

    // ── Extraer ángulos ──
    float roll, pitch;
    extract_angles(rel, roll, pitch);

    // ── Detectar marcha por zona ──
    int8_t detected = detect_gear_from_angles(roll, pitch);

    // ── Auto-recaptura de neutral ──
    // Si la palanca está quieta cerca de N durante ~1s, re-centrar la
    // referencia. Las zonas son desplazamientos relativos a N, así que
    // re-centrar N compensa cambios de pendiente del coche en marcha.
    //
    // SEGURIDAD CRÍTICA: además de la ventana de ±8°, se exige que la
    // marcha DETECTADA sea N o -1 (zona ambigua). Sin esta condición,
    // la 5ª (pitch -9.2) entra en la ventana con solo ~1.3° de
    // pendiente y la recaptura corrompería el neutral sobre la marcha.
    // Con el gate por detección, estar en cualquier marcha real
    // (autopista en 5ª, por ejemplo) bloquea la recaptura.
    bool near_neutral =
        fabsf(roll) < NEUTRAL_RECAPTURE_WINDOW &&
        fabsf(pitch) < NEUTRAL_RECAPTURE_WINDOW;
    bool safe_to_recapture = (detected == GEAR_N || detected == -1);

    if (near_neutral && safe_to_recapture)
    {
        if (neutral_stable_start == 0)
        {
            neutral_stable_start = millis();
            neutral_sum = {0, 0, 0, 0};
            neutral_sample_count = 0;
        }

        // Evitar que el signo del cuaternion (q vs -q) cancele el promedio.
        if (neutral_sample_count > 0 &&
            (neutral_sum.w * current.w + neutral_sum.x * current.x +
             neutral_sum.y * current.y + neutral_sum.z * current.z) < 0)
        {
            current.w = -current.w;
            current.x = -current.x;
            current.y = -current.y;
            current.z = -current.z;
        }

        neutral_sum.w += current.w;
        neutral_sum.x += current.x;
        neutral_sum.y += current.y;
        neutral_sum.z += current.z;
        neutral_sample_count++;

        if (millis() - neutral_stable_start >= NEUTRAL_RECAPTURE_MS)
        {
            neutral_quat = {
                neutral_sum.w / neutral_sample_count,
                neutral_sum.x / neutral_sample_count,
                neutral_sum.y / neutral_sample_count,
                neutral_sum.z / neutral_sample_count
            };
            neutral_quat = q_normalize(neutral_quat);
            neutral_recaptured = true;
            neutral_stable_start = 0;
            neutral_sample_count = 0;
            neutral_sum = {0, 0, 0, 0};
            Serial.println("Neutral re-capturada (pendiente compensada)");
        }
    }
    else
    {
        neutral_stable_start = 0;
        neutral_sample_count = 0;
        neutral_sum = {0, 0, 0, 0};
    }

    // Send debug data via BLE if enabled
    ble_send_debug(roll, pitch, detected);

    // Debug
    Serial.printf("Roll: %.1f  Pitch: %.1f  -> ", roll, pitch);

    if (detected >= 0)
    {
        const char *names[] = {"R", "1", "2", "3", "4", "5", "N"};
        Serial.println(names[detected]);
    }
    else
    {
        Serial.println("???");
        stability_counter = 0;
        pending_gear = -1;
        return;
    }

    // ── Stabilization: exigir N frames consecutivos ──
    if (detected == pending_gear)
    {
        stability_counter++;
    }
    else
    {
        pending_gear = detected;
        stability_counter = 1;
    }

    if (stability_counter >= STABILITY_FRAMES && (uint8_t)detected != current_gear)
    {
        gears_set((uint8_t)detected);
        stability_counter = 0;
        pending_gear = -1;
    }
}
