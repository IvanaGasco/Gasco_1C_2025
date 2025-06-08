/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
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
#include <sensor_practica.h>
#include "lcditse0803.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "switch.h"
#include "buzzer.h"
#include "hc_sr04.h"


/*==================[macros and definitions]=================================*/
#define PERIODO_DISTANCIA_PERSONA 1000000
#define PERIODO_TEMPERATURA_PERSONA 100000
#define BUFFER_SIZE 10
uint16_t buffer_temperatura[BUFFER_SIZE];
/*==================[internal data definition]===============================*/
TaskHandle_t MedirDistanciaPersonaTaskHandle = NULL;
TaskHandle_t MedirTemperaturaPersonaTaskHandle = NULL;
uint16_t distancia;
uint16_t temperatura;
uint16_t suma_temperatura;
uint8_t indice_temperatura = 0;
uint8_t contador_temperatura = 0;
bool distancia_correcta = false;
bool distancia_fuera_rango = false;
bool alerta_temperatura_elevada = false;
float promedio_temperatura;


/*==================[internal functions declaration]=========================*/
void FuncTimerA(void* param){
	vTaskNotifyGiveFromISR(MedirDistanciaPersonaTaskHandle, pdFALSE);     
}

void FuncTimerB(void* param){
    vTaskNotifyGiveFromISR(MedirTemperaturaPersonaTaskHandle, pdFALSE);    
}

void Calcular_Promedio_Temperatura() {

	suma_temperatura = 0;
	for (int i = 0; i < BUFFER_SIZE; i++) {
    	suma_temperatura += buffer_temperatura[i];
	}
	promedio_temperatura = (float)suma_temperatura / BUFFER_SIZE;

	if (promedio_temperatura > 37.5){
		alerta_temperatura_elevada = true;
	} else {
		alerta_temperatura_elevada = false;
	}

}
void Comunicacion_UART(){
	UartSendString(UART_PC, (char*)UartItoa(promedio_temperatura, 10));
	UartSendString(UART_PC, "ºC\r\n");
	UartSendString(UART_PC, "Persona a \r\n");
	UartSendString(UART_PC, (char*)UartItoa(distancia, 10));
	UartSendString(UART_PC, "cm\r\n");
	//char msg[64];
	//sprintf(msg, "%.0fºC persona a %d cm\r\n", promedio_temperatura, distancia);
	//UartSendString(UART_PC, msg);

}

void Verificar_Temperatura(){
	if (alerta_temperatura_elevada){
		BuzzerOn();
	} else {
		BuzzerOff();
	}
}

static void MedirDistanciaPersonaTask(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		distancia = HcSr04ReadDistanceInCentimeters();
		if (distancia < 8) {
                // persona muy cerca
				distancia_correcta = false;
				distancia_fuera_rango = false;
                LedOn(LED_1);
                LedOff(LED_2);
                LedOff(LED_3);
            } else if (distancia >= 8 && distancia <= 12) {
                // distancia ideal
				distancia_correcta = true;
				distancia_fuera_rango = false;
                LedOff(LED_1);
                LedOn(LED_2);
                LedOff(LED_3);
            } else if (distancia > 12 && distancia < 140) {
                // persona muy lejos
				distancia_correcta = false;
				distancia_fuera_rango = false;
                LedOff(LED_1);
                LedOff(LED_2);
                LedOn(LED_3);
            } else {
                // persona fuera de rango REINICIO
				distancia_fuera_rango = true;
				distancia_correcta = false;
				LedOff(LED_1);
				LedOff(LED_2);
                LedOff(LED_3);
            }
        }
}

static void MedirTemperaturaPersonaTask(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (distancia_correcta){
			
			temperatura = Sensor_Medir_Temp_K();
			buffer_temperatura[indice_temperatura] = temperatura;
			indice_temperatura++;
		
			if (indice_temperatura == BUFFER_SIZE){
			Calcular_Promedio_Temperatura();
			Comunicacion_UART();
			Verificar_Temperatura();
			indice_temperatura = 0;
			
		}	
		} else if (distancia_fuera_rango){
			indice_temperatura = 0;
			BuzzerOff();
		}
	}
}


/*==================[external functions definition]==========================*/
void app_main(void){

	LedsInit();
	HcSr04Init(GPIO_3, GPIO_2);
	BuzzerInit(GPIO_22);
	Sensor_TempInit();

	timer_config_t timerMedirDistanciaPersona = {
        .timer = TIMER_A,
        .period = PERIODO_DISTANCIA_PERSONA,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timerMedirDistanciaPersona);

	timer_config_t timerMedirTemperaturaPersona= {
        .timer = TIMER_B,
        .period = PERIODO_TEMPERATURA_PERSONA,
        .func_p = FuncTimerB,
        .param_p = NULL
    };
    TimerInit(&timerMedirTemperaturaPersona);

	serial_config_t uart_config = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL
	};	
	UartInit(&uart_config);

	xTaskCreate(&MedirDistanciaPersonaTask, "MedirDistanciaPersonaTask", 2048, NULL, 5, &MedirDistanciaPersonaTaskHandle);
	xTaskCreate(&MedirTemperaturaPersonaTask, "MedirTemperaturaPersonaTask", 2048, NULL, 5, &MedirTemperaturaPersonaTaskHandle);
    
    TimerStart(timerMedirDistanciaPersona.timer);
	TimerStart(timerMedirTemperaturaPersona.timer);
}
/*==================[end of file]============================================*/
