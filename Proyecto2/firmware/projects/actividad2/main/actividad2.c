/*! @mainpage Actividad 1 - Proyecto 2
 *
 * @section genDesc Descripción General
 *
 * El siguiente programa realiza mediciones de distancia mediante un sensor ultrasónico (HC-SR04).
 * En base a la medición se controlan las LEDs: se encienden según distintos rangos de distancia,
 * y además se visualiza la distancia medida en el display (LCD ITSE0803).
 * El sistema permite activar o desactivar la medición (medir_encendido) y
 * mantener (hold_encendido) una medición específica para visualizarla de forma fija.
 * Se agrega funcionalidad de temporizador, se fusiona la tarea de medición y visualización,
 * y se reemplaza el uso de teclas por el temporizador.
 * 
 * 
 * @subsection sensorConn Sensor Ultrasónico HC-SR04
 * 
 * | Señal        | ESP32 GPIO |
 * |:------------:|:----------:|
 * | Trigger      | GPIO_3     |
 * | Echo         | GPIO_2     |
 *
 * @subsection displayConn Display ITSE0803 
 * 
 * | PIN ESP32    | Entrada CD4543 | Función             |
 * |:------------:|:--------------:|:--------------------|
 * | GPIO_20      | D1 (b0)        | Entrada BCD         |
 * | GPIO_21      | D2 (b1)        | Entrada BCD         |
 * | GPIO_22      | D3 (b2)        | Entrada BCD         |
 * | GPIO_23      | D4 (b3)        | Entrada BCD         |
 * | GPIO_19      | SEL_1          | Selección dígito 1  |
 * | GPIO_18      | SEL_2          | Selección dígito 2  |
 * | GPIO_9       | SEL_3          | Selección dígito 3  |
 * | +5V          | Vcc            | Alimentación        |
 * | GND          | GND            | Tierra              |
 *
 * @section changelog Changelog
 *
 * |   Fecha     | Descripción                                      |
 * |:-----------:|:------------------------------------------------:|
 * | 12/04/2025  | Creación del documento                           |
 *
 * @author Ivana Gasco (ivana.gasco@ingenieria.uner.edu.ar)
 */



/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <hc_sr04.h> 
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "gpio_mcu.h"
#include "lcditse0803.h"
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/

/**
 * @brief Período de activación del temporizador para la tarea de visualización (en microsegundos).
 */

#define CONFIG_BLINK_PERIOD_VISUALIZACION_US 1000000
/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
/*==================[internal data definition]===============================*/

/**
 * @brief Handle para la tarea de visualización.
 */
TaskHandle_t visualizar_tsk_handle = NULL;

/**
 * @brief Última distancia medida por el sensor ultrasónico.
 */
uint16_t distancia;

/**
 * @brief Distancia almacenada cuando el sistema se encuentra en modo HOLD.
 */
uint16_t distancia_hold;

/**
 * @brief Indica si la medición está activada (true) o desactivada (false).
 */
bool medir_encendido = false;

/**
 * @brief Indica si el modo HOLD está activado (true) o no (false).
 */
bool hold_encendido = false;

/*==================[internal functions declaration]=========================*/

/**
 * @brief Función invocada en la interrupción del temporizador,
 * envía una notificación a la tarea de visualización para que ejecute su lógica
 */
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(visualizar_tsk_handle, pdFALSE);    /* Envía una notificación a la tarea asociada al LED_1 */
}


/**
 * @brief Función que es ejecutada mediante la interrupción asociada al SWITCH 1
 * La misma posee la siguiente actividad: 
 * - Alterna el estado de medición (activado/desactivado)
 */
void Func_tecla_1(void){
    medir_encendido = !medir_encendido;
}

/** @brief Función que es ejecutada mediante la interrupción asociada al SWITCH 2
 * La misma realiza las siguientes actividades: 
 * - Activa o desactiva el modo hold
 * - Si se activa, guarda el valor actual del sensor para mantenerlo fijo en el display
 */
void Func_tecla_2(void){
    hold_encendido = !hold_encendido;
    if(hold_encendido==true){
        distancia_hold = distancia;
    }
}

// TAREA VISULIZACIÓN
/**
 * @brief Tarea encargada de visualizar la distancia en el display y controlar el encendido de LEDs, 
 * también realiza la medicion de la distancia
 * 
 * Si el sistema está apagado, se apaga el display
 * Si está encendido, se mide la distancia con el sensor HC-SR04, 
 * y muestra la distancia(distancia) actual o 
 * la distancia almacenada en modo hold (hold_encendido)
 * 
 * Enciende los LEDs según el siguiente criterio:
 * - <10cm: Todos apagados
 * - 10–20cm: LED 1 encendido
 * - 20–30cm: LED 1 y 2 encendidos
 * - >30cm: Todos encendidos
 * Esta tarea se ejecuta en respuesta a una notificación del temporizador
 */
static void VisualizacionTask(void *pvParameter) {

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (medir_encendido == true){
            // mide la distancia con el driver
                distancia = HcSr04ReadDistanceInCentimeters();
            }
        if (medir_encendido == false) {

            LcdItsE0803Off(); // apago el display

        } else {
            // si está activado

			//si está activado el hold muestro el valor guardado
            if (hold_encendido == true) {
            
                LcdItsE0803Write(distancia_hold);

			//sino muestro el valor de la medición actual	
            } else {
                LcdItsE0803Write(distancia);
            }

            // sincronización de leds segun distancia
            if (distancia < 10) {
                // si es menor a 10cm no enciende ninguna
                LedOff(LED_1);
                LedOff(LED_2);
                LedOff(LED_3);
            } else if (distancia >= 10 && distancia < 20) {
                // entre 10 y 20cm solo enciende led1
                LedOn(LED_1);
                LedOff(LED_2);
                LedOff(LED_3);
            } else if (distancia >= 20 && distancia < 30) {
                // entre 20 y 30cm enciende led1 y led2
                LedOn(LED_1);
                LedOn(LED_2);
                LedOff(LED_3);
            } else {
                // si la distancia es mayor a 30cm se encienden los 3
                LedOn(LED_1);
                LedOn(LED_2);
                LedOn(LED_3);
            }
        }
        // cada 1 segundo
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}



/*==================[external functions definition]==========================*/
/**
 * @brief Función principal del programa
 *
 * Inicializa los módulos necesarios:
 * - LEDs
 * - Switches
 * - Display ITSE0803
 * - Sensor ultrasónico HC-SR04
 *
 * Crea la tarea de visualización (VisualizacionTask), que integra la medición,
 * el control por interruptores y el control de LEDs
 * Configura y activa el temporizador para disparar periódicamente dicha tarea
 */
void app_main(void){
    LedsInit();
	SwitchesInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);

    SwitchActivInt(SWITCH_1, Func_tecla_1, 0);
    SwitchActivInt(SWITCH_2, Func_tecla_2,0);

    timer_config_t timer_visualizacion = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD_VISUALIZACION_US,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timer_visualizacion);
       
    xTaskCreate(&VisualizacionTask, "Visualizacion", 2048, NULL, 5, &visualizar_tsk_handle);
    
    TimerStart(timer_visualizacion.timer);
}
/*==================[end of file]============================================*/