#include <stdio.h>
#include <stdint.h>
#include "gpio_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    gpio_t pin;
    io_t dir;
} gpioConf_t;

/**
 * @brief Convierte un número entero a un arreglo BCD
 * 
 * Esta función toma un número entero y lo convierte en su representación BCD,
 * de manera que almacena cada dígito decimal en un arreglo separado. 
 * Y el dígito más significativo se almacena en la posición más baja del arreglo.
 * 
 * @param data Es el número entero a convertir a BCD (máximo 10 dígitos para uint32_t)
 * @param digits Es la cantidad de dígitos a convertir (tamaño del arreglo bcd_number)
 * @param bcd_number Es el arreglo de salida donde se almacenarán los dígitos BCD
 * @return int8_t Retorna el estado de la operación: 
 *         - 0 si la conversión fue exitosa
 *         - -1 si el puntero bcd_number es NULL
 * @note El tamaño del arreglo bcd_number tiene que ser al menos igual a la cantidad de dígitos
 * @warning Si el número de dígitos solicitado es mayor que los dígitos del número, en tonces 
 * las posiciones restantes en el arreglo se completan con ceros.
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number) {
    if (bcd_number == NULL) return -1;
    
    for (int i = digits - 1; i >= 0; i--) {
        bcd_number[i] = data % 10;
        data /= 10;
    }

    return 0;
}

/**
 * @brief Establece el estado de los pines GPIO según el dígito de BCD
 * 
 * Esta función toma un dígito en formato BCD de 4 bits y establece el estado (alto/bajo) de 4 pines 
 * GPIO según el patrón de bits del dígito. 
 * De manera que cada bit del dígito BCD controla un pin GPIO correspondiente en el arreglo vectorGPIO.
 * 
 * @param digitoBCD Dígito BCD a decodificar
 * @param vectorGPIO Arreglo de estructuras de configuración GPIO. 
 * Cada elemento contiene la configuración de un pin GPIO.
 * 
 * @note La función itera sobre los 4 bits menos significativos del digitoBCD 
 * @warning El arreglo vectorGPIO debe contener al menos 4 elementos 
 * 
 */
void setEstadoGPIO(uint8_t digitoBCD, gpioConf_t *vectorGPIO) {
    for(uint8_t i = 0; i < 4; i++) {
        if (digitoBCD & 1) { 
            printf("Pin %d en alto\n", vectorGPIO[i].pin);
            GPIOOn(vectorGPIO[i].pin);
        } else {
            printf("Pin %d en bajo\n", vectorGPIO[i].pin);
            GPIOOff(vectorGPIO[i].pin);
        }
        digitoBCD = digitoBCD >> 1;
    }
}

/**
 * @brief Muestra un número en un display de 7 segmentos multiplexado
 * 
 * Esta función controla al display de 7 segmentos multiplexado para mostrar el número en decimal.
 * 
 * @param data Número a mostrar en el display
 * @param digits Cantidad de dígitos a mostrar 
 * @param segmentos Arreglo de configuraciones GPIO para los segmentos
 * @param digitos Arreglo de configuraciones GPIO para los dígitos
 * 
 * @return int8_t Retorna el estado de la operación:
 *         - 0 si la operación fue exitosa
 *         - -1 si algún puntero es inválido (no verificado actualmente)
 * 
 * @note La función realiza multiplexado por persistencia de visión con un delay de 5ms por dígito
 * @warning Los arreglos segmentos y digitos deben estar correctamente inicializados
 * 
 */
int8_t displayNumber(uint32_t data, uint8_t digits, gpioConf_t *segmentos, gpioConf_t *digitos) {
    uint8_t bcd_result[10];
    convertToBcdArray(data, digits, bcd_result);
    
    for (uint8_t i = 0; i < digits; i++) {
        setEstadoGPIO(bcd_result[i], segmentos);
        GPIOOn(digitos[i].pin);
        vTaskDelay(pdMS_TO_TICKS(5));
        GPIOOff(digitos[i].pin);
    }
    return 0;
}

/**
 * @brief Función principal muestra el funcionamiento del programa incluyendo las funciones recicladas.
 * 
 * Esta función muestra cómo funciona displayNumber() la cual muestra el número en el display de 7
 * segmentos multiplexado usando pines GPIO para controlar segmentos y dígitos.
 * 
 * @details La funcion realiza las siguientes funciones:
 * 1. Configura pines GPIO para los segmentos (4 pines) y dígitos (3 pines)
 * 2. Inicializa todos los GPIO necesarios como salidas
 * 3. Muestra el número en un display de 3 dígitos
 * 4. Añade un pequeño retardo para estabilidad del display
 * 
 * @warning Los números de pines GPIO deben coincidir con su configuración hardware real
 * 
 */
void app_main(void) {
   //Config GPIO para segmentos
    gpioConf_t segmentos[4] = {
        {GPIO_20, GPIO_OUTPUT}, 
        {GPIO_21, GPIO_OUTPUT},
        {GPIO_22, GPIO_OUTPUT}, 
        {GPIO_23, GPIO_OUTPUT}
    };
    
    // Config GPIO para dígitos
    gpioConf_t digitos[3] = {
        {GPIO_19, GPIO_OUTPUT},
        {GPIO_18, GPIO_OUTPUT}, 
        {GPIO_9, GPIO_OUTPUT}
    };

    for (uint8_t i = 0; i < 4; i++) {
        GPIOInit(segmentos[i].pin, segmentos[i].dir);
    }
    
    for (uint8_t i = 0; i < 3; i++) {
        GPIOInit(digitos[i].pin, digitos[i].dir);
    }

    displayNumber(831, 3, segmentos, digitos);
   
    //Delay
    vTaskDelay(pdMS_TO_TICKS(20));
}