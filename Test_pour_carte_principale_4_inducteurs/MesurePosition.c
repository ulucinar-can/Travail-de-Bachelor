//###########################################################################
//
// FILE:    Sustentation_Main.c
//
// TITLE:   Power command and regulation for magnetic sustenance
//
// AUTHOR :
//          - Thomas Freyche - 2025
//          - Can Uluçinar   - 2026
//
//###########################################################################

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "sfo_v8.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "FunctionHeader2.h"

/* ========================================================================= *
 * CONFIGURATION DEFINES
 * ========================================================================= */
#define LIMITE_MAX_DUTY_FINE        80
#define LIMITE_MIN_DUTY_FINE        30
#define NUM_OF_PWM_CHANNEL          4
#define EPWM_TIMER_TBPRD            100UL
#define MIN_HRPWM_DUTY_PERCENT      4.0f/((float)EPWM_TIMER_TBPRD)*100.0f
#define TIME_BASE_PERIOD            599
#define INV_FACTOR                  0.01f
#define OFFSET_COUNT_INV            0.001f

#define POS_CORRECTION_1            1.07f
#define POS_CORRECTION_2            1.06f
#define TAKEOFF_CURRENT_STEP1       0.04f
#define TAKEOFF_CURRENT_STEP2       0.004f
#define POS_DETECT                  0.99f

#define COUNT_TO_REACH              10000

#define DUTY_CYCLE_1                50
#define DUTY_CYCLE_2                50
#define DUTY_CYCLE_3                50
#define DUTY_CYCLE_4                50

#define ADC_ZERO_CURRENT            1861.36f

#define RX_BUF_LEN                  64

#define ALPHA                       0.001f
#define ALPHA_INV                   0.999f

#define STATE_1                     1
#define STATE_2                     2
#define STATE_3                     3
#define STATE_4                     4
#define STATE_5                     5
#define STATE_6                     6
#define STATE_7                     7
#define STATE_8                     8

#define SKIP_BACK                   0
#define SKIP_FRONT                  0

#define GAIN_COR_1                  1.1092f
#define OFFSET_COR_1                (-0.0407f / 1000.0f)

#define GAIN_COR_2                  1.08f
#define OFFSET_COR_2                (-0.0244f / 1000.0f)

#define GAIN_COR_3                  1.062f
#define OFFSET_COR_3                (0.1138f / 1000.0f)

#define GAIN_COR_4                  1.078f
#define OFFSET_COR_4                (0.0402f / 1000.0f)

/* ========================================================================= *
 * GLOBAL VARIABLES
 * ========================================================================= */
// --- SFO Library ---
int MEP_ScaleFactor;

// --- ADC & Offset ---
bool Offset_stop = false;
float Offset_count = 0;
float Offset_ADC1 = 0, Offset_ADC2 = 0, Offset_ADC3 = 0, Offset_ADC4 = 0;
uint16_t ADC_pos_1, ADC_pos_2, ADC_pos_3, ADC_pos_4, ADC_cur_1, ADC_cur_2, ADC_cur_3, ADC_cur_4;

// --- System States ---
float Position1 = DELTA_0, Position2 = DELTA_0, Position3 = DELTA_0, Position4 = DELTA_0;
float Current1 = 0, Current2 = 0, Current3 = 0, Current4 = 0;
float dutyCycle1 = 0.5, dutyCycle2 = 0.5, dutyCycle3 = 0.5, dutyCycle4 = 0.5;
float uc1 = 0, uc2 = 0, uc3 = 0, uc4 = 0;
float mean1 = 0, mean2 = 0, mean3 = 0, mean4 = 0;
unsigned int dt_mean = 0;

// --- Shared Control Variables ---
float I = 1; // Conservée en variable pour pouvoir couper l'intégrateur en direct via debugger
float Kw = KW, Kd = KD, Kddot = KDDOT, Kr = KR, Kr_sans_int = KR_SANS_INT;

bool takeOff = true;
bool takeOff2 = true;
bool phase1 = true;
uint32_t i_store = 0;

// --- Inductor 1 Control ---
float ic1 = 0.0, ue1 = 0.0, integral_i1 = 0;
float fc1 = 0;
float pos1Buff[FILTWINDOW] = {[0 ... 8] = 0};
float v1 = 0, ep1 = 0, xr1 = 0, fce1 = 0, sum_vp1 = 0, fc1_prim = 0;
float Position_c1 = DELTA_0, Position_c1_dec = 0;

