/*! @mainpage Actividad 2 - Proyecto 2
 * * \section genDesc Descripción General
 *
 * Esta aplicación mide distancias utilizando el sensor ultrasónico HC-SR04 y 
 * muestra el resultado en un display LCD de 7 segmentos.
 * Además, enciende progresivamente los LED_1, LED_2 y LED_3 según la distancia medida.
 * El sistema utiliza tareas y temporizadores de FreeRTOS para realizar mediciones 
 * periódicas, y botones para activar o desactivar la medición y mantener un valor fijo en pantalla.
 *
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 7/05/2025 | Document creation		                         |
 *
 * @author Ivana Gasco(ivana.gasco@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <gpio_mcu.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include <hc_sr04.h>
#include "lcditse0803.h"
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/
#define PERIODO_VISUALIZAR 1000000
/*==================[internal data definition]===============================*/
TaskHandle_t visualizarTaskHandle = NULL;
bool medicionActivada = false; 
bool visualizacionActivada = false;
bool holdMedicion = false;
uint16_t medicionSensor;
uint16_t medicionHold;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Enciende los LEDs según el valor de la medición.
 *
 * Se usa un esquema de umbrales para encender diferentes combinaciones de LEDs:
 * - < 10 cm: todos apagados
 * - 10–20 cm: LED_1 encendido
 * - 20–30 cm: LED_1 y LED_2 encendidos
 * - > 30 cm: los tres LEDs encendidos
 *
 * @param medicion Valor de distancia en centímetros.
 */

void encenderLedsSegunMedicion(uint16_t medicion) {
    if (medicion < 10) {
        LedOff(LED_1);
        LedOff(LED_2);
        LedOff(LED_3);
    } 
    else if (medicion > 10 && medicion < 20) {
        LedOn(LED_1);
        LedOff(LED_2);
        LedOff(LED_3);
    }
    else if (medicion > 20 && medicion < 30) {
        LedOn(LED_1);
        LedOn(LED_2);
        LedOff(LED_3);
    }

    else if (medicion > 30) {
        LedOn(LED_1);
        LedOn(LED_2);
        LedOn(LED_3);
    }
}



/**
 * @brief Tarea periódica que actualiza el estado de los LEDs y el display LCD.
 *
 * Esta tarea se activa mediante una notificación enviada por el temporizador.
 * Si la medición está activada, lee la distancia del sensor, actualiza LEDs y 
 * muestra el valor en el display. Si el modo hold está activo, muestra el valor retenido.
 *
 * @param pvParameter No se utiliza.
 */
static void visualizarTask(void *pvParameter){
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        if (medicionActivada) {
            medicionSensor = HcSr04ReadDistanceInCentimeters();
            encenderLedsSegunMedicion(medicionSensor);
            if (holdMedicion) {
                LcdItsE0803Write(medicionHold);
            } else {
                LcdItsE0803Write(medicionSensor);  
            }    
        } else {
            LcdItsE0803Off();
        }
    }
}

/**
 * @brief Función llamada por la interrupción del temporizador A.
 *
 * Notifica a la tarea de visualización para que se ejecute.
 *
 * @param param No se utiliza.
 */
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(visualizarTaskHandle, pdFALSE);    
}


/**
 * @brief Callback asociado a SWITCH_1.
 *
 * Activa o desactiva la medición con el sensor ultrasónico.
 */
void FuncTecla1() {
    medicionActivada = !medicionActivada;
}

/**
 * @brief Callback asociado a SWITCH_2.
 *
 * Activa o desactiva el modo hold. Si se activa, guarda el valor actual del sensor
 * para mantenerlo fijo en el display.
 */
void FuncTecla2() {
    holdMedicion = !holdMedicion;
    if (holdMedicion == true) {
        medicionHold = medicionSensor;
    }
}


/*==================[external functions definition]==========================*/
/**
 * @brief Función principal del programa.
 *
 * Inicializa todos los periféricos (LEDs, botones, sensor ultrasónico, display),
 * configura un temporizador para disparar periódicamente la actualización de la medición,
 * y crea la tarea responsable de la visualización.
 */
void app_main(void){
    // Inicializacion de perifericos
    LedsInit();
    SwitchesInit();
    SwitchActivInt(SWITCH_1, FuncTecla1, 0);
    SwitchActivInt(SWITCH_2, FuncTecla2, 0);
    HcSr04Init(GPIO_3,GPIO_2);
    LcdItsE0803Init();


    timer_config_t timerVisualizar = {
        .timer = TIMER_A,
        .period = PERIODO_VISUALIZAR,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timerVisualizar);



    xTaskCreate(&visualizarTask, "visualizarTask", 2048, NULL, 5, &visualizarTaskHandle);
    
    TimerStart(timerVisualizar.timer);
    

    
}
