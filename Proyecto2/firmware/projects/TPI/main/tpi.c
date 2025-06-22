/*! @mainpage Trabajo Integrador Final
 *
 * @section genDesc General Description
 * 
 * Este proyecto consiste en un sistema embebido desarrollado sobre una placa ESP32-C6 
 * con el objetivo de medir parámetros funcionales respiratorios, en particular el índice FEV1/CVF, 
 * a partir del análisis de una maniobra espiratoria forzada. 
 * 
 * Para ello se emplea el sensor MP3v5050 conectado a una boquilla tipo Venturi para medir 
 * la presión diferencial durante la espiración. Con esta medición, el sistema calcula el flujo de aire 
 * espirado (Q) y estima el volumen espirado en el primer segundo (FEV1) y el volumen total espirado 
 * (CVF). Luego, determina el índice FEV1/CVF, que permite evaluar la funcionalidad pulmonar del paciente.
 * 
 * Los datos son transmitidos por UART para visualización en un monitor serial o Serial Plotter, 
 * y también enviados vía Bluetooth Low Energy (BLE) para su representación en tiempo real en una aplicación móvil. 
 * Además, el sistema cuenta con una interfaz visual mediante LEDs que indica si el resultado se encuentra dentro 
 * de un rango saludable.
 * 
 * El programa está diseñado para funcionar en tiempo real, con un muestreo periódico de 100 ms, 
 * y permite reiniciar la medición presionando un botón físico (SWITCH_1).
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	MP3v5050	| 	 CH_1 	    | 
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
/** @brief Período de muestreo del temporizador en microsegundos (100ms). */
#define PERIODO_MUESTREO 100000

/** @brief Cantidad de ciclos sin presión antes de marcar el fin de espiración. */
#define CICLOS_FIN_ESPIRACION 10

/** @brief Coeficiente de descarga del tubo Venturi. */
#define Cd 0.97

/** @brief Área de entrada del tubo (en m²). */
#define A1 7.55e-4  // m²

/** @brief Área de garganta del tubo (en m²). */
#define A2 5.027e-5 // m²

/** @brief Densidad del aire en Kg/m³. */
#define p 1.2

/** @brief Umbral de presión en kPa para detectar inicio de espiración. */
#define umbral 2.0

/** @brief Umbral de índice FEV1/CVF objetivo para LED verde. */
#define umbral_objetivo 70.0

/** @brief Umbral de índice FEV1/CVF bajo el cual se enciende LED rojo. */
#define umbral_limite 50.0

/*==================[internal data definition]===============================*/
/** @brief Handle de la tarea que mide presión. */
TaskHandle_t MedirPresionTaskHandle = NULL;

/** @brief Handle de la tarea que actualiza los LEDs. */
TaskHandle_t ActualizarLedsTaskHandle = NULL;

/** @brief Presión diferencial medida (en kPa). */
float presion_Kpa = 0;

/** @brief Flujo espirado en m³/s. */
float qv = 0;

/** @brief Voltaje leído en mV. */
uint16_t voltaje_mv;

/** @brief Período actual del temporizador. */
uint32_t periodo_actual = PERIODO_MUESTREO;

/** @brief Buffer para mensajes a enviar por UART o BLE. */
char msg[32];

/** @brief Bandera que indica si se detectó el inicio de espiración. */
bool inicio_espiracion;

/** @brief Bandera que indica si se detectó el fin de espiración. */
bool fin_espiracion;

/** @brief Volumen total espirado (CVF). */
float volumen_total_CVF = 0;

/** @brief Volumen espirado en el primer segundo (FEV1). */
float volumen_1s_FEV1 = 0;

/** @brief Delta de tiempo entre mediciones (en segundos). */
float dt = 0.1;

/** @brief Tiempo total transcurrido desde el inicio de espiración. */
float tiempo_total = 0;

/** @brief Índice FEV1/CVF expresado en porcentaje. */
float indice_de_capacidad_pulmonar;

/** @brief Contador de ciclos consecutivos sin presión. */
uint16_t ciclos_sin_presion = 0;

/*==================[internal functions declaration]=========================*/

/**
 * @brief Función de callback del temporizador. 
 * Notifica a la tarea de medición de presión cuando se alcanza el período.
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

/**
 * @brief Función asociada al SWITCH 1, el propósito es reiniciar los valores
 * del BLE para comenzar de nuevo, sin embargo no se pudo implementar correctamente
 * previamente se intentó enviar mediante un bucle for varios valores cero para reiniciar
 * sin conseguirlo y actualmente al presionar la TECLA 1 se produce una desconexión en BLE.
 */
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
		printf("reinicio");
		BleSendString(msg);
	}

}

/**
 * @brief Detecta el inicio y fin de la espiración según el valor de presión y ciclos.
 */
void Detectar_Inicio_Fin_Espiracion(){
	if (presion_Kpa >= umbral){
		inicio_espiracion = true;
		fin_espiracion = false;
		ciclos_sin_presion = 0;
	} else {
		ciclos_sin_presion++;
		if (ciclos_sin_presion > CICLOS_FIN_ESPIRACION) { 
			inicio_espiracion = false;
			fin_espiracion = true;

		}
	}
}

/**
 * @brief Calcula los volúmenes espirados (CVF y FEV1) e índice FEV1/CVF.
 */
void Calculo_FEV1_CVF(){
    float volumen = qv * 100; // Escalar para trabajar con valores más manejables, sino se pierden datos.

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

/**
 * @brief Tarea que actualiza los LEDs según el estado del índice FEV1/CVF.
 */
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
	}
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
 * @brief Tarea principal del sistema. Lee la presión, detecta espiración, 
 * calcula volumen y flujo, y transmite datos por UART y BLE.
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
        } else if (fin_espiracion){
            // Reset de variables
            indice_de_capacidad_pulmonar = 0;
            volumen_total_CVF = 0;
            volumen_1s_FEV1 = 0;
            tiempo_total = 0;

            // Reset de banderas booleanas
            inicio_espiracion = false;
            fin_espiracion = false;
            ciclos_sin_presion = 0;
        }
		xTaskNotifyGive(ActualizarLedsTaskHandle);
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
    .func_p = NULL
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

	xTaskCreate(&ActualizarLedsTask, "ActualizarLedsTask", 2048, NULL, 5, &ActualizarLedsTaskHandle);

    TimerStart(timerMedirPresion.timer);
	
}
/*==================[end of file]============================================*/