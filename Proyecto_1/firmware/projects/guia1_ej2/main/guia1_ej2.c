/*! @mainpage Blinking switch
 *
 * \section genDesc General Description
 *
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
/*
void app_main(void){
    uint8_t teclas;
    LedsInit();
    SwitchesInit();

    while(1) {
        teclas = SwitchesRead();

        if (teclas == 0){
            LedOff(LED_1);
            LedOff(LED_2);
            LedOff(LED_3);
        }

        // Si se presiona SWITCH_1, titila LED_1
        if (teclas & SWITCH_1) {
            LedToggle(LED_1);
        }
        else {

        }


        // Si se presiona SWITCH_2, titila LED_2
        if (teclas & SWITCH_2) {
            LedToggle(LED_2);
        }

        // Si ambas teclas están presionadas simultáneamente, titila LED_3
        if ((teclas & SWITCH_1) && (teclas & SWITCH_2)) {
            LedOff(LED_1);
            LedOff(LED_2);
            LedToggle(LED_3);
        }

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
*/

void app_main(void){
	uint8_t teclas;
	LedsInit();
	SwitchesInit();
    while(1)    {
    	teclas  = SwitchesRead();
    	switch(teclas){
    		case SWITCH_1:
                LedOff(LED_2);
                LedOff(LED_3);
    			LedToggle(LED_1);
    		break;
    		case SWITCH_2:
                LedOff(LED_3);
                LedOff(LED_1);
    			LedToggle(LED_2);
    		break;
			case SWITCH_1 | SWITCH_2:
                LedOff(LED_1);
                LedOff(LED_2);
                LedToggle(LED_3);
    	}
	    //LedToggle(LED_3);
		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
	}
}


