#include "esp_event.h"
#include "driver/gpio.h"
#include "stdio.h"
// Definir los estados posibles del sistema
typedef enum {
    INICIAL,//0
    FINAL,//1
    PROGRESIVO,//2
    REGRESIVO,//3
    PAUSA//4
} TIPODEESTADO; //TIPODEESTADO ES EL TIPO DE VARIABLE
//Antes de pausar, era progresivo o regresivo
typedef enum {
    PROGRESIVA,//2
    REGRESIVA,//3
} TIPOPAUSA;
// Definir pines de los segmentos del display
#define Seg_a GPIO_NUM_5
#define Seg_b GPIO_NUM_18
#define Seg_c GPIO_NUM_2
#define Seg_d GPIO_NUM_22
#define Seg_e GPIO_NUM_23
#define Seg_f GPIO_NUM_4
#define Seg_g GPIO_NUM_15
#define Seg_dp GPIO_NUM_16
// Definir pines de los cátodos (para multiplexado)
#define Catodo_0 GPIO_NUM_4
#define Catodo_1 GPIO_NUM_2
#define Catodo_2 GPIO_NUM_19
#define Catodo_3 GPIO_NUM_21
// Definir pines de los pulsadores
#define BtnAzul GPIO_NUM_33
#define BtnRojo GPIO_NUM_35
#define BtnBlanco GPIO_NUM_32
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
    gpio_set_level(Catodo_0, 1);	
    gpio_set_level(Catodo_1, 1);
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
//================================================== Main ==========================================================
void app_main(void)
{
	// =========================================== 	Configuracion de entradas y salidas ============================
	    // Configuración de salidas
    gpio_config_t ConfiguracionDeLasSalidas = {
        .pin_bit_mask = (1ULL << Seg_a | 1ULL << Seg_b | 1ULL << Seg_c | 
                         1ULL << Seg_d | 1ULL << Seg_e | 1ULL << Seg_f | 
                         1ULL << Seg_g | 1ULL << Catodo_0 | 1ULL << Catodo_1 | 
                         1ULL << Catodo_2 | 1ULL << Catodo_3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&ConfiguracionDeLasSalidas);

    // Configuración de entradas
    gpio_config_t ConfiguracionDeLasEntradas = {
        .pin_bit_mask = (1ULL << BtnAzul | 1ULL << BtnRojo | 1ULL << BtnBlanco),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&ConfiguracionDeLasEntradas);
    ApagarDisplays();
     printf("Iniciando el Programa...\n");
    // =============================================== VARIABLES DE CONTROL ==========================================
    // Guarda el estado actual del boton
    int EstadoActualBtnAzul,EstadoActualBtnRojo,EstadoActualBtnBlanco;
    // Guarda el estado anterior del boton
    int EstadoAnteriorBtnAzul=0;
    int EstadoAnteriorBtnRojo=0;
    int EstadoAnteriorBtnBlanco=0;
    // Variable que indica que se presiono el boton
    int BtnAzulPresionado,BtnRojoPresionado,BtnBlancoPresionado;
    //Variables para cifras
    uint8_t Display, Unidades, Decenas;
    int Cuenta, Residuo;
    int MilisAcum = 0;
    //Variable que acumula la cuenta
    Cuenta = 0;
    //Variable que dice en que display 
    Display = 0;			
    //Empieza con el estado inicial
	TIPODEESTADO ESTADOACTUAL = INICIAL; // Estado inicial
	TIPOPAUSA TIPODEPAUSA=PROGRESIVA;		
   // ===================================================== FIN =====================================================   
    while (true) 
    {
		// =============================================== LOGICA DEL DISPLAY =======================================
		//Separa el numero en cifras para asignarlas a cada dispaly
    	Decenas = Cuenta/10;
    	Residuo = Cuenta%10;
    	Unidades = Residuo%10;
        ApagarDisplays();
        //Leemos el estado de los pulsadores
        EstadoActualBtnAzul = gpio_get_level(BtnAzul);
        EstadoActualBtnRojo = gpio_get_level(BtnRojo);
        EstadoActualBtnBlanco = gpio_get_level(BtnBlanco);
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
    	// ============================================= Contador de  milisegundos  ================================
    	//El retraso entre cada cambio de display
        vTaskDelay(1 / portTICK_PERIOD_MS);
        //Contamos cuantos milisegundos estan pasando
        MilisAcum++;
        // =================================== Detecta si se presiona un boton =====================================
        if (EstadoAnteriorBtnAzul==1 && EstadoActualBtnAzul==0)
        {
			BtnAzulPresionado=1;
		}
		if (EstadoAnteriorBtnRojo==1 && EstadoActualBtnRojo==0)
        {
			BtnRojoPresionado=1;
		}
		if (EstadoAnteriorBtnBlanco==1 && EstadoActualBtnBlanco==0)
        {
			BtnBlancoPresionado=1;
		}
        // =================================== Comportamiento de cada boton ===========================================
        if (BtnAzulPresionado == 1) 
            {  
                if (ESTADOACTUAL == INICIAL || ESTADOACTUAL == PAUSA) 
                {
                    ESTADOACTUAL = PROGRESIVO;
                } 
                else if (ESTADOACTUAL == PROGRESIVO) 
                {
                    ESTADOACTUAL = PAUSA;
                    TIPODEPAUSA = PROGRESIVA;
                }
            }
          else if (BtnRojoPresionado==1)
          {  
                if (ESTADOACTUAL == FINAL ||ESTADOACTUAL == PAUSA) 
                {
                    ESTADOACTUAL = REGRESIVO;
                } 
                else if (ESTADOACTUAL == REGRESIVO) 
                {
                    ESTADOACTUAL = PAUSA;
                    TIPODEPAUSA = REGRESIVA;
                }
		  }
		  else if (BtnBlancoPresionado==1)
          {  
                if (ESTADOACTUAL == PAUSA) 
            	{
					if(TIPODEPAUSA == PROGRESIVA )
					{
						ESTADOACTUAL = INICIAL;
					}
					else if (TIPODEPAUSA == REGRESIVA)
					{
						ESTADOACTUAL = FINAL;
					}
				}
		  }
        // =================================== Realiza el cambio en el display y cuenta el tiempo
		if (MilisAcum==500)
		{
			MilisAcum = 0;
			if(ESTADOACTUAL==PAUSA)
			{
					Cuenta = Cuenta;
			}
			else if (ESTADOACTUAL==PROGRESIVO)
			{
					if (Cuenta==99){
						Cuenta= Cuenta;
						ESTADOACTUAL = FINAL;
					}else{
						Cuenta =Cuenta+1;
					}
			}
			else if(ESTADOACTUAL==REGRESIVO)
			{
					if (Cuenta==0){ Cuenta= Cuenta;	ESTADOACTUAL = INICIAL;
					}else{
						Cuenta =Cuenta-1;
					}
			}
			else if(ESTADOACTUAL==INICIAL)
			{
					Cuenta = 0;
			}
			else if(ESTADOACTUAL==FINAL)
			{
					Cuenta = 99;
			}
			// ================================================ Impresion de datos
			printf("==============================\n");
			printf("El numero es: %d\n", Cuenta);
			printf("El boton azul: %d\n", EstadoActualBtnAzul);
			printf("El boton rojo: %d\n", EstadoActualBtnRojo);
			printf("El boton blanco: %d\n", EstadoActualBtnBlanco);
			printf("ESTADO: %d\n", ESTADOACTUAL);
		}
		// ===================================== Se establece el estado anterior para el siguiente ciclo
		EstadoAnteriorBtnAzul=EstadoActualBtnAzul;
		EstadoAnteriorBtnRojo=EstadoActualBtnRojo;
		EstadoAnteriorBtnBlanco=EstadoActualBtnBlanco;
		// Se reinicia el estado de los botones
		BtnAzulPresionado=0;
		BtnRojoPresionado=0;
		BtnBlancoPresionado=0;
    }
  }
  //cod