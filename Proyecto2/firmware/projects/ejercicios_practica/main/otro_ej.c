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
/*==================[macros and definitions]=================================*/
#define PERIODO_VISUALIZAR 50 //frec 20 Hz
#define BUFFER_SIZE 20
uint16_t buffer[BUFFER_SIZE];
uint16_t Temperatura_K;
uint16_t min_temperatura = 0;
uint16_t max_temperatura = 0;
uint8_t indice = 0;


/*==================[internal data definition]===============================*/
TaskHandle_t MedirTemperaturaTaskHandle = NULL;
float promedio_temperatura;
float suma_temp = 0;
float contador = 0;

/*==================[internal functions declaration]=========================*/

void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(MedirTemperaturaTaskHandle, pdFALSE);    
}

/*void Comunicacion_Serie() {
	snprintf(msg, sizeof(msg), "%.3f m3/s\r\n", qv);
	UartSendString(UART_PC, msg);
	snprintf(msg, sizeof(msg), "%.2f Kpa\r\n", presion_Kpa);
	UartSendString(UART_PC, msg);

}*/

void callbackADC(void *param) {
	// imprimir.		
}

void Calcular_Max_Temperatura() {
    if (Temperatura_K > max_temperatura) {
        max_temperatura = Temperatura_K;
    }
}


void Calcular_Min_Temperatura() {
    if (Temperatura_K < min_temperatura) {
        min_temperatura = Temperatura_K;
    }
}

void Calcular_Promedio_Temperatura() {
	buffer[indice] = Temperatura_K;
	indice = (indice + 1) % BUFFER_SIZE;

	suma_temp = 0;
	for (int i = 0; i < BUFFER_SIZE; i++) {
    	suma_temp += buffer[i];
	}
	promedio_temperatura = suma_temp / BUFFER_SIZE;

}

void ManejoPuertoSerial(void *param) {
    uint8_t recibido;
    if (UartReadByte(UART_PC, &recibido)) {
        switch (recibido) {
            case 'a':
				Calcular_Max_Temperatura();
				LcdItsE0803Write(max_temperatura);
				LedOn(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
                UartSendString(UART_PC, (char*)UartItoa(max_temperatura, 10));
				UartSendString(UART_PC, " K\r\n"); 
                break;
            case 'b':
				Calcular_Min_Temperatura();
				LcdItsE0803Write(min_temperatura);
				LedOff(LED_1);
				LedOn(LED_2);
				LedOff(LED_3);
                UartSendString(UART_PC, (char*)UartItoa(min_temperatura, 10));
				UartSendString(UART_PC, " K\r\n");
                break;
            case 'c':
				Calcular_Promedio_Temperatura();
				LcdItsE0803Write((uint16_t)promedio_temperatura);
				LedOff(LED_1);
				LedOff(LED_2);
				LedOn(LED_3);
                UartSendString(UART_PC, (char*)UartItoa(promedio_temperatura, 10));
				UartSendString(UART_PC, " K\r\n");
                break;
			default:
                break;
        }
	}
}


static void MedirTemperaturaTask(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		Temperatura_K = Sensor_Medir_Temp_K();
	}
}






/*==================[external functions definition]==========================*/
void app_main(void){

	LedsInit();
	LcdItsE0803Init();

	timer_config_t timerMedirTemperatura = {
        .timer = TIMER_A,
        .period = PERIODO_VISUALIZAR,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timerMedirTemperatura);

	serial_config_t uart_config = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = ManejoPuertoSerial,
		.param_p = NULL
	};	
	UartInit(&uart_config);


	xTaskCreate(&MedirTemperaturaTask, "MedirTemperaturaTask", 2048, NULL, 5, &MedirTemperaturaTaskHandle);

    
    TimerStart(timerMedirTemperatura.timer);
}


/*==================[end of file]============================================*/
