//###########################################################################
// FILE:    Sustentation_Main.c
// TITLE:   Minimal ADC Position Debug
//###########################################################################

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include <stdint.h>

/* ========================================================================= *
 * GLOBAL VARIABLES (Volatile for debugger visibility)
 * ========================================================================= */
volatile uint16_t ADC_pos_1 = 0;
volatile uint16_t ADC_pos_2 = 0;
volatile uint16_t ADC_pos_3 = 0;
volatile uint16_t ADC_pos_4 = 0;

/* ========================================================================= *
 * FUNCTION PROTOTYPES
 * ========================================================================= */
void error(void);
void init(void);
__interrupt void adcA1ISR(void);
__interrupt void INT_Push_Button_Start_XINT_ISR(void);
__interrupt void INT_mySCI0_RX_ISR(void);
__interrupt void INT_mySCI0_TX_ISR(void);

/* ========================================================================= *
 * MAIN PROGRAM
 * ========================================================================= */
void main(void)
{
    init();

    while(1)
    {
        // Infinite loop - Execution happens in the ADC ISR
    }
}

void init(void)
{
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();

    // Initialize Board (Pins, ADC SOCs, Timers)
    Board_init();

    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    EINT;
    ERTM;
}

void error (void)
{
    ESTOP0;
}

/* ========================================================================= *
 * INTERRUPT SERVICE ROUTINES
 * ========================================================================= */
__interrupt void adcA1ISR(void)
{
    // --- 1. LECTURE BRUTE ADC (POSITION) ---
    ADC_pos_1 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
    ADC_pos_2 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1);
    ADC_pos_3 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER2);
    ADC_pos_4 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER3);

    // --- 2. ACQUITTEMENTS ADC ---
    ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);

    if(ADC_getInterruptOverflowStatus(myADC0_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(myADC0_BASE, ADC_INT_NUMBER1);
        ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);
    }

    Interrupt_clearACKGroup(INT_myADC0_1_INTERRUPT_ACK_GROUP);
}

// --- Coquilles vides pour prévenir les ISR Trap si activées par Board_init() ---
__interrupt void INT_Push_Button_Start_XINT_ISR(void)
{
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

__interrupt void INT_mySCI0_RX_ISR(void)
{
    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

__interrupt void INT_mySCI0_TX_ISR(void)
{
    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
