/**
 * @file mp3v5050.c
 * @author Ivana Gasco (ivana.gasco@ingenieria.uner.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-10-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/*==================[inclusions]=============================================*/
#include "MP3v5050.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include <stdio.h>

/*==================[macros and definitions]=================================*/
#define VS_MV 3000.0 
/*==================[internal data declaration]==============================*/

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/

bool MP3v5050Init(void) {
    // Si hay inicialización adicional del ADC, agregar aquí
    return true;
}

float MP3v5050ReadPressure_kPa(void) {
    uint16_t value_mv;
    AnalogInputReadSingle(CH1, &value_mv);  // Lee del canal CH1

    float vout = (float)value_mv / 1000.0;  // convierte a voltios
    float pressure_kPa = ((vout / 3.0) - 0.04) / 0.018;

    return pressure_kPa;
}

static void MP3v5050ReadPressureTask(void *pvParameter){
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        float pressure = MP3v5050ReadPressure_kPa();

        char buffer[32];
        snprintf(buffer, sizeof(buffer), ">presion: %.2f kPa\r\n", pressure);
        UartSendString(UART_PC, buffer);
    }
}

bool MP3v5050Deinit(void){
	// No hay recursos que liberar
	return true;
}

/*==================[end of file]============================================*/
