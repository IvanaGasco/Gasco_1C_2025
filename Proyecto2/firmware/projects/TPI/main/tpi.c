/*! @mainpage Trabajo Integrador Final
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
 * | 	VCC 	 	| 	 3.3 V	    |
 * |    GND         |     GND       |
 * |    OUT         |  CH1(GPIO_1)  |
 * |--------------------------------|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 11/06/2025 | Document creation		                         |
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
#include "switch.h"

/*==================[macros and definitions]=================================*/
#define PERIODO_MUESTREO 100000
#define CICLOS_FIN_ESPIRACION 10
#define Cd 0.97
#define A1 7.55e-4  // m² (área de entrada, 7.55 cm²)
#define A2 5.027e-5 // m² (área de garganta, 0.50 cm²)
#define p 1.2 //  Kg/m3
#define umbral 2.0 // definir umbral
#define umbral_objetivo 70.0
#define umbral_limite 50.0
/*==================[internal data definition]===============================*/
TaskHandle_t MedirPresionTaskHandle = NULL;
TaskHandle_t ActualizarLedsTaskHandle = NULL;
float presion_Kpa = 0; //p1-p2
float qv = 0;
uint16_t voltaje_mv;
uint32_t periodo_actual = PERIODO_MUESTREO;
char msg[32];

bool inicio_espiracion;
bool fin_espiracion;
float volumen_total_CVF = 0;
float volumen_1s_FEV1 = 0;
float dt = 0.1; // 100 ms = 0.1 s
float tiempo_total = 0;
float indice_de_capacidad_pulmonar;
uint16_t ciclos_sin_presion = 0; // global

/*==================[internal functions declaration]=========================*/

/**
 * @brief Función de callback del temporizador. 
 * Notifica a la tarea de medición de presión cuando se alcanza el período.
 * 
 * @param param Parámetro no utilizado.
 */
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(MedirPresionTaskHandle, pdFALSE);    
}

/**
 * @brief Calcula el flujo de aire espirado en m³/s 
 * a partir de la presión diferencial leída del sensor.
 */
void CalcularFlujoEspiracion(){
	qv = Cd*A2*sqrt((2*(presion_Kpa * 1000)) / (p * (1.0 - pow((A2/A1), 2)))); // qv = m3/s
	
}

//Intentar resetear
void FuncTecla1() {
	volumen_total_CVF = 0;
	volumen_1s_FEV1 = 0;
	tiempo_total = 0;
	indice_de_capacidad_pulmonar = 0;
	inicio_espiracion = false;
	fin_espiracion = false;
	ciclos_sin_presion = 0;
	qv= 0;
	presion_Kpa = 0;
	if (BleStatus() == BLE_CONNECTED) {
		snprintf(msg, sizeof(msg), "*B%.2f*\n", indice_de_capacidad_pulmonar);
		BleSendString(msg);
	}

}

void Detectar_Inicio_Fin_Espiracion(){
	if (presion_Kpa >= umbral){
		inicio_espiracion = true;
		fin_espiracion = false;
		ciclos_sin_presion = 0;
	} else {
		ciclos_sin_presion++;
		if (ciclos_sin_presion > CICLOS_FIN_ESPIRACION) { // por ejemplo, 10 ciclos = 1 segundo sin presión
			inicio_espiracion = false;
			fin_espiracion = true;

		}
	}
}

void Calculo_FEV1_CVF(){
    float volumen = qv * 100; // Escalar para trabajar con valores más manejables

    volumen_total_CVF += volumen * dt;

    if (tiempo_total <= 1.0) {
        volumen_1s_FEV1 += volumen * dt;
    }

    tiempo_total += dt;

    // Evitar división por cero
    if (volumen_total_CVF != 0) {
        indice_de_capacidad_pulmonar = (volumen_1s_FEV1 / volumen_total_CVF) * 100;
    } else {
        indice_de_capacidad_pulmonar = 0.0;
    }
}

