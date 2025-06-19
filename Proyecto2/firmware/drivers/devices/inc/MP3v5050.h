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

uint16_t Valor_voltaje(void);

#endif /* MP3V5050_H */
