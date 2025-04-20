/*! @mainpage Guia 1 - Ejercicio 4
 *
 * @section genDesc Descripción del Código
 *
 * El siguiente programa convierte un número entero a un arreglo en BCD.
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 10/04/2025 | Document creation		                         |
 *
 * @author Ivana Gasco (ivana.gasco@ingenieria.uner.edu.ar)
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