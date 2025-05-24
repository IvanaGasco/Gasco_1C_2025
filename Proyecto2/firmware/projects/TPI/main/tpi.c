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
#include "analog_io_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_mcu.h"
#include "timer_mcu.h"
#include "led.h"
#include "MP3v5050"

/*==================[macros and definitions]=================================*/
#define PERIODO_VISUALIZAR 1000000

/*==================[internal data definition]===============================*/
TaskHandle_t MedirPresionTaskHandle = NULL;
uint16_t presion_Kpa;
uint32_t periodo_actual = PERIODO_VISUALIZAR;

/*==================[internal functions declaration]=========================*/

/**
 * @brief Función invocada en la interrupción del temporizador,
 * envía una notificación a la tarea de visualización para que ejecute su lógica
 */
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(MedirPresionTaskHandle, pdFALSE);    
}

void CalcularFlujoEspiracion(void){
	pass; //aplicar formula
	return flujo_espiracion
}

static void MedirPresionTask(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		presion_Kpa = MP3v5050ReadPressure_kPa(void);
		if (presion_Kpa > 0){
			flujo_espiracion = CalcularFlujoEspiracion();
		}


	}
}

void ManejoPuertoSerial(void *param) {
	pass;
}

/*==================[external functions definition]==========================*/

void app_main(void){
	LedsInit();
	MP3v5050Init();
	MP3v5050ReadPressureTask(void *pvParameter);
	
	serial_config_t uart_config = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = ManejoPuertoSerial,
		.param_p = NULL
	};	
	UartInit(&uart_config);

	timer_config_t timerMedirPresion = {
        .timer = TIMER_A,
        .period = PERIODO_VISUALIZAR,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timerVisualizar);


    xTaskCreate(&MedirPresionTask, "visualizarTask", 2048, NULL, 5, &MedirPresionTaskHandle);
    
    TimerStart(timerMedirPresion.timer);
	//inicializar mp3v5050
	//inicializar timer
}
/*==================[end of file]============================================*/