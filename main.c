#include "esp_event.h"
#include "driver/gpio.h"
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
uint8_t Digitos[][7] = {
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
void ApagarDisplays(void){
    gpio_set_level(Catodo_2, 1);
    gpio_set_level(Catodo_3, 1);
}
void AsignarSegmentos(uint8_t BCD_Value){
	gpio_set_level(Seg_a, Digitos[BCD_Value][0]);
	gpio_set_level(Seg_b, Digitos[BCD_Value][1]);
	gpio_set_level(Seg_c, Digitos[BCD_Value][2]);
	gpio_set_level(Seg_d, Digitos[BCD_Value][3]);
	gpio_set_level(Seg_e, Digitos[BCD_Value][4]);
	gpio_set_level(Seg_f, Digitos[BCD_Value][5]);
	gpio_set_level(Seg_g, Digitos[BCD_Value][6]);
}
//================================================== interrupcion ==================================================
volatile int direccion = 0;  // 1 = horario, -1 = antihorario, 0 = sin movimiento
volatile int Cuenta = 0;  // 1 = horario, -1 = antihorario, 0 = sin movimiento
// ISR para detectar dirección de giro
static void IRAM_ATTR encoder_isr_handler(void* arg) {
    int estado_A = gpio_get_level(BtnA);
    int estado_B = gpio_get_level(BtnB);
    	if (estado_A == estado_B) 
    	{		
				direccion = -1;  // Giro antihorario
                if (Cuenta>0){
           		Cuenta=Cuenta-1;
				}
				else{Cuenta=Cuenta;}
        } 
        else {
               direccion = 1;  // Giro horario
            	if (Cuenta<99){
					Cuenta=Cuenta+1;
				}
				else{Cuenta=Cuenta;}
        }
}
// ISR para detectar cuando se presiona Btn
static void IRAM_ATTR btn_isr_handler(void* arg) 
{
  
}
//================================================== Main ==========================================================
void app_main(void)
{
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
    gpio_config_t ConfiguracionDeLasEntradas = {
        .pin_bit_mask = (1ULL << BtnA | 1ULL << BtnB | 1ULL << Btn),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE //dETECTAR CAMBIO DE ESTADO
    };
    gpio_config(&ConfiguracionDeLasEntradas);
    // =============================================== VARIABLES DE CONTROL ==========================================
    //Variables para cifras
    uint8_t Display, Unidades, Decenas;
    int  Residuo; 
    //Pagardisplay 
     ApagarDisplays();
    //Variable que acumula la cuenta
    Cuenta = 0;
    //Variable que dice en que display 
    Display = 0;		
    //==================================================== Interrupciones ============================================
	// Configurar interrupción en A
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BtnA, encoder_isr_handler, NULL);
    // Configurar interrupción en Btn
    gpio_isr_handler_add(Btn, btn_isr_handler, NULL);   
    while (true) 
    {
		// =============================================== LOGICA DEL DISPLAY =======================================
		//Separa el numero en cifras para asignarlas a cada dispaly
    	Decenas = Cuenta/10;
    	Residuo = Cuenta%10;
    	Unidades = Residuo%10;
        ApagarDisplays();
        //Asigna la cifra al display
    	switch(Display){
    		case 0:
    			AsignarSegmentos(Decenas);
    			gpio_set_level(Catodo_2, 0);// Enciende las Unidades
    			break;
    		case 1:
    			AsignarSegmentos(Unidades);
    			gpio_set_level(Catodo_3, 0);// Enciende las Unidades
    			break;
    	}
    	//Siguiente display
    	Display++;
    	//Solo existen 2 display, cuando llegue al segundo, se reinicia
    	Display &= 1;
    	// ============================================= Delay  ===================================================
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
  }
  
