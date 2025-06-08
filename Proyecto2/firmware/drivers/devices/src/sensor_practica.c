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
#include "sensor_temp.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include <stdio.h>
#include "gpio_mcu.h"

/*==================[macros and definitions]=================================*/
//#define VS_MV 3000.0 
/*==================[internal data declaration]==============================*/

//analog_input_config_t configuracionCanal = {CH1, ADC_SINGLE, NULL, NULL, 10000}

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/

bool Sensor_TempInit(void) {
    analog_input_config_t adc_config = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
    };

    AnalogInputInit(&adc_config);
    return true;
}


float Sensor_Medir_Temp_K(void) {
    uint16_t value_mv;
    AnalogInputReadSingle(CH1, &value_mv);  // Lee del canal CH1

    //EJEMPLO DE FUNCION DE DIGITALIZACION DE SEÑAL
    float vout = (float)value_mv/ 1000.0;  // convierte a voltios
    float Temperatura_K = (((vout/ 3.3) - 0.04) / 0.018) + 20;
    printf("value_mv = %d\n", value_mv);

    // Convertir y enviar por UART
    
    return Temperatura_K;

}

float Sensor_pH_Medir_pH(void) {
    float Medicion_pH = 0.0;
    return Medicion_pH;
}

bool  Sensor_Humedad_Medir(void){
    return GPIORead(GPIO_12);
}

/*==================[end of file]============================================*/
