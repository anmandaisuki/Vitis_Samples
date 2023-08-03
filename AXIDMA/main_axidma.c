// Includes
    #include <stdio.h>
    #include <stdlib.h>

    #include "xaxidma.h"
    #include "xscugic.h"
    #include "xil_cache.h"
    #include "xparameters.h"

    #include "sleep.h"
    #include <xil_io.h>

// Define
    #define INIT_SUCCESS 0
    #define INIT_FAILURE 1
    #define BUFSIZE 100

// Prototype Declaration
    static int  init_DMA();
    static int  init_GIC();
    static void dmacplt_isr();

// Global variable
    u32 status;
    int rx_buf[BUFSIZE];

    // DMA
    XAxiDma         dma_inst;
    XAxiDma_Config *dma_cfg;
    //GIC
    XScuGic         gic_inst;
    XScuGic_Config *gic_cfg;

int main(){

    status = init_DMA();
     if(status != XST_SUCCESS){
         print("ERROR! Failed to initialize DMA.\n\r");
     }

    status = init_GIC();
    if(status != XST_SUCCESS){
        print("ERROR! Failed to initialize GIC.\n\r");
    }

    while(1){
        usleep(10);
        status = XAxiDma_SimpleTransfer(&dma_inst,(UINTPTR)rx_buf,4*BUFSIZE,XAXIDMA_DEVICE_TO_DMA);
        
        // You maybe need Reset to do DMA again. 
        // XAxiDma_Reset(&dma_inst);
    	// while (!XAxiDma_ResetIsDone(&dma_inst)) {}
        // XAxiDma_IntrEnable(&dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);

    }

}

// Initialize DMA
static int init_DMA(){

    // Create instance
    dma_cfg = XAxiDma_LookupConfigBaseAddr(XPAR_AXI_DMA_0_BASEADDR);

    status = XAxiDma_CfgInitialize(&dma_inst, dma_cfg);
    if(status != XST_SUCCESS){
        return INIT_FAILURE;
    }

    // Check DMA mode
    if (!XAxiDma_HasSg(&dma_inst)){
        print("DMA is NOT configured as SG mode.\n\r");
    } else {
        print("DMA is configured as SG mode.\n\r");
    }

    // Reset DMA
	XAxiDma_Reset(&dma_inst);
	while (!XAxiDma_ResetIsDone(&dma_inst)) {}

    // Enable DMA interrupts
    XAxiDma_IntrEnable(&dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);

    return INIT_SUCCESS;
}

// Initialize GIC
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

// Interrupt service routine. This function is called by interrupt signal
static void dmacplt_isr(void* CallbackRef){
    // Disable interrupt
    XScuGic_Disable(&gic_inst, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR);
    // IOC_irq flag reset by writing. IOC_irq is write clear.
    u32 tmpBuf_DMASR = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x34);
    tmpBuf_DMASR |= 0x1000;
    Xil_Out32(XPAR_AXI_DMA_0_BASEADDR + 0x34,tmpBuf_DMASR);

    // Write your code.

    // Enable interrupt
    XScuGic_Enable(&gic_inst, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR);
}