static void ActualizarLedsTask(void *pvParameter) {
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		// LED_1 siempre encendido durante espiración
		if (inicio_espiracion){
			LedOn(LED_1);

			if (indice_de_capacidad_pulmonar >= umbral_objetivo) {
				LedOn(LED_2);
				LedOff(LED_3);
			} else if (indice_de_capacidad_pulmonar <= umbral_limite) {
				LedOff(LED_2);
				LedOn(LED_3);
			} else {
				LedOff(LED_2);
				LedOff(LED_3);
			}
		} else {
			LedOff(LED_1);
			LedOff(LED_2);
			LedOff(LED_3);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

// Función callback para BLE (maneja datos recibidos)
void BLE_Callback(uint8_t *data, uint8_t length){
    // agregar codigo para tomar datos
}

/**
 * @brief Envía por UART el flujo de aire espirado y la presión medida.
 */
void Comunicacion_Serie() {
	snprintf(msg, sizeof(msg), "%.3f m3/s\r\n", qv);
	UartSendString(UART_PC, msg);
	snprintf(msg, sizeof(msg), "%.2f Kpa\r\n", presion_Kpa);
	UartSendString(UART_PC, msg);
	snprintf(msg, sizeof(msg), "Indice FEV1/CVF = %.2f\r\n", indice_de_capacidad_pulmonar);
    UartSendString(UART_PC, msg);
	snprintf(msg, sizeof(msg), "tiempo total= %.2f\r\n", tiempo_total);
    UartSendString(UART_PC, msg);
	snprintf(msg, sizeof(msg), "volumen_FE1= %.2f\r\n", volumen_1s_FEV1);
    UartSendString(UART_PC, msg);
	snprintf(msg, sizeof(msg), "volumen_Total= %.2f\r\n", volumen_total_CVF);
    UartSendString(UART_PC, msg);

}

/**
 * @brief Envía el voltaje leído por el sensor al Serial Plotter.
 */
void Serial_Plotter(){

	voltaje_mv = Valor_voltaje();

    char *str_val = (char *)UartItoa(voltaje_mv, 10);
    UartSendString(UART_PC, ">voltaje:");
	UartSendString(UART_PC, str_val);
    UartSendString(UART_PC, "\r\n");
	

}

/**
 * @brief Tarea periódica que mide la presión, calcula el flujo, 
 * controla los LEDs, y envía datos por UART y BLE.
 * 
 * @param pvParameter No se utiliza.
 */

 static void MedirPresionTask(void *pvParameter){
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        presion_Kpa = MP3v5050ReadPressure_kPa();
        Detectar_Inicio_Fin_Espiracion();

        if (inicio_espiracion){
            CalcularFlujoEspiracion();
            Calculo_FEV1_CVF();

            Comunicacion_Serie();
            Serial_Plotter();

            // Enviar datos por BLE
            if (BleStatus() == BLE_CONNECTED) {
                snprintf(msg, sizeof(msg), "*G%.2f,%.3f*\n", presion_Kpa, qv);
                BleSendString(msg);

                snprintf(msg, sizeof(msg), "*B%.2f*\n", indice_de_capacidad_pulmonar);
                BleSendString(msg);
            }
			xTaskNotifyGive(ActualizarLedsTaskHandle);

        } else if (fin_espiracion){
            // Apagar LEDs
            LedOff(LED_1);
            LedOff(LED_2);
            LedOff(LED_3);

            // Reset de variables
            indice_de_capacidad_pulmonar = 0;
            volumen_total_CVF = 0;
            volumen_1s_FEV1 = 0;
            tiempo_total = 0;

            // Reset de flags para permitir una nueva maniobra
            inicio_espiracion = false;
            fin_espiracion = false;
            ciclos_sin_presion = 0;
        }
    }
}


/*==================[external functions definition]==========================*/

/**
 * @brief Función principal del programa. Inicializa periféricos, 
 * configura UART, BLE, temporizador y lanza la tarea de medición.
 */
void app_main(void){
	LedsInit();

	MP3v5050Init();
	SwitchesInit();
    SwitchActivInt(SWITCH_1, FuncTecla1, 0);
	
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
        .period = PERIODO_MUESTREO,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timerMedirPresion);

    xTaskCreate(&MedirPresionTask, "MedirPresionTask", 2048, NULL, 5, &MedirPresionTaskHandle);

	xTaskCreate(&ActualizarLedsTask, "ActualizacionLedsTask", 2048, NULL, 5, &ActualizarLedsTaskHandle);

    TimerStart(timerMedirPresion.timer);
	
}
/*==================[end of file]============================================*/