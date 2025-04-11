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
 *
 */

 #include <stdio.h>
 #include <stdint.h>
 #include <stdbool.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "led.h"
 #include "switch.h"
 /*==================[macros and definitions]=================================*/
 #define CONFIG_BLINK_PERIOD 100
 /*==================[internal data definition]===============================*/
 
 /*==================[internal functions declaration]=========================*/
 
 /*==================[external functions definition]==========================*/

enum {ON, OFF, TOGGLE};

 typedef struct
 {
	uint8_t mode; //ON, OFF, TOGGLE
	uint8_t n_led; //indica el número de led
	uint8_t n_ciclos; //cantidad de ciclos encendido/apagado
	uint16_t periodo; //indica el tiempo de cada ciclo
 } MyLeds;


 void funcion_leds(MyLeds *mis_leds){
	printf("Comienza funcion leds");
	int8_t retardo = mis_leds->periodo/CONFIG_BLINK_PERIOD;
	switch (mis_leds->mode)
	{
	case ON:
		LedOn(mis_leds->n_led);
	break;
	case OFF:
		LedOff(mis_leds->n_led);
	break;
	case TOGGLE:
		for (int i = 0; i < mis_leds->n_ciclos; i++) {
			LedToggle(mis_leds->n_led);
			for (int8_t j = 0; j < retardo; j++) {
				vTaskDelay(CONFIG_BLINK_PERIOD/portTICK_PERIOD_MS);
			}
		}
		LedToggle(mis_leds->n_led);
	break;
	default:
		printf("No válido");
	break;
	}
 }

 int app_main() {
	LedsInit();
	MyLeds my_leds = {TOGGLE, LED_1, 10, 1000}; 
	funcion_leds(&my_leds);
	return 0;

 }