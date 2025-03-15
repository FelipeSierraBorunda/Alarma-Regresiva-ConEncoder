#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "stdio.h"
// Definir pines de los segmentos del display
#define Seg_a GPIO_NUM_5
#define Seg_b GPIO_NUM_18
#define Seg_c GPIO_NUM_2
#define Seg_d GPIO_NUM_22
#define Seg_e GPIO_NUM_23
#define Seg_f GPIO_NUM_4
#define Seg_g GPIO_NUM_15
// Definir pines de los cátodos (para multiplexado)
#define Catodo_2 GPIO_NUM_19
#define Catodo_3 GPIO_NUM_21
// Definir pines de los pulsadores
#define BtnA GPIO_NUM_32
#define BtnB GPIO_NUM_33
#define Buzzer GPIO_NUM_25
#define LedRojo GPIO_NUM_14
#define LedVerde GPIO_NUM_12
#define Btn GPIO_NUM_13
// Matriz de segmentos para los dígitos 0-9
uint8_t Digitos[][7] =
 {
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,0,0,1,1}  // 9
};
//Apagar
void ApagarDisplays(void)
{
    gpio_set_level(Catodo_2, 1);
    gpio_set_level(Catodo_3, 1);
}
void AsignarSegmentos(uint8_t BCD_Value)
{
	gpio_set_level(Seg_a, Digitos[BCD_Value][0]);
	gpio_set_level(Seg_b, Digitos[BCD_Value][1]);
	gpio_set_level(Seg_c, Digitos[BCD_Value][2]);
	gpio_set_level(Seg_d, Digitos[BCD_Value][3]);
	gpio_set_level(Seg_e, Digitos[BCD_Value][4]);
	gpio_set_level(Seg_f, Digitos[BCD_Value][5]);
	gpio_set_level(Seg_g, Digitos[BCD_Value][6]);
}
//================================================== Variables Locales ==============================================
uint8_t Display, Unidades, Decenas;
int  Residuo; 
bool EstadoRegresivo = false;
volatile int direccion = 0;  // 1 = horario, -1 = antihorario, 0 = sin movimiento
volatile int Cuenta = 0;  // 1 = horario, -1 = antihorario, 0 = sin movimiento
volatile int CuentaBuzzer = 0;  // 1 = horario, -1 = antihorario, 0 = sin movimiento
//================================================== Funciones  ====================================================
//================================================== Logica del display
void actualizar_display()
{
	//Separa el numero en cifras para asignarlas a cada dispaly
	Decenas = Cuenta/10;
	Residuo = Cuenta%10;
	Unidades = Residuo%10;
	ApagarDisplays();
	//Asigna la cifra al display
	switch(Display)
	{
		case 0:
			AsignarSegmentos(Decenas);
			gpio_set_level(Catodo_2, 0);
		break;
		case 1:
			AsignarSegmentos(Unidades);
			gpio_set_level(Catodo_3, 0);
		break;
	}
	//Siguiente display
	Display++;
	//Solo existen 2 display, cuando llegue al segundo, se reinicia
	Display &= 1;
}
// Estado AlarmaDesarmada
void AlarmaDesarmada(void* arg) 
{
		gptimer_handle_t timeralarma = (gptimer_handle_t) arg;
        EstadoRegresivo=false;
		gpio_set_level(LedRojo, 1);
		gpio_set_level(LedVerde, 0);
		gpio_set_level(Buzzer, 0);
		gptimer_stop(timeralarma);
}
//================================================== interrupciones =================================================
//================================================== Display
// ISR para detectar dirección de giro
static void IRAM_ATTR encoder_isr_handler(void* arg) 
{
    int estado_A = gpio_get_level(BtnA);
    int estado_B = gpio_get_level(BtnB);
    if (estado_A == estado_B) 
    {		
		direccion = -1;  // Giro antihorario
        if (Cuenta>0){Cuenta=Cuenta-1;}
		else{Cuenta=Cuenta;}
     } 
     else 
     {
        direccion = 1;  // Giro horario
       	if (Cuenta<99){Cuenta=Cuenta+1;}
		else{Cuenta=Cuenta;}
     }
}
// Interrupcion de Actualizacion de alarma
static bool IRAM_ATTR ActualizacionDelDisplay(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
	BaseType_t high_task_awoken = pdFALSE;
	actualizar_display();
	return (high_task_awoken == pdTRUE); 		
}
//================================================== Boton y estado regresivo
static void IRAM_ATTR btn_isr_handler(void* arg) 
{
 	gptimer_handle_t timeralarma = (gptimer_handle_t) arg;
 	gpio_isr_handler_remove(Btn);
	if(EstadoRegresivo==false){
		gpio_isr_handler_remove(BtnA);
		gpio_set_level(LedRojo, 0);
		gpio_set_level(LedVerde, 1);
		EstadoRegresivo = true;											
		gptimer_set_raw_count(timeralarma, 0);			
		gptimer_start(timeralarma);
	}
	else if(EstadoRegresivo==true)
	{
		AlarmaDesarmada(timeralarma);
		gpio_isr_handler_add(BtnA, encoder_isr_handler, timeralarma);
		gpio_isr_handler_add(Btn, btn_isr_handler,timeralarma);
		CuentaBuzzer=0;
		if(Cuenta==0){
			Cuenta=1;
		}
	}
}
// Interrupcion de Actualizacion de alarma
static bool IRAM_ATTR InterrupcionCuentaAbajo(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
	BaseType_t high_task_awoken = pdFALSE;
	gpio_isr_handler_add(Btn, btn_isr_handler,(void*)timer);
 	if(Cuenta==0)
 	{
		if(CuentaBuzzer==6)
		{
		CuentaBuzzer=0;
		Cuenta=1;
		AlarmaDesarmada(timer);
		gpio_isr_handler_add(BtnA, encoder_isr_handler, timer);
		}
		else
		{
		gpio_set_level(Buzzer, 1);
		CuentaBuzzer=CuentaBuzzer+1;
		}
	}
	else{
		Cuenta=Cuenta-1;
	}
	return (high_task_awoken == pdTRUE); 		
}
//================================================== Main ==========================================================
void app_main(void)
{
// =========================================== 	Configuracion y habilitacion del timer ============================
  // Primero configuramos al timer de proposito general (64bits) para contar hacia arriba
	gptimer_config_t TimerConfiguracionGeneral = 
	{
		.clk_src = GPTIMER_CLK_SRC_APB,			 
		.direction = GPTIMER_COUNT_UP, 			
		.resolution_hz = 1000000,				 
	};
	// Declaramos el manejador del gptimer llamado Timerdisplay
	gptimer_handle_t TimerDisplay = NULL;		  	
	//Aplicamos Configuracion General
	gptimer_new_timer(&TimerConfiguracionGeneral, &TimerDisplay);  
	// Configuracion de la alarma
	gptimer_alarm_config_t AlarmaDisplay = 
	{
		.alarm_count = 1000,
		.reload_count = 0,
		.flags.auto_reload_on_alarm = true
	};
	// Aplicamos la configuracion de la alarma para el manejador
	gptimer_set_alarm_action(TimerDisplay, &AlarmaDisplay);
	// Establecemos que la alarma del manejador, active la siguiente interrupcion
	gptimer_event_callbacks_t CambioDeDisplay = 
	{
		.on_alarm = ActualizacionDelDisplay,
	};
	// Aplicamos el evento al manejador
	gptimer_register_event_callbacks(TimerDisplay, &CambioDeDisplay, NULL);
	// Habilitaos y empezamos el timer del display
	gptimer_enable(TimerDisplay);
	gptimer_start(TimerDisplay);
	//======================================== Timer de la alarma regresiva
	// 
	// 
	//
	//
 //
//dcsxzcxz
 	gptimer_handle_t TimerAlarma = NULL;
 	// Aplicamos la configuracion general al nuevo timer
	gptimer_new_timer(&TimerConfiguracionGeneral, &TimerAlarma); 
	gptimer_alarm_config_t AlarmaDescuenta =
	{
		.alarm_count = 500000,
		.reload_count = 0,
		.flags.auto_reload_on_alarm = true
	};
	
	// Aplicamos
	gptimer_set_alarm_action(TimerAlarma, &AlarmaDescuenta);
	// Establecemos Evento
	gptimer_event_callbacks_t CuentaAbajo =
	{
		.on_alarm = InterrupcionCuentaAbajo,
	};
	// Aplicamos evento
	gptimer_register_event_callbacks(TimerAlarma, &CuentaAbajo, NULL);
	// Habilitaos y empezamos el timer del display
	gptimer_enable(TimerAlarma);
// =========================================== 	Configuracion de entradas y salidas ============================	
  // Configuración de salidas
    gpio_config_t ConfiguracionDeLasSalidas = {
        .pin_bit_mask = (1ULL << Seg_a | 1ULL << Seg_b | 1ULL << Seg_c | 
                         1ULL << Seg_d | 1ULL << Seg_e | 1ULL << Seg_f | 
                         1ULL << Seg_g | 1ULL << Catodo_2 | 1ULL << Catodo_3|
                         1ULL << Buzzer| 1ULL << LedRojo| 1ULL << LedVerde),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&ConfiguracionDeLasSalidas);

    // Configuración de entradas
    gpio_config_t ConfiguracionDeLasEntradas = 
    {
        .pin_bit_mask = (1ULL << BtnA | 1ULL << BtnB | 1ULL << Btn),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE //dETECTAR CAMBIO DE ESTADO
    };
    
    gpio_config(&ConfiguracionDeLasEntradas);
    // =============================================== VARIABLES DE INICIO =======================================
    //El Led rojo empieza encendido
    gpio_set_level(LedRojo, 1);
    //Pagardisplay 
    ApagarDisplays();
    //Variable que acumula la cuenta
    Cuenta = 0;
    //Variable que dice en que display 
    Display = 0;		
    //==================================================== Interrupciones =========================================
	// 	Activamos el servicio de interruppciones
    gpio_install_isr_service(0);
    // Se declara quien activa la interrupcion y cual interrupcion (Encoder)
    gpio_isr_handler_add(BtnA, encoder_isr_handler, NULL);
    // Se declara quien activa la interrupcion y cual interrupcion (Boton)
    gpio_isr_handler_add(Btn, btn_isr_handler,(void*)TimerAlarma);
    // =============================================== LOGICA DEL DISPLAY =========================================
    while (true) 
    {
    	// Delay
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
  }
  
