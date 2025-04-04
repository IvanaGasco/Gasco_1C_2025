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

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/

#include <stdio.h>
#include <stdint.h>


int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number) {
    if (bcd_number == NULL) return -1;
    
    for (int i = digits - 1; i >= 0; i--) {
        bcd_number[i] = data % 10;
        data /= 10;
    }

    return 0;
}


void app_main(void) {
    uint8_t bcd_array[6]; 
    uint32_t number = 101123;
    uint8_t digits = 6;

    printf("Número a convertir: %lu\n", (unsigned long)number); 
    printf("Cantidad de dígitos: %u\n", digits);

    int8_t result = convertToBcdArray(number, digits, bcd_array);

    if (result == 0) {
        printf("Resultado en BCD: ");
        for (int i = 0; i < digits; i++) {
            printf("%u ", bcd_array[i]);  
        }
        printf("\n");
    } else if (result == -1) {
        printf("Error: Puntero inválido\n");
    } else {
        printf("Error: desconocido %d\n", result);
    }
}

/*==================[end of file]=============================================================*/