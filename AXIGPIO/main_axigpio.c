
// Includes
    #include <stdio.h>
    #include <stdlib.h>

    #include "xparameters.h"
    #include "xgpio.h"

    #include <xil_io.h>
    #include "sleep.h"

// prototype declaration
    static int init_GPIO();

//global variable
    u32 status;
    //GPIO
    XGpio gpio_inst;


int main(){

    init_GPIO();

    print("All Initialize is done\n\r");

    while(1){
        // waiting for interruption
    	usleep(10);
    	XGpio_DiscreteWrite(&gpio_inst, 1, 0xffffffff);
        //XGpio_DiscreteRead(&gpio_inst, 1);    // Input
        usleep(10);
    	XGpio_DiscreteWrite(&gpio_inst, 1, 0x00000000);
  
    }

    return 1;
}



static int init_GPIO(){
	status = XGpio_Initialize(&gpio_inst, XPAR_GPIO_0_DEVICE_ID);
	XGpio_SetDataDirection(&gpio_inst, 1, 0);     // Output
	// XGpio_SetDataDirection(&gpio_inst, 1, 1);  // Input
	XGpio_DiscreteClear(&gpio_inst, 1, 0xFFFFFFFF);

	XGpio_DiscreteWrite(&gpio_inst, 1, 0x00000000); //off
//	XGpio_DiscreteWrite(&gpio_inst, 1, 1); //on
	return INIT_SUCCESS;
}

