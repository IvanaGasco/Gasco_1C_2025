/**
 * @file mp3v5050.c
 * @author Ivana Gasco (ivana.gasco@ingenieria.uner.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2025-05-20
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

//analog_input_config_t configuracionCanal = {CH1, ADC_SINGLE, NULL, NULL, 10000}

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/

bool MP3v5050Init(void) {
    analog_input_config_t adc_config = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
    };

    AnalogInputInit(&adc_config);
    return true;
}


float MP3v5050ReadPressure_kPa(void) {
    uint16_t value_mv;
    AnalogInputReadSingle(CH1, &value_mv);  // Lee del canal CH1

    float vout = (float)value_mv/ 1000.0;  // convierte a voltios
    float pressure_kPa = (((vout - 0.01 )/ 3.3) - 0.04) / 0.018;
    printf("value_mv = %d\n", value_mv);

    // Convertir y enviar por UART
    
    return pressure_kPa;

}

uint16_t Valor_voltaje(void){
    uint16_t voltaje_mv;
    AnalogInputReadSingle(CH1, &voltaje_mv);

    return voltaje_mv;

}

/*==================[end of file]============================================*/
