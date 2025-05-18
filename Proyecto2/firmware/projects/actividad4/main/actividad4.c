/*! @mainpage Actividad 4
 *
 * @section descripcion_general General Description
 *
 * Esta aplicación resuelve dos problemas: 
 * 1) Digitaliza una señal analógica proveniente de un potenciometro utilizando el canal CH1 del microcontrolador a 500Hz
 * y envía los valores a través de UART a la PC en formato ASCII, para su visualización mediante un serial plotter
 * 2) Genera una señal de analogica de ECG, apartir de una señal digital muestreada a una frecuencia de 250 Hz
 * Ambos problemas son resueltos utilizando el conversor ADC integrado en el microprocesador
 *
 * 
 *  @section hardConn Conexión de Hardware
 *
 * | Periférico        | ESP32 (Pin)       |
 * |:-----------------:|:-----------------:|
 * | Entrada analógica | CH1 (GPIO_1)      |
 *
 * @section changelog Historial de Cambios
 *
 * | Fecha       | Descripción                    |
 * |:----------: |:------------------------------:|
 * | 12/09/2023  | Creación del documento         |
 *
 * @author Ivana Gasco (ivana.gasco@ingenieria.uner.edu.ar)
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "analog_io_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_mcu.h"
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/
/**
 * @def PERIODO_VISUALIZAR
 * @brief Periodo en microsegundos para temporización a 500Hz
 */
#define PERIODO_VISUALIZAR 40000    // microseg para 500Hz

/**
 * @def PERIODO_VISUALIZAR_2
 * @brief Periodo para la generación de la señal ECG
 */
#define PERIODO_VISUALIZAR_2 20000 

/**
 * @def TAMANO_BUFFER
 * @brief Tamaño del buffer con datos de la señal ECG 
 */
#define TAMANO_BUFFER 231

/*==================[internal data definition]===============================*/
/**
 * @param MuestreoTaskHandle
 * @brief Manejador de la tarea de muestreo analógico
 */
TaskHandle_t MuestreoTaskHandle = NULL;

/**
 * @param ECGTaskHandle
 * @brief Manejador de la tarea de reproducción ECG
 */
TaskHandle_t ECGTaskHandle = NULL;

/*==================[internal functions declaration]=========================*/
static void MuestreoTask(void *pvParameter);
static void ECGTask(void *pvParameter);

/**
 * @param muestra_ECG
 * @brief Índice actual de lectura en el buffer de ECG
 */
uint16_t muestra_ECG;

/**
 * @param ecg
 * @brief Buffer con valores precargados que simulan una señal ECG (brindado por la cátedra)
 */
const char ecg[TAMANO_BUFFER] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};
/*==================[external functions definition]==========================*/


/**
 * @brief Función invocada en la interrupción del temporizador A
 *
 * Notifica a la tarea de muestreo para que lea un valor del ADC
 * y lo envíe por UART
 */
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(MuestreoTaskHandle, NULL);  
}

/**
 * @brief Función invocada en la interrupción del temporizador B.
 *
 * Notifica a la tarea de ECG para que envíe el siguiente valor del buffer
 * a la salida analógica
 */
void FuncTimerB(void* param){
    vTaskNotifyGiveFromISR(ECGTaskHandle, NULL);  
}

/**
 * @brief Tarea de muestreo del canal analógico.
 *
 * Esta tarea espera una notificación del temporizador, lee el valor del canal CH1,
 * lo convierte a ASCII y envía el mismo por UART para su visualización en la PC
 */
static void MuestreoTask(void *pvParameter){
    uint16_t value_mv;

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Leer CH1
        AnalogInputReadSingle(CH1, &value_mv);

        // Convertir y enviar por UART
        char *str_val = (char *)UartItoa(value_mv, 10);
        UartSendString(UART_PC, ">muestra:");
		UartSendString(UART_PC, str_val);
        UartSendString(UART_PC, "\r\n");
    }
}

/**
 * @brief Tarea de reproducción de señal ECG
 *
 * Esta tarea espera una notificación del temporizador, y luego
 * escribe el siguiente valor del buffer en la salida analógica
 * Cuando se llega al final del buffer, se reinicia el índice, y vuelve a comenzar
 */
static void ECGTask(void *pvParameter){
	while (true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        AnalogOutputWrite(ecg[muestra_ECG]);
        muestra_ECG++;

        if (muestra_ECG >= TAMANO_BUFFER){
            muestra_ECG = 0;
        }
	}
} 


/**
 * @brief Función principal de la aplicación
 *
 * Inicializa UART, ADC y salida analógica
 * Crea las tareas y configura los temporizadores para  ejecutar 
 * las tareas con el periodo especificado segun corresponda
 */
void app_main(void){

    // Inicializar UART
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

    // Crear la tarea de muestreo
    xTaskCreate(MuestreoTask, "MuestreoTask", 2048, NULL, 5, &MuestreoTaskHandle);
	xTaskCreate(ECGTask, "ECGTask", 2048, NULL, 5, &ECGTaskHandle);	

    // Inicializar Timer
    timer_config_t timerFunc_A = {
        .timer = TIMER_A,
        .period = PERIODO_VISUALIZAR,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
	 timer_config_t timerFunc_B = {
        .timer = TIMER_B,
        .period = PERIODO_VISUALIZAR_2,
        .func_p = FuncTimerB,
        .param_p = NULL
    };
    TimerInit(&timerFunc_A);
	TimerInit(&timerFunc_B);
	TimerStart(timerFunc_A.timer);
	TimerStart(timerFunc_B.timer);

}
/*==================[end of file]============================================*/
