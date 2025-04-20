/*! @mainpage Actividad 1 - Proyecto 2
 *
 * @section genDesc Descripción General
 *
 * El siguiente programa realiza mediciones de distancia mediante un sensor ultrasónico (HC-SR04).
 * En base a la medición se controlan las LEDs: se encienden según distintos rangos de distancia,
 * y además se visualiza la distancia medida en el display (LCD ITSE0803).
 * El sistema permite activar o desactivar la medición (medir_encendido) y
 * mantener (hold_encendido) una medición específica para visualizarla de forma fija.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
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
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
TaskHandle_t visualizar_tsk_handle = NULL;
TaskHandle_t medir_tsk_handle = NULL;
TaskHandle_t teclas_task_handle = NULL;
uint16_t distancia;              // distancia medida más reciente
uint16_t distancia_hold;         // distancia guardada en modo HOLD
bool medir_encendido = false;            // True si la medición está activa (SWITCH 1)
bool hold_encendido = false;  
/*==================[internal functions declaration]=========================*/

// TAREA VISULIZACIÓN
/**
 * @brief Tarea encargada de visualizar la distancia en el display y controlar el encendido de LEDs
 * 
 * Si el sistema está apagado, se apaga el display.
 * Si está encendido, muestra la distancia(distancia) actual o 
 * la distancia almacenada en modo hold (hold_encendido).
 * Enciende los LEDs según el siguiente criterio:
 * - <10cm: Todos apagados
 * - 10–20cm: LED 1 encendido
 * - 20–30cm: LED 1 y 2 encendidos
 * - >30cm: Todos encendidos
 */
static void VisualizacionTask(void *pvParameter) {

    while (true) {
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


// TAREA MEDIR 
/**
 * @brief Tarea encargada de medir la distancia utilizando el sensor HC-SR04.
 * 
 * Si medir está activada (medir_encendido == true), actualiza la variable global "distancia".
 * La medición se realiza cada 1 segundo.
 */
static void MedirTask(void *pvParameter) {
    while (true) {
            if (medir_encendido == true){
            // mide la distancia con el driver
                distancia = HcSr04ReadDistanceInCentimeters();
            }
        //cada 1 segundo
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
        
    }
}

//TAREA TECLAS
/**
 * @brief Tarea encargada de gestionar las entradas de las teclas.
 * 
 * - SWITCH_1: Activa o desactiva la medición (toggle medir_encendido).
 * - SWITCH_2: Activa o desactiva el modo HOLD 
 * (si se activa, guarda el valor actual de "distancia" en "distancia_hold").
 * Se lee el estado de los switches cada 100 ms 
 * (para que no sea necesario apretar demasiado tiempo la tecla).
 */
static void TeclasTask(void *pvParameter) {
    uint8_t teclas;

    while (true) {
        teclas = SwitchesRead();

        switch(teclas) {
            case SWITCH_1:
                medir_encendido = !medir_encendido;
                break;

            case SWITCH_2:
                hold_encendido = !hold_encendido;
                if(hold_encendido==true){
                    distancia_hold = distancia;
                }
                break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
}



/*==================[external functions definition]==========================*/
/**
 * @brief Función principal del programa. 
 * 
 * Inicializa:
 * - LEDs
 * - Switches
 * - Display ITSE0803
 * - Sensor HC-SR04
 * 
 * Crea las tareas:
 * - VisualizacionTask
 * - MedirTask
 * - TeclasTask
 */
void app_main(void){
    LedsInit();
	SwitchesInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);

    xTaskCreate(&VisualizacionTask, "Visualizacion", 2048, NULL, 5, &visualizar_tsk_handle);
    xTaskCreate(&MedirTask, "Medir", 2048, NULL, 5, &medir_tsk_handle);
    xTaskCreate(&TeclasTask, "Teclas", 2048, NULL, 5, &teclas_task_handle);
}

/*==================[end of file]============================================*/