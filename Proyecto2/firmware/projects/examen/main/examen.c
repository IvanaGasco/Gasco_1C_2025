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
 * | 09/06/2025  | Creación del documento         |
 *
 * @author Ivana Gasco (ivana.gasco@ingenieria.uner.edu.ar)
 */


/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Hello world!\n");
}
/*==================[end of file]============================================*/