/*! @mainpage Examen
 *
 * @section descripcion_general General Description
 *
 * Esta aplicación resuelve dos problemas: 
 * Resuelve el enunciado en cuestión: utilizando lo aprendido a lo largo del cursado.
 * ONSIGNA:
El sistema está compuesto por un sensor de humedad y temperatura DHT11 y 
un  sensor analógico de radiación.

Por un lado se debe detectar el riesgo de nevada, la cual se da si la húmedad 
se encuentra por encima del 85%  y la temperatura entre 0 y 2ºC. 
Para esto se deben tomar muestras cada 1 segundo y se envían por UART con el siguiente formato:
“Temperatura: 10ºC - Húmedad: 70%”

Si se da la condición de riesgo de nevada se debe indicar el estado encendiendo el 
led Rojo de la placa, además del envío de un mensaje por la UART:
“Temperatura: 1ºC - Húmedad: 90% - RIESGO DE NEVADA”

Además se debe monitorizar la radiación ambiente, para ello se cuenta con un 
sensor analógico que da una salida de 0V para 0mR/h y 3.3V para una salida de 
100 mR/h. Se deben tomar muestras de radiación cada 5 segundos, enviando el mensaje por UART:
“Radiación 30mR/h”

Si se sobrepasan los 40mR/h se debe informar del riesgo por Radiación, 
encendiendo el led Amarillo de la placa, y enviando en el mensaje:
“Radiación 50mR/h - RADIACIÓN ELEVADA”

Si no hay riesgo de nevada ni radiación excesiva, se indicará con el led Verde esta situación.
El botón 1 se utilizará para encender el dispositivo, comenzando el muestreo de los sensores
 y el envío de información. El botón 2 apaga el dispositivo, deteniendo el proceso de muestreo 
 y apagando todas las notificaciones. 
Para el manejo del DHT11 cuenta con los siguientes drivers: dht11.c y dht11.h. 
Agreguelos al proyecto y utilice las funciones provistas para comunicarse con el dispositivo.

 * 
 * 
 * 
 * 
 *  @section hardConn Conexión de Hardware
 *
 * | Periférico        | ESP32 (Pin)       |
 * |:-----------------:|:-----------------:|
 * | Entrada analógica | CH1 (GPIO_1)      |
 * | DHT11             | GPIO_03           |   
 * 
 *
 * @section changelog Historial de Cambios
 *
 * | Fecha       | Descripción                    |
 * |:----------: |:------------------------------:|
 * | 09/06/2025  | Creación del documento         |
 *
 * @author Ivana Gasco (ivana.gasco@ingenieria.uner.edu.ar)
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <gpio_mcu.h>
#include "analog_io_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "switch.h"
#include "dht11.h"
/*==================[macros and definitions]=================================*/
#define PERIODO_MUESTREAR 1000000 // cada 1 seg
#define PERIODO_RADIACION 5000000 //cada 5 seg
/**
 * @brief Humedad medida con el sensor dht11
 */
uint16_t Humedad;

/**
 * @brief Temperatura medida con el sensor dht11
 */
uint16_t Temperatura;

/**
 * @brief Radiación medida con el sensor analógico
 */
uint16_t Radiacion;

/**
 * @brief bandera booleana global que indica si hay o no riesgo de nevada
 */
bool riesgo_nevada = false;

/**
 * @brief bandera booleana global que indica si hay o no radiacion elevada
 */
bool radiacion_elevada = false;

/**
 * @brief bandera booleana global que indica si el dispositivo esta encendido o no
 */
bool encender_dispositivo = false;

/**
 * @brief bandera booleana global que indica si hay o no condiciones favorables
 */
bool condiciones_favorables = false;

/*==================[internal data definition]===============================*/
/**
 * @brief Handle para la tarea de medir humedad y temperatura.
 */
TaskHandle_t MedirHumedad_y_Temperatura_TaskHandle = NULL;

/**
 * @brief Handle para la tarea de medir radiacion.
 */
TaskHandle_t MedirRadiacion_TaskHandle = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función invocada en la interrupción del temporizador A
 *
 * Notifica a la tarea de mediones de la temperatura y humedad
 * y lo envíe por UART
 */
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(MedirHumedad_y_Temperatura_TaskHandle, pdFALSE);    
}

/**
 * @brief Función invocada en la interrupción del temporizador A
 *
 * Notifica a la tarea de medicion de la radiacion
 * y lo envíe por UART
 */
void FuncTimerB(void* param){
    vTaskNotifyGiveFromISR(MedirRadiacion_TaskHandle, pdFALSE);    
}

/**
 * @brief Función que es ejecutada mediante la interrupción asociada al SWITCH 1, 
 * La misma posee la siguiente actividad: 
 * Cambia el estado "encender dispositivo" solo de desactivado a activado, enciende el
 * funcionamiento del mismo.
 */
void FuncTecla1() {
    encender_dispositivo = true;
}

/**
 * @brief Función que es ejecutada mediante la interrupción asociada al SWITCH 2, 
 * La misma posee la siguiente actividad: 
 * Cambia el estado "encender dispositivo" solo de activado a desactivado, apaga el
 * funcionamiento del mismo. Lo opuesto a tecla 1.
 */
void FuncTecla2() {
    encender_dispositivo = false;
}

/**
 * @brief Función que es ejecutada para verificar si las codiciones son buenas o no, 
 * La misma posee la siguiente actividad: 
 * Verifica los estados de las banderas booleanas riesgo nevada y radiacion elevada, si 
 * ambas se encuentran desactivadas, enciende el led verde indcando condiciones favorables.
 */
