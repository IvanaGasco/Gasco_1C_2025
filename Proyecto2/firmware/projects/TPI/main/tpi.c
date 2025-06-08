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
 * @author Ivana Gasco (ivana.gasco@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "analog_io_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_mcu.h"
#include "timer_mcu.h"
#include "led.h"
#include "ble_mcu.h"
#include "MP3v5050.h"

/*==================[macros and definitions]=================================*/
#define PERIODO_VISUALIZAR 100000
#define Cd 0.97
#define A1 7.55e-4  // m² (área de entrada, 7.55 cm²)
#define A2 5.027e-5 // m² (área de garganta, 0.50 cm²)
#define p 1.2 //  Kg/m3
#define umbral 0.0 // definir umbral
#define umbral_objetivo 0.0
#define umbral_limite 0.0
/*==================[internal data definition]===============================*/
TaskHandle_t MedirPresionTaskHandle = NULL;
float presion_Kpa = 0; //p1-p2
float qv = 0;
uint32_t periodo_actual = PERIODO_VISUALIZAR;
char msg[32];

/*==================[internal functions declaration]=========================*/

/**
 * @brief Función invocada en la interrupción del temporizador,
 * envía una notificación a la tarea de visualización para que ejecute su lógica
 */
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(MedirPresionTaskHandle, pdFALSE);    
}

 
void CalcularFlujoEspiracion(){
	qv = Cd*A2*sqrt((2*(presion_Kpa * 1000)) / (p * (1.0 - pow((A2/A1), 2)))); // qv = m3/s
	
}

// Función callback para BLE (maneja datos recibidos)
void BLE_Callback(uint8_t *data, uint8_t length){
    // agregar codigo para tomar datos
}

void Comunicacion_Serie() {
	snprintf(msg, sizeof(msg), "%.3f m3/s\r\n", qv);
	UartSendString(UART_PC, msg);
	snprintf(msg, sizeof(msg), "%.2f Kpa\r\n", presion_Kpa);
	UartSendString(UART_PC, msg);

}

/*void Serial_Plotter(){

	// Leer CH1
    AnalogInputReadSingle(CH1, &value_mv);

    // Convertir y enviar por UART
    char *str_val = (char *)UartItoa(value_mv, 10);
    UartSendString(UART_PC, ">voltaje:");
	UartSendString(UART_PC, str_val);
    UartSendString(UART_PC, "\r\n");

}
*/

static void MedirPresionTask(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		presion_Kpa = MP3v5050ReadPressure_kPa();
		if (presion_Kpa > umbral){
			CalcularFlujoEspiracion();
			Comunicacion_Serie();
			//Serial_Plotter();
			// ENVIAR DATOS POR BLE
            if (BleStatus() == BLE_CONNECTED) {
                snprintf(msg, sizeof(msg), "*G%.2f,%.3f*\n", presion_Kpa, qv);
                BleSendString(msg);
            }
			//BLE_Transmicion();
			LedOn(LED_1);
			if (qv >= umbral_objetivo){
				LedOn(LED_2);
			} else {
				LedOff(LED_2);
			} if (qv <= umbral_limite) {
				LedOn(LED_3);
			} else {
				LedOff(LED_3);
			}
		} else {
			LedOff(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
	}
}

void Comunicacion_Usuario(){
	//extraer edad, sexo, patologias respiratorias
	//definir el umbral_objetivo para usuario
}




/*==================[external functions definition]==========================*/

void app_main(void){
	LedsInit();

	MP3v5050Init();
	
	serial_config_t uart_config = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = Comunicacion_Serie,
		.param_p = NULL
	};	
	UartInit(&uart_config);

   
	ble_config_t ble_config = {
    .device_name = "ESP_EDU_Ivana",
    .func_p = BLE_Callback
	};

	BleInit(&ble_config);

	timer_config_t timerMedirPresion = {
        .timer = TIMER_A,
        .period = PERIODO_VISUALIZAR,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timerMedirPresion);

	/*analog_input_config_t adc_config = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
    };

    AnalogInputInit(&adc_config);
	AnalogOutputInit();
	*/

    xTaskCreate(&MedirPresionTask, "MedirPresionTask", 2048, NULL, 5, &MedirPresionTaskHandle);

    
    TimerStart(timerMedirPresion.timer);
	
	
}
/*==================[end of file]============================================*/