// --- Inductor 2 Control ---
float ic2 = 0, ue2 = 0, integral_i2 = 0;
float fc2 = 0;
float pos2Buff[FILTWINDOW] = {[0 ... 8] = 0};
float v2 = 0, ep2 = 0, xr2 = 0, fce2 = 0, sum_vp2 = 0, fc2_prim = 0;
float Position_c2 = DELTA_0, Position_c2_dec = 0;

// --- Inductor 3 Control ---
float ic3 = 0, ue3 = 0, integral_i3 = 0;
float fc3 = 0;
float pos3Buff[FILTWINDOW] = {[0 ... 8] = 0};
float v3 = 0, ep3 = 0, xr3 = 0, fce3 = 0, sum_vp3 = 0, fc3_prim = 0;
float Position_c3_dec = 0, Position2_c3 = DELTA_0;

// --- Inductor 4 Control ---
float ic4 = 0, ue4 = 0, integral_i4 = 0;
float fc4 = 0;
float pos4Buff[FILTWINDOW] = {[0 ... 8] = 0};
float v4 = 0, ep4 = 0, xr4 = 0, fce4 = 0, sum_vp4 = 0, fc4_prim = 0;
float Position_c4_dec = 0 ,Position2_c4 = DELTA_0;

// --- PID Variables (Archive for future tests) ---
//float Kp_pid = 2050;
//float Ki_pid = 100;
//float Kd_pid = -75e1;
//float K_antiwindupPID = 1;
//float dpos1 = 0, kep1 = 0;
//float dpos2 = 0, kep2 = 0;

// --- Filter Buffers ---
float fc1f = 0.0, IN1[Nb] ={[0 ... 2] = 0}, OUT1[Na] = {[0 ... 1] = 0};
float fc2f = 0.0, IN2[Nb] ={[0 ... 2] = 0}, OUT2[Na] = {[0 ... 1] = 0};
float fc3f = 0.0, IN3[Nb] ={[0 ... 2] = 0}, OUT3[Na] = {[0 ... 1] = 0};
float fc4f = 0.0, IN4[Nb] ={[0 ... 2] = 0}, OUT4[Na] = {[0 ... 1] = 0};

// --- PWM Management ---
float dutyFine = MIN_HRPWM_DUTY_PERCENT;
float duty_table[NUM_OF_PWM_CHANNEL] ={50, 50, 50, 50};
const float duty_cycle_table[NUM_OF_PWM_CHANNEL] = {DUTY_CYCLE_1, DUTY_CYCLE_2, DUTY_CYCLE_3, DUTY_CYCLE_4};
float count = 0;
uint32_t compCount = 0;
uint16_t i = 0, status;
const uint32_t ePWM[NUM_OF_PWM_CHANNEL] = {myEPWM1_BASE, myEPWM2_BASE, myEPWM3_BASE, myEPWM4_BASE};

// --- User Interface & Buttons ---
bool ButtonS2 = false, Ext_Int_Flag = false, state_PIN = false;
uint16_t count_ext_int = 0;

// --- Communication (UART) ---
volatile uint16_t UartCounter = 0;
volatile char txBuffer[TX_BUF_LEN];
volatile uint16_t txIndex = 0;
volatile uint16_t txLength = 0;

volatile char rxBuffer[RX_BUF_LEN];
volatile uint16_t rxIndex = 0;
static uint16_t dataIndex = 0;

// --- State machine variable ---
uint8_t state = STATE_1;
bool PosRegFlag1 = false;
bool PosRegFlag3 = false;

// --- Variable for value sending ---
float Pos1_filt = DELTA_0;
float Pos2_filt = DELTA_0;
float Pos3_filt = DELTA_0;
float Pos4_filt = DELTA_0;

float Cur1_filt = 0;
float Cur2_filt = 0;
float Cur3_filt = 0;
float Cur4_filt = 0;