void Verificar_Condiciones() {
    if (riesgo_nevada == false && radiacion_elevada == false){
		LedOff(LED_1);
		LedOff(LED_2);
		LedOn(LED_3);
		condiciones_favorables = true;
	} else {
		LedOff(LED_1);
		LedOff(LED_2);
		LedOff(LED_3);
		condiciones_favorables = false;
	}
}

/**
 * @brief Tarea encargada de realizar las mediones de temperatura y humedad, además de
 * controlar el encendido del LED rojo (LED_1), 
 * 
 * Si el sistema está apagado, no se realiza ninguna funcion y se apagan las leds en caso de que
 * hubiesen quedado encendidas
 * Si está encendido, se mide la humedad y temperatura con el sensor dht11, 
 * 
 * Enciende los LED roja si se encuenta la temperatura entre 0 y 2 ºC y si al mimo tiempo la humedad es
 * igual o superior al 85%
 * 
 * Tambien se encarga de llamar a verificar condiciones
 * 
 * Esta tarea se ejecuta en respuesta a una notificación del temporizador
 */
static void MedirHumedad_y_Temperatura_Task(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (encender_dispositivo){
			_Bool dht11Read( float *Humedad, float *Temperatura );
			if (Humedad >= 85){
				if(Temperatura >= 0 && Temperatura <= 2){
					char msg[64];
					sprintf(msg, "%.0fºC - Humedad de %d pc - RIESGO NEVADA\r\n", Temperatura, Humedad);
					UartSendString(UART_PC, msg);
					riesgo_nevada = true;
					LedOn(LED_1);
					LedOff(LED_2);
					LedOff(LED_3);
				}
			} else{
				riesgo_nevada = false;
				Verificar_Condiciones(); // Llamo esta funcion aca porque tiene el timer más rápido, 
				//como son variables globales verifican igual(humedad, temperatura y radiacion)
				LedOff(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
				char msg[64];
				sprintf(msg, "%.0fºC - Humedad de %d pc\r\n", Temperatura, Humedad);
				UartSendString(UART_PC, msg);
			}

		}else{
			LedOff(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
	}
}

/**
 * @brief Tarea medicion de la radiación mediante el canal analógico.
 *
 * Esta tarea espera una notificación del temporizador, lee el valor del canal CH1,
 * lo convierte a ASCII, lo compara con el valor establecido como elevado, en base a esto
 * envía el mismo por UART para su visualización en la PC, indicando o no la radiacion elevada 
 * y encendiendo si es pertinente el led amarillo (indicando radiacion alta).
 */
static void MedirRadiacion_Task(void *pvParameter){
	uint16_t valor_radiacion;
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (encender_dispositivo){
			AnalogInputReadSingle(CH1, &valor_radiacion);
			float vout = valor_radiacion/ 1000.0;  // convierte a voltios
    		float Radiacion = ((vout)/3.3)*100; //si 3.3 equivale a 100R/h valor de salida = x

			// Convertir y enviar por UART
			
			if (Radiacion < 40){
				char *str_val = (char *)UartItoa(Radiacion, 10);
				UartSendString(UART_PC, ">Valor Radiación:");
				UartSendString(UART_PC, str_val);
				UartSendString(UART_PC, "R/h\r\n");
				LedOff(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
				radiacion_elevada = false;
			} 
			if (Radiacion >= 40){
				char *str_val = (char *)UartItoa(Radiacion, 10);
				UartSendString(UART_PC, ">Valor Radiación:");
				UartSendString(UART_PC, str_val);
				UartSendString(UART_PC, "R/h\r\n");
				UartSendString(UART_PC, "> - RADIACIÓN ELEVADA:");
				radiacion_elevada = true;
				LedOff(LED_1);
				LedOn(LED_2);
				LedOff(LED_3);

			}
		} else {
			LedOff(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
	}
}




/*==================[external functions definition]==========================*/
/**
 * @brief Función principal del programa
 *
 * Inicializa los módulos necesarios:
 * - LEDs
 * - Switches
 * - Sensor analogico
 * - Sensor uht11
 * - Puerto serie (UART_PC) 
 * 
 * Crea la tarea de Mediones de Temperatura y humedad (MedirHumedad_y_Temperatura) y 
 * medir radiacion (MedirRadiacion), donde ademas cada tarea enciende un led si es necesario
 *
 * Configura:
 * - Los temporizadores para medir las variables pertinentes y comunicar por uart según los 
 * tiempos del enunciado
 */

void app_main(void){
	void dht11Init( gpio_t GPIO_20);
	LedsInit();
	SwitchesInit();
    SwitchActivInt(SWITCH_1, FuncTecla1, 0);
    SwitchActivInt(SWITCH_2, FuncTecla2, 0);

	timer_config_t timerMedirHumedad_y_Temperatura = {
        .timer = TIMER_A,
        .period = PERIODO_MUESTREAR,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timerMedirHumedad_y_Temperatura);

	timer_config_t timerMedirRadiacion = {
        .timer = TIMER_B,
        .period = PERIODO_RADIACION,
        .func_p = FuncTimerB,
        .param_p = NULL
    };
    TimerInit(&timerMedirRadiacion);

	serial_config_t uart_config = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL
	};	
	UartInit(&uart_config);

	 // Inicializar ADC CH1
    analog_input_config_t adc_config = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
    };

    AnalogInputInit(&adc_config);
	AnalogOutputInit();

	xTaskCreate(&MedirHumedad_y_Temperatura_Task, "MedirHumedad_y_Temperatura_Task", 2048, NULL, 5, &MedirHumedad_y_Temperatura_TaskHandle);
	xTaskCreate(&MedirRadiacion_Task, "MedirRadiacion_Task", 2048, NULL, 5, &MedirRadiacion_TaskHandle);
	
	TimerStart(timerMedirHumedad_y_Temperatura.timer);
	TimerStart(timerMedirRadiacion.timer);
	


}
/*==================[end of file]============================================*/