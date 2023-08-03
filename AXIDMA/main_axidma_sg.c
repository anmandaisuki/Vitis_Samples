// Includes
    #include <stdio.h>
    #include <stdlib.h>

    #include "xaxidma.h"
    #include "xparameters.h"
    #include "xil_cache.h"
    #include "xscugic.h"

    #include <xil_io.h>
    #include "sleep.h"

// Define
    #define INIT_SUCCESS 0
    #define INIT_FAILURE 1

    //DMA
    #define DMA_BUF_ADDRESS 0x00ff0000  // Refer to Adress Map
    #define DMA_BUF_LENGTH 500           // Length of buffer (byte)

    //BDRING
    #define BDRING_NUM 1                // The number of BD in BD ring
    #define BDRING_ADDRESS 0x00f10000  // Refer to Adress Map

    //BD setting
    //offset
    #define BUFFER_ADDRESS_OFFSET 0x08
    #define CONTROL_OFFSET 0x18
    #define STATUS_OFFSET 0x1c

    //mask
    #define RXSOF_MASK 0x4000000
    #define RXEOF_MASK 0x2000000

// prototype declaration
    static int init_DMA();
    static int init_GIC();
    static int init_BDRING();
    static void dmacplt_isr();

//global variable
    u32 status;
    // DMA
    XAxiDma         dma_inst;
    XAxiDma_Config *dma_cfg;
    //GIC
    XScuGic         gic_inst;
    XScuGic_Config *gic_cfg;
    // BD ring for SGDMA
    XAxiDma_BdRing *bdring_inst;
    XAxiDma_Bd     *bd_ptr;
	XAxiDma_Bd     *cur_bd_ptr;
    XAxiDma_Bd      bd_template;

int main(){

     status = init_DMA();
     if(status != XST_SUCCESS){
         print("ERROR! Failed to initialize DMA.\n\r");
     }

     status = init_BDRING();
     if(status != XST_SUCCESS){
         print("ERROR! Failed to initialize BDRING\n\r");
     }

    status = init_GIC();
    if(status != XST_SUCCESS){
        print("ERROR! Failed to initialize GIC.\n\r");
    }

    print("All Initialize is done\n\r");

    status = XAxiDma_BdRingStart(bdring_inst);
    if(status != XST_SUCCESS){
        // SGDMA is failed.
        print("SGDMA process is failed\n\r");
    }

    while(1){

        Xil_DCacheFlushRange((UINTPTR)rx_buf,4*100);
    }

    return 1;
}

static int init_DMA(){

    // Create instance
    dma_cfg = XAxiDma_LookupConfigBaseAddr(XPAR_AXI_DMA_0_BASEADDR);

    status = XAxiDma_CfgInitialize(&dma_inst, dma_cfg);
    if(status != XST_SUCCESS){
        return INIT_FAILURE;
    }

    // Check DMA mode
    if (!XAxiDma_HasSg(&dma_inst)){
         print("DMA is not configured as SG mode.\n\r");
    }

    print("DMA is configured as SG mode.\n\r");

    // Reset DMA
	XAxiDma_Reset(&dma_inst);
	while (!XAxiDma_ResetIsDone(&dma_inst)) {}

    // Enable DMA interrupts
    XAxiDma_IntrEnable(&dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);

    return INIT_SUCCESS;
}

static int init_GIC(){

    // Create instance
    gic_cfg = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
    status = XScuGic_CfgInitialize(&gic_inst, gic_cfg, gic_cfg->CpuBaseAddress);
    if(status != XST_SUCCESS){
        return INIT_FAILURE;
    }

    // Set interrupt priorities and trigger type
	XScuGic_SetPriorityTriggerType(&gic_inst, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR, 0, 0x3); // 4th arguments 0x3 indicates this is called with rising edge.

    // Connect isr with GIC
    status = XScuGic_Connect(&gic_inst, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR, (Xil_InterruptHandler)dmacplt_isr, &dma_inst);
     if(status != XST_SUCCESS){
        return INIT_FAILURE;
    }

    // Enable interrupts
	XScuGic_Enable(&gic_inst, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR);


    // Connection OS or CPU and GIC.
        // Initialize exception table and register the interrupt controller handler with exception table
        Xil_ExceptionInit();
        Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, (void *)&gic_inst);

        // Enable non-critical exceptions
        Xil_ExceptionEnable();

     return INIT_SUCCESS;

}