/* ========================================================================= *
 * FUNCTION PROTOTYPES (Local)
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
    // Main init
    init();

    // Infinite loop
    while(1);
}

void init(void)
{
    // Initialize device clock and peripherals
    Device_init();

    // Disable pin locks and enable internal pull ups.
    Device_initGPIO();

    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    Interrupt_initModule();

    // Initialize the PIE vector table
    Interrupt_initVectorTable();

    // SFO Initialization
    while(status == SFO_INCOMPLETE)
    {
        status = SFO();
        if(status == SFO_ERROR)
        {
            error();
        }
    }

    // Disable sync
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    // Initialize the EPWM GPIO Pins, SCI and XBAR
    Board_init();
    SCI_enableInterrupt(mySCI0_BASE, SCI_INT_RXFF);

    // Enable sync and clock to PWM
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    EINT;
    ERTM;

    // Affection des PWMs
    for(i = 0;i < NUM_OF_PWM_CHANNEL;i++)
    {
        dutyFine = ((float)(duty_cycle_table[i]*TIME_BASE_PERIOD) * INV_FACTOR);
        count = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
        compCount = (count);
        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
    }

    // Extinctions LEDs
    GPIO_writePin(LED_D1,1);
    GPIO_writePin(LED_D2,1);
    GPIO_writePin(LED_D5,0);
    GPIO_writePin(LED_D6,0);
}

void error (void)
{
    ESTOP0;
}

/* ========================================================================= *
 * INTERRUPT SERVICE ROUTINES (ISRs)
 * ========================================================================= */

__interrupt void adcA1ISR(void)
{
    // Allumer LED de debug pour mesurer le temps d'exécution de la boucle
    GPIO_writePin(LED_D5, 1);

    /* --------------------------------------------------------------------- *
     * 1. LECTURE ADC & CALIBRATION
     * --------------------------------------------------------------------- */
    // --- Lecture de la position ---
    ADC_pos_1 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
    ADC_pos_2 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1);
    ADC_pos_3 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER2);
    ADC_pos_4 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER3);

    // --- Lecture du courant ---
    ADC_cur_1 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER0);
    ADC_cur_2 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER1);
    ADC_cur_3 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER2);
    ADC_cur_4 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER3);

    // --- Calibration de l'offset initial (0A = 1.5V = ~1861.36) ---
    if(Offset_count <= 999 && !Offset_stop)
    {
        Offset_ADC1 += ((float)ADC_cur_1) - ADC_ZERO_CURRENT;
        Offset_ADC2 += ((float)ADC_cur_2) - ADC_ZERO_CURRENT;
        Offset_ADC3 += ((float)ADC_cur_3) - ADC_ZERO_CURRENT;
        Offset_ADC4 += ((float)ADC_cur_4) - ADC_ZERO_CURRENT;
        Offset_count++;

        // Moyenne pour definir l'offset de mesure de courant (1000 échantillons)
        if(Offset_count > 999)
        {
            Offset_ADC1 = Offset_ADC1 * OFFSET_COUNT_INV;
            Offset_ADC2 = Offset_ADC2 * OFFSET_COUNT_INV;
            Offset_ADC3 = Offset_ADC3 * OFFSET_COUNT_INV;
            Offset_ADC4 = Offset_ADC4 * OFFSET_COUNT_INV;

            Offset_stop = true; // Arret de l'echantillonnage

            // Mesure de position une fois stabilisée
            Position_c1_dec = Position1 * POS_DETECT;
            Position_c2_dec = Position2 * POS_DETECT;
            Position_c3_dec = Position3 * POS_DETECT;
            Position_c4_dec = Position4 * POS_DETECT;
        }
    }

    /* --------------------------------------------------------------------- *
     * 2. CONVERSIONS PHYSIQUES
     * --------------------------------------------------------------------- */
    // --- Conversion 12 bits -> Position en mm ---
//    Position1 = (float)(ADC_pos_1) * CONV_POS2;
//    Position2 = (float)(ADC_pos_2) * CONV_POS2;
//    Position3 = (float)(ADC_pos_3) * CONV_POS2;
//    Position4 = (float)(ADC_pos_4) * CONV_POS2;

    // --- Conversion 12 bits -> Position en mm ---
      Position1 = ADC_pos_1;
      Position2 = ADC_pos_2;
      Position3 = ADC_pos_3;
      Position4 = ADC_pos_4;

