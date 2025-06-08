
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa el sensor MP3V5050.
 *
 * @return true si la inicialización fue exitosa.
 */
bool Sensor_TempInit(void);

/**
 * @brief Lee la presión diferencial medida por el sensor MP3V5050.
 *
 * @return Presión en kilopascales (kPa).
 */
float Sensor_Medir_Temp_K(void);

float Sensor_pH_Medir_pH(void);

bool Sensor_Humedad_Medir(void);