static int init_BDRING(){
    // Create instance
    bdring_inst = XAxiDma_GetRxRing(&dma_inst);

    // Enable interrupts
	XAxiDma_BdRingIntEnable(bdring_inst, XAXIDMA_IRQ_ALL_MASK);

    // Setup space for BD ring in memory
	status = XAxiDma_BdRingCreate(bdring_inst, BDRING_ADDRESS, BDRING_ADDRESS, XAXIDMA_BD_MINIMUM_ALIGNMENT, BDRING_NUM);
    if(status != XST_SUCCESS){
         print("BD ring create is failed.\n\r");
        return INIT_FAILURE;
    }


    // Initialize BD ring
    XAxiDma_BdClear(&bd_template);
    status = XAxiDma_BdRingClone(bdring_inst, &bd_template);
    if(status != XST_SUCCESS){
         print("BD ring clone is failed.\n\r");
        return INIT_FAILURE;
    }

    // Allocate BD ring to certain area to write/read BD and get pointer of first BD
    status = XAxiDma_BdRingAlloc(bdring_inst, BDRING_NUM, &bd_ptr);
    if(status != XST_SUCCESS){
         print("BD ring allocate is failed.\n\r");
        return INIT_FAILURE;
    }

    // Write BD
    cur_bd_ptr = bd_ptr;
    int bd_cnt;
    for ( bd_cnt = 0; bd_cnt < BDRING_NUM; bd_cnt++)
    {
        // Set buffer address on current BD
		XAxiDma_BdWrite(cur_bd_ptr,BUFFER_ADDRESS_OFFSET,DMA_BUF_ADDRESS);

        // Set buffer length and RXSOF/EOF bit
        int control_register_write_data = 0; // data to write to the controll register on BD

            // Set RXSOF and RXEOF
            if(bd_cnt == 0)
                control_register_write_data |= RXSOF_MASK;

            if(bd_cnt == BDRING_NUM -1)
                control_register_write_data |= RXEOF_MASK;

            // Set buffer length
                control_register_write_data |= DMA_BUF_LENGTH;

        XAxiDma_BdWrite(cur_bd_ptr,CONTROL_OFFSET,control_register_write_data);

        // Next BD
        cur_bd_ptr = (XAxiDma_Bd*)XAxiDma_BdRingNext(bdring_inst, cur_bd_ptr);
    }

    // // Setup interrupt coalescing so only 1 interrupt fires per BD ring transfer. Only activate when BD ring is more than 2
	// status = XAxiDma_BdRingSetCoalesce(bdring_inst, BDRING_NUM, 0);
    // if(status != XST_SUCCESS){
    //         return INIT_FAILURE;
    //     }

    // Pass the BD to hardware for transmission
	status = XAxiDma_BdRingToHw(bdring_inst, BDRING_NUM, bd_ptr);
    if(status != XST_SUCCESS){
         print("BD ring to Hardware is failed.\n\r");
            return INIT_FAILURE;
    }

    // Enable interrupts
	XAxiDma_BdRingIntEnable(bdring_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK));

    return INIT_SUCCESS;
}


static int init_GPIO(){
	status = XGpio_Initialize(&gpio_inst, XPAR_GPIO_0_DEVICE_ID);
	XGpio_SetDataDirection(&gpio_inst, 1, 0);
	XGpio_DiscreteClear(&gpio_inst, 1, 0xFFFFFFFF);

	XGpio_DiscreteWrite(&gpio_inst, 1, 0x00000000); //off
//	XGpio_DiscreteWrite(&gpio_inst, 1, 1); //on
	return INIT_SUCCESS;
}

// Interrupt service routine. This function is called by interrupt signal
static void dmacplt_isr(void* CallbackRef){
    // Disable interrupt
    XScuGic_Disable(&gic_inst, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR);
    // IOC_irq flag reset by writing. IOC_irq is write clear.
    u32 tmpBuf_DMASR = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x34);
    tmpBuf_DMASR |= 0x1000;
    Xil_Out32(XPAR_AXI_DMA_0_BASEADDR + 0x34,tmpBuf_DMASR);

    // write your code

    // Enable interrupt
    XScuGic_Enable(&gic_inst, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR);

}