//    Position1 = ((float)(ADC_pos_1) * CONV_POS2) * GAIN_COR_1 + OFFSET_COR_1;
//    Position2 = ((float)(ADC_pos_2) * CONV_POS2) * GAIN_COR_2 + OFFSET_COR_2;
//    Position3 = ((float)(ADC_pos_3) * CONV_POS2) * GAIN_COR_3 + OFFSET_COR_3;
//    Position4 = ((float)(ADC_pos_4) * CONV_POS2) * GAIN_COR_4 + OFFSET_COR_4;

    // --- Position filtré pour l'envoie ---
    Pos1_filt = (ALPHA * Position1) + (ALPHA_INV * Pos1_filt);
    Pos2_filt = (ALPHA * Position2) + (ALPHA_INV * Pos2_filt);
    Pos3_filt = (ALPHA * Position3) + (ALPHA_INV * Pos3_filt);
    Pos4_filt = (ALPHA * Position4) + (ALPHA_INV * Pos4_filt);

    // --- Conversion 12 bits -> Courant (TFE 2025) ---
    Current1  = (((float)(ADC_cur_1) - ADC_ZERO_CURRENT - Offset_ADC1) / (ADC_ZERO_CURRENT + Offset_ADC1)) * I_MAX;
    Current2  = (((float)(ADC_cur_2) - ADC_ZERO_CURRENT - Offset_ADC2) / (ADC_ZERO_CURRENT + Offset_ADC2)) * I_MAX;
    Current3  = (((float)(ADC_cur_3) - ADC_ZERO_CURRENT - Offset_ADC3) / (ADC_ZERO_CURRENT + Offset_ADC3)) * I_MAX;
    Current4  = (((float)(ADC_cur_4) - ADC_ZERO_CURRENT - Offset_ADC4) / (ADC_ZERO_CURRENT + Offset_ADC4)) * I_MAX;

    // --- Courant filtré pour l'envoie ---
    Cur1_filt = (0.0001f * Current1) + (0.9999f * Cur1_filt);
    Cur2_filt = (0.0001f * Current2) + (0.9999f * Cur2_filt);
    Cur3_filt = (0.0001f * Current3) + (0.9999f * Cur3_filt);
    Cur4_filt = (0.0001f * Current4) + (0.9999f * Cur4_filt);

    /* --------------------------------------------------------------------- *
     * 3. COMMUNICATION (TELEMETRIE)
     * --------------------------------------------------------------------- */
    UartCounter++;
    if (UartCounter >= 25000)
    {
        UartCounter = 0; // Reset du compteur (~1s)

        //Envoie des positions en mm
        //SendFloatAsText(Pos1_filt*1000.0f, Pos2_filt*1000.0f, Pos3_filt*1000.0f, Pos4_filt*1000.0f);

        //Envoie des positions en brut
        SendFloatAsText(Pos1_filt, Pos2_filt, Pos3_filt, Pos4_filt);

        //Envoie des courrants
        //SendFloatAsText(Cur1_filt, Cur2_filt, Cur3_filt, Cur4_filt);
    }

    /* --------------------------------------------------------------------- *
     * 6. ACQUITTEMENTS & FLAGS (ADC)
     * --------------------------------------------------------------------- */
    ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);
    if(ADC_getInterruptOverflowStatus(myADC0_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(myADC0_BASE, ADC_INT_NUMBER1);
        ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);
    }
    Interrupt_clearACKGroup(INT_myADC0_1_INTERRUPT_ACK_GROUP);

    // Eteindre LED de debug (fin de boucle de régulation)
    GPIO_writePin(LED_D5, 0);
}

/* ========================================================================= *
 * EXTERNAL INTERRUPT & UART ISRs
 * ========================================================================= */

__interrupt void INT_Push_Button_Start_XINT_ISR(void)
{
    Ext_Int_Flag = true;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

// --- UART Receive (RX) ---
__interrupt void INT_mySCI0_RX_ISR(void)
{
    uint16_t c;
    int b;

    while (SCI_getRxFIFOStatus(SCIA_BASE) > 0)
    {
        c = SCI_readCharBlockingFIFO(SCIA_BASE);
        if (rxIndex < RX_BUF_LEN) {
            rxBuffer[rxIndex++] = c;
        }

        if(c == '\x02'){
            dataIndex = rxIndex; // Début de trame
        }

        if(c == '\x03'){ // Fin de trame
            if(dataIndex > 0 && dataIndex < rxIndex - 1) {
                // Utilisation d'une évaluation booléenne directe !
                state_PIN = (rxBuffer[dataIndex] == '1');
            }
            // Reset du buffer
            rxIndex = 0;
            for(b = 0; b < RX_BUF_LEN; b++) {
                rxBuffer[b] = 0;
            }
        }
    }

    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

// --- UART Transmit (TX) ---
__interrupt void INT_mySCI0_TX_ISR(void)
{
    while (SCI_getTxFIFOStatus(SCIA_BASE) < SCI_FIFO_TX16 && txIndex < txLength)
    {
        SCI_writeCharBlockingFIFO(SCIA_BASE, txBuffer[txIndex++]);
    }

    if (txIndex >= txLength)
    {
        SCI_disableInterrupt(mySCI0_BASE, SCI_INT_TXFF); // Fin d'envoi
    }

    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
