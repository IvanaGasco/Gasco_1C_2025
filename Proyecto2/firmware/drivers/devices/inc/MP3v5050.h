#ifndef MP3V5050_H
#define MP3V5050_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa el sensor MP3V5050.
 *
 * @return true si la inicialización fue exitosa.
 */
bool MP3v5050Init(void);

/**
 * @brief Lee la presión diferencial medida por el sensor MP3V5050.
 *
 * @return Presión en kilopascales (kPa).
 */
float MP3v5050ReadPressure_kPa(void);

/**
 * @brief Tarea para leer la presión del sensor y enviarla por UART.
 *
 * Esta función está diseñada para ser usada como tarea de FreeRTOS.
 *
 * @param pvParameter Parámetro de tarea (no se utiliza).
 */
void MP3v5050ReadPressureTask(void *pvParameter);

/**
 * @brief Desinicializa el sensor MP3V5050.
 *
 * @return true si la desinicialización fue exitosa.
 */
bool MP3v5050Deinit(void);

#endif /* MP3V5050_H */
