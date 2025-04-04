#include <stdio.h>
#include <stdint.h>
#include "gpio_mcu.h"

typedef struct {
    gpio_t pin;    
    io_t dir;       
} gpioConf_t;


int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number) {
    if (bcd_number == NULL) return -1;
    
    for (int i = digits - 1; i >= 0; i--) {
        bcd_number[i] = data % 10;
        data /= 10;
    }

    return 0;
}


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


void app_main(void) {
    // Configuración de los pines GPIO
    gpioConf_t gpioConfiguracion[4] = {
        {GPIO_20, GPIO_OUTPUT},
        {GPIO_21, GPIO_OUTPUT},
        {GPIO_22, GPIO_OUTPUT},
        {GPIO_23, GPIO_OUTPUT}
    };

    // Inicialización de los pines GPIO
    for (uint8_t i = 0; i < 4; i++) {
        GPIOInit(gpioConfiguracion[i].pin, gpioConfiguracion[i].dir);
    }

    // Conversión de número a BCD
    uint32_t numero = 4321;
    uint8_t digitos = 4;
    uint8_t bcd_result[4]; 

    int8_t resultado = convertToBcdArray(numero, digitos, bcd_result);

    if(resultado == 0) {
        printf("Conversión exitosa.\nDígitos BCD: ");
        for(uint8_t i = 0; i < digitos; i++) {
            printf("%d ", bcd_result[i]);
        }
        printf("\n");

        // Visualización de cada dígito en los GPIOs
        for(uint8_t i = 0; i < digitos; i++) {
            printf("\nMostrando dígito %d:\n", bcd_result[i]);
            setEstadoGPIO(bcd_result[i], gpioConfiguracion);
        }
    } else {
        printf("Error en la conversión: %d\n", resultado);
    }
}

/*==================================[end of file]=====================================================*/
