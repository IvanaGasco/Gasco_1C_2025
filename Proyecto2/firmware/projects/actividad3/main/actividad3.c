/*! @mainpage Actividad 3 - Proyecto 2
 *
 *@section descripcion_general Descripción general
 * El siguiente programa realiza mediciones de distancia mediante un sensor ultrasónico (HC-SR04).
 * En base a la medición se controlan los LED, que se encienden según distintos rangos de distancia.
 * Además, se visualiza la distancia medida en un display LCD de 3 dígitos (ITSE0803).
 *
 * En este ejercicio no solo se reemplaza la tarea de lectura de teclas por el uso de un temporizador (Timer), 
 * sino que también se agregan funcionalidades mediante el puerto serie (UART).
 *
 * @section funcionalidades Funcionalidades
 * 
 * **Control por botones físicos:**
 * -`SWITCH_1`: Activa o desactiva la medición.
 * - `SWITCH_2`: Activa el modo hold (congela el valor medido actual).

 * **Control por comandos UART (Serial Monitor):**
 * - `'o'`: Activa/desactiva la medición (equivale a SWITCH_1).
 * - `'h'`: Activa/desactiva el modo hold (equivale a SWITCH_2).
 * - `'i'`: Cambia entre centímetros y pulgadas.
 * - `'m'`: Muestra la máxima distancia registrada.
 * - `'f'`: Aumenta la frecuencia de actualización
 * - `'s'`: Disminuye la frecuencia de actualización.
 * 
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
 * | 07/05/2025  | Creación del documento                           |
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
#include "uart_mcu.h"


/*==================[macros and definitions]=================================*/

/**
 * @brief Período de activación del temporizador para la tarea de visualización (en microsegundos).
 */
#define PERIODO_VISUALIZAR 1000000
/*==================[internal data definition]===============================*/
TaskHandle_t visualizarTaskHandle = NULL;
bool medir_encendido = false; 
bool hold_encendido = false;
bool medir_pulgadas = false;
bool aumentar_vel = false;
bool disminuir_vel = false;
bool mostrar_max = false;
uint16_t distancia;
uint16_t distancia_hold;
uint16_t maxMedicion = 0;
uint32_t periodo_actual = PERIODO_VISUALIZAR;

/*==================[internal functions declaration]=========================*/

/**
 * @brief Función que es ejecutada mediante la interrupción asociada al SWITCH 1, o al ingresar la letra 'o' mediante puerto serie
 * La misma posee la siguiente actividad: 
 * - Alterna el estado de medición (activado/desactivado)
 */
void FuncTecla1() {
    medir_encendido = !medir_encendido;
}

/** @brief Función que es ejecutada mediante la interrupción asociada al SWITCH 2, o al ingresar la letra 'h' mediante puerto serie
 * La misma realiza las siguientes actividades: 
 * - Activa o desactiva el modo hold
 * - Si se activa, guarda el valor actual del sensor para mantenerlo fijo en el display
 */
void FuncTecla2() {
    hold_encendido = !hold_encendido;
    if (hold_encendido == true) {
        distancia_hold = distancia;
    }
}

/**
 * @brief Guarda la mayor distancia medida hasta el momento y la fija en el display.
 *
 * Esta función compara la distancia actual con la máxima registrada. 
 * Si la actual es mayor, actualiza el valor máximo.
 * Luego, activa el modo hold y muestra esa distancia máxima en el display.
 */
void max_distancia() {
    if (distancia > maxMedicion) {
        maxMedicion = distancia;
    }
    distancia_hold = maxMedicion;
    hold_encendido = true;
}


/**
 * @brief ManejoPuertoSerial que se ejecuta al recibir un byte por UART.
 *
 * Esta función interpreta comandos enviados desde el monitor serie y 
 * modifica el comportamiento del sistema en consecuencia.
 *
 * Comandos válidos:
 * - `'o'`: Alterna la activación de la medición.
 * - `'h'`: Alterna el modo hold (congela la medición actual).
 * - `'i'`: Cambia entre visualización en centímetros o pulgadas.
 * - `'m'`: Activa el modo de visualización de la mayor distancia registrada.
 * - `'f'`: Aumenta la frecuencia de actualización (reduce el período).
 * - `'s'`: Disminuye la frecuencia de actualización (aumenta el período).
 *
 */

void ManejoPuertoSerial(void *param) {
    uint8_t recibido;
    if (UartReadByte(UART_PC, &recibido)) {
        switch (recibido) {
            case 'o':
                FuncTecla1();  // Simula la tecla 1
                break;
            case 'h':
                FuncTecla2();  // Simula la tecla 2
                break;
            case 'i':
                medir_pulgadas = !medir_pulgadas;
                break;
            case 'm':
                mostrar_max = !mostrar_max;
                if (mostrar_max) {
                    max_distancia();
                }
                break;
				case 'f':
				if (periodo_actual >= 100000) {  // Evita que baje de 100 ms
					periodo_actual -= 100000;
					TimerStop(TIMER_A);
					TimerUpdatePeriod(TIMER_A, periodo_actual);
					TimerStart(TIMER_A);
				}
				break;
			case 's':
				periodo_actual += 100000;
				TimerStop(TIMER_A);
				TimerUpdatePeriod(TIMER_A, periodo_actual);
				TimerStart(TIMER_A);
				break;			
            default:
                break;
        }
    }
}




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
static void visualizarTask(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        if (medir_encendido) {
			if (medir_pulgadas){
				distancia = HcSr04ReadDistanceInInches();
			} else {
				distancia = HcSr04ReadDistanceInCentimeters();
			}
		
			if (hold_encendido) {
				LcdItsE0803Write(distancia_hold);
				UartSendString(UART_PC, (char*)UartItoa(distancia_hold, 10));
				UartSendString(UART_PC, medir_pulgadas ? " in\r\n" : " cm\r\n");
			} else {
				LcdItsE0803Write(distancia);
				UartSendString(UART_PC, (char*)UartItoa(distancia, 10));
				UartSendString(UART_PC, medir_pulgadas ? " in\r\n" : " cm\r\n");
			}
		} else {
			LcdItsE0803Off();
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
}
		


/**
 * @brief Función invocada en la interrupción del temporizador,
 * envía una notificación a la tarea de visualización para que ejecute su lógica
 */
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(visualizarTaskHandle, pdFALSE);    
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
 * - Puerto serie (UART_PC) para recepción de comandos por terminal
 * 
 * Crea la tarea de visualización ("visualizarTask"), que también realiza la medición
 * de distancia, visualiza en el display y controla el encendido de LEDs
 *
 * Configura:
 * - El temporizador para notificar periódicamente a la tarea de visualización
 * - La UART para manejar comandos mediante la función "ManejoPuertoSerial"
 */
void app_main(void){
    // Inicializacion de perifericos
    LedsInit();
    SwitchesInit();
    SwitchActivInt(SWITCH_1, FuncTecla1, 0);
    SwitchActivInt(SWITCH_2, FuncTecla2, 0);
    HcSr04Init(GPIO_3,GPIO_2);
    LcdItsE0803Init();

	serial_config_t uart_config = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = ManejoPuertoSerial,
		.param_p = NULL
	};	
	UartInit(&uart_config);
	

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
