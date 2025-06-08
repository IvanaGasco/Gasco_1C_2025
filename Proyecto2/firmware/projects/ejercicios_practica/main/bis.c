/*==================[macros and definitions]=================================*/
#define PERIODO_MEDICIONES 3000000
#define PERIODO_COM_UART 5000000


/*==================[internal data definition]===============================*/
TaskHandle_t MedirHumedad_y_pHTaskHandle = NULL;
TaskHandle_t Comunicacion_UartTaskHandle = NULL;
float Medicion_pH;
float hold_pH;
float hold_Humedad;
bool valor_humedad = false;
bool iniciar_funcionamiento = false;
bool detener_funcionamineto = false;
bool activacion_bomba_agua = false;
bool activacion_bomba_pH_acido;
bool activacion_bomba_pH_basico;


/*==================[internal functions declaration]=========================*/
void FuncTimerA(void* param){
    vTaskNotifyGiveFromISR(MedirHumedad_y_pHTaskHandle, pdFALSE);    
}

void FuncTimerB(void* param){
    vTaskNotifyGiveFromISR(Comunicacion_UartTaskHandle, pdFALSE);    
}

void FuncTecla1() {
	iniciar_funcionamiento = !iniciar_funcionamiento;
}

void FuncTecla2() {
    detener_funcionamineto = !detener_funcionamineto;
    if (detener_funcionamineto == true) {
        hold_pH = Medicion_pH;
		hold_Humedad = valor_humedad;
    }
}

void FuncBombaAgua(){
	if (activacion_bomba_agua){
		Activar_Bomba_Agua();
	} else {
		Desactivar_Bomba_Agua();
	}
}



static void Comunicacion_UartTask(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		float valor_pH;
		bool humedad;
		if (detener_funcionamineto){
			valor_pH = hold_pH;
			humedad = hold_Humedad;
		} else{
			valor_pH = Medicion_pH;
			humedad = valor_humedad;
		}
		UartSendString(UART_PC, (char*)UartItoa(valor_pH, 10));
		if (activacion_bomba_pH_acido == true){
			UartSendString(UART_PC, "Bomba de solución ácida encendida (pH Alto)\r\n");
		} else if (activacion_bomba_pH_acido == false && activacion_bomba_pH_basico == false){
			UartSendString(UART_PC, "Bomba solución ácida y básica apagadas (pH Ideal)\r\n");
		} else if (activacion_bomba_pH_basico == true){
			UartSendString(UART_PC, "Bomba de solución básica encendida (pH Bajo)\r\n");
		}
		if (humedad == true){
			UartSendString(UART_PC, "Bomba de agua encendida (Humedad Baja)\r\n");
		} else {
			UartSendString(UART_PC, "Bomba de agua apagada (Humedad Ideal)\r\n");
		}
	}
}

static void MedirHumedad_y_pHTask(void *pvParameter){
	
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		float valor_pH;
		bool humedad;
		if (iniciar_funcionamiento) {
			Medicion_pH = Sensor_pH_Medir_pH();
			valor_humedad = Sensor_Humedad_Medir();
			if (detener_funcionamineto){
			valor_pH = hold_pH;
			humedad = hold_Humedad;
			} else{
			valor_pH = Medicion_pH;
			humedad = valor_humedad;
			}
			if (valor_pH < 6.0){
				activacion_bomba_pH_basico = true;
				activacion_bomba_pH_acido = false; 
			} else if (valor_pH > 6.7) {
				activacion_bomba_pH_acido = true;
				activacion_bomba_pH_basico = false;
			}
			if (humedad == true){
				activacion_bomba_agua = true;
			} else {
				activacion_bomba_agua = false;
			} 
		}
	}
}




/*==================[external functions definition]==========================*/
void app_main(void){

	SwitchesInit();
    SwitchActivInt(SWITCH_1, FuncTecla1, 0);
    SwitchActivInt(SWITCH_2, FuncTecla2, 0);

	timer_config_t timerMedirHumedad_y_pH = {
        .timer = TIMER_A,
        .period = PERIODO_MEDICIONES,
        .func_p = FuncTimerA,
        .param_p = NULL
    };
    TimerInit(&timerMedirHumedad_y_pH);

	timer_config_t timerComunicacionUart = {
        .timer = TIMER_B,
        .period = PERIODO_COM_UART,
        .func_p = FuncTimerB,
        .param_p = NULL
    };
    TimerInit(&timerComunicacionUart);

	serial_config_t uart_config = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL
	};	
	UartInit(&uart_config);

	xTaskCreate(&MedirHumedad_y_pHTask, "MedirHumedad_y_pHTask", 2048, NULL, 5, &MedirHumedad_y_pHTaskHandle);
	xTaskCreate(&Comunicacion_UartTask, "ComunicacionUartTask", 2048, NULL, 5, &Comunicacion_UartTaskHandle);
    
    TimerStart(timerMedirHumedad_y_pH.timer);
	TimerStart(timerComunicacionUart.timer);
}
/*==================[end of file]============================================*/