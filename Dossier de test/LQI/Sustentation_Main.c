//###########################################################################
//
// FILE:    Sustentation_Main.c
//
// TITLE:   Power command and regulation for magnetic sustenance
//
// AUTHOR :
//          - Can Ulu inar   - 2026
//
//###########################################################################

#include <FunctionHeader.h>
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "sfo_v8.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

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

#define COUNT_TO_REACH              500

#define DUTY_CYCLE_1                50
#define DUTY_CYCLE_2                50
#define DUTY_CYCLE_3                50
#define DUTY_CYCLE_4                50

#define ADC_ZERO_CURRENT            2047.5f

#define RX_BUF_LEN                  64

#define ALPHA                       0.001f
#define ALPHA_INV                   0.999f

#define STATE_0                     0
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

#define GAIN_COR_1                  8.6659e-7f
#define OFFSET_COR_1                2.0069e-4f

#define GAIN_COR_2                  8.5282e-7f
#define OFFSET_COR_2                2.5155e-4f

#define GAIN_COR_3                  8.1712e-7f
#define OFFSET_COR_3                4.2764e-4f

#define GAIN_COR_4                  8.3439e-7f
#define OFFSET_COR_4                3.9979e-4f

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
float I = 1; // Conserv e en variable pour pouvoir couper l'int grateur en direct via debugger
float Kw = KW, Kd = KD, Kddot = KDDOT, Kr = KR, Kr_sans_int = KR_SANS_INT;

bool takeOff = true;
bool takeOff2 = true;
bool phase1 = true;
uint32_t i_store = 0;

// --- Inductor 1 Control ---
float ic1 = 0.0, ue1 = 0.0, integral_i1 = 0;
float fc1 = 0;
float v1 = 0, ep1 = 0, xr1 = 0, fce1 = 0, sum_vp1 = 0, fc1_prim = 0;
float Position_c1 = DELTA_0, Position_c1_dec = 2.85e-3f;

// --- Inductor 2 Control ---
float ic2 = 0, ue2 = 0, integral_i2 = 0;
float fc2 = 0;
float v2 = 0, ep2 = 0, xr2 = 0, fce2 = 0, sum_vp2 = 0, fc2_prim = 0;
float Position_c2 = DELTA_0, Position_c2_dec = 2.85e-3f;

// --- Inductor 3 Control ---
float ic3 = 0, ue3 = 0, integral_i3 = 0;
float fc3 = 0;
float v3 = 0, ep3 = 0, xr3 = 0, fce3 = 0, sum_vp3 = 0, fc3_prim = 0;
float Position_c3_dec = 2.85e-3f, Position_c3 = DELTA_0;

// --- Inductor 4 Control ---
float ic4 = 0, ue4 = 0, integral_i4 = 0;
float fc4 = 0;
float v4 = 0, ep4 = 0, xr4 = 0, fce4 = 0, sum_vp4 = 0, fc4_prim = 0;
float Position_c4_dec = 2.85e-3f ,Position_c4 = DELTA_0;

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
bool SystemOn = false, Ext_Int_Flag = false;
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
uint8_t state = STATE_0;
bool PosRegFlag1 = false;
bool PosRegFlag3 = false;

// --- Variable for filtrered value sending ---
float Pos1_filt = DELTA_0;
float Pos2_filt = DELTA_0;
float Pos3_filt = DELTA_0;
float Pos4_filt = DELTA_0;

float Cur1_filt = 0;
float Cur2_filt = 0;
float Cur3_filt = 0;
float Cur4_filt = 0;

// --- Variables Observateurs ---
float pos_est1 = DELTA_0, vit_est1 = 0.0f, for_est1 = 0.0f, err_obs1 = 0.0f;
float pos_est2 = DELTA_0, vit_est2 = 0.0f, for_est2 = 0.0f, err_obs2 = 0.0f;
float pos_est3 = DELTA_0, vit_est3 = 0.0f, for_est3 = 0.0f, err_obs3 = 0.0f;
float pos_est4 = DELTA_0, vit_est4 = 0.0f, for_est4 = 0.0f, err_obs4 = 0.0f;


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


    // Infinite loop
    while(1);
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
    // Allumer LED de debug pour mesurer le temps d'ex cution de la boucle
    //GPIO_writePin(LED_D5, 1);

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

        // Moyenne pour definir l'offset de mesure de courant (1000  chantillons)
        if(Offset_count > 999)
        {
            Offset_ADC1 = Offset_ADC1 * OFFSET_COUNT_INV;
            Offset_ADC2 = Offset_ADC2 * OFFSET_COUNT_INV;
            Offset_ADC3 = Offset_ADC3 * OFFSET_COUNT_INV;
            Offset_ADC4 = Offset_ADC4 * OFFSET_COUNT_INV;

            Offset_stop = true; // Arret de l'echantillonnage
        }
    }

    /* --------------------------------------------------------------------- *
     * 2. CONVERSIONS PHYSIQUES
     * --------------------------------------------------------------------- */
    // --- Conversion 12 bits -> Position (Ecart int gr ) ---
    Position1 = ((float)(ADC_pos_1) * CONV_POS2);
    Position2 = ((float)(ADC_pos_2) * CONV_POS2);
    Position3 = ((float)(ADC_pos_3) * CONV_POS2);
    Position4 = ((float)(ADC_pos_4) * CONV_POS2);

//    Position1 = apply_poly5((float)(ADC_pos_1), POS_COR_1);
//    Position2 = apply_poly5((float)(ADC_pos_2), POS_COR_2);
//    Position3 = apply_poly5((float)(ADC_pos_3), POS_COR_3);
//    Position4 = apply_poly5((float)(ADC_pos_4), POS_COR_4);

//    Position1 = ((float)(ADC_pos_1)) * GAIN_COR_1 + OFFSET_COR_1;
//    Position2 = ((float)(ADC_pos_2)) * GAIN_COR_2 + OFFSET_COR_2;
//    Position3 = ((float)(ADC_pos_3)) * GAIN_COR_3 + OFFSET_COR_3;
//    Position4 = ((float)(ADC_pos_4)) * GAIN_COR_4 + OFFSET_COR_4;

    // ================================================================= //
    // OBSERVATEURS D'ÉTAT (Remplacement du filtre de vitesse)
    // Note : fc1, fc2... contiennent la commande du cycle précédent,
    // ce qui est physiquement correct pour prédire l'état actuel !
    // ================================================================= //

    // --- Inducteur 1 ---
    err_obs1 = Position1 - pos_est1;
    float pos_est1_new = AD11_1 * pos_est1 + AD12_1 * vit_est1                     + L1_1 * err_obs1;
    float vit_est1_new =                     AD22_1 * vit_est1 + AD23_1 * for_est1 + L2_1 * err_obs1;
    float for_est1_new =                                         AD33_1 * for_est1 + BD3_1 * (fc1 - FP) + L3_1 * err_obs1;
    pos_est1 = pos_est1_new; vit_est1 = vit_est1_new; for_est1 = for_est1_new;

    // --- Inducteur 2 ---
    err_obs2 = Position2 - pos_est2;
    float pos_est2_new = AD11_2 * pos_est2 + AD12_2 * vit_est2                     + L1_2 * err_obs2;
    float vit_est2_new =                     AD22_2 * vit_est2 + AD23_2 * for_est2 + L2_2 * err_obs2;
    float for_est2_new =                                         AD33_2 * for_est2 + BD3_2 * (fc2 - FP) + L3_2 * err_obs2;
    pos_est2 = pos_est2_new; vit_est2 = vit_est2_new; for_est2 = for_est2_new;

    // --- Inducteur 3 ---
    err_obs3 = Position3 - pos_est3;
    float pos_est3_new = AD11_3 * pos_est3 + AD12_3 * vit_est3                     + L1_3 * err_obs3;
    float vit_est3_new =                     AD22_3 * vit_est3 + AD23_3 * for_est3 + L2_3 * err_obs3;
    float for_est3_new =                                         AD33_3 * for_est3 + BD3_3 * (fc3 - FP) + L3_3 * err_obs3;
    pos_est3 = pos_est3_new; vit_est3 = vit_est3_new; for_est3 = for_est3_new;

    // --- Inducteur 4 ---
    err_obs4 = Position4 - pos_est4;
    float pos_est4_new = AD11_4 * pos_est4 + AD12_4 * vit_est4                     + L1_4 * err_obs4;
    float vit_est4_new =                     AD22_4 * vit_est4 + AD23_4 * for_est4 + L2_4 * err_obs4;
    float for_est4_new =                                         AD33_4 * for_est4 + BD3_4 * (fc4 - FP) + L3_4 * err_obs4;
    pos_est4 = pos_est4_new; vit_est4 = vit_est4_new; for_est4 = for_est4_new;

    // --- Position filtr  pour l'envoie ---
    Pos1_filt = (ALPHA * Position1) + (ALPHA_INV * Pos1_filt);
    Pos2_filt = (ALPHA * Position2) + (ALPHA_INV * Pos2_filt);
    Pos3_filt = (ALPHA * Position3) + (ALPHA_INV * Pos3_filt);
    Pos4_filt = (ALPHA * Position4) + (ALPHA_INV * Pos4_filt);

    // --- Conversion 12 bits -> Courant (TFE 2025) ---
    Current1  = (((float)(ADC_cur_1) - ADC_ZERO_CURRENT - Offset_ADC1) / (ADC_ZERO_CURRENT + Offset_ADC1)) * I_MAX;
    Current2  = (((float)(ADC_cur_2) - ADC_ZERO_CURRENT - Offset_ADC2) / (ADC_ZERO_CURRENT + Offset_ADC2)) * I_MAX;
    Current3  = (((float)(ADC_cur_3) - ADC_ZERO_CURRENT - Offset_ADC3) / (ADC_ZERO_CURRENT + Offset_ADC3)) * I_MAX;
    Current4  = (((float)(ADC_cur_4) - ADC_ZERO_CURRENT - Offset_ADC4) / (ADC_ZERO_CURRENT + Offset_ADC4)) * I_MAX;

    // --- Courant filtr  pour l'envoie ---
    Cur1_filt = (0.0001f * Current1) + (0.9999f * Cur1_filt);
    Cur2_filt = (0.0001f * Current2) + (0.9999f * Cur2_filt);
    Cur3_filt = (0.0001f * Current3) + (0.9999f * Cur3_filt);
    Cur4_filt = (0.0001f * Current4) + (0.9999f * Cur4_filt);

    /* --------------------------------------------------------------------- *
     * 3. COMMUNICATION (TELEMETRIE)mon
     * --------------------------------------------------------------------- */
    UartCounter++;
    if (UartCounter >= 250) // Envoi   100 Hz
    {
        UartCounter = 0;

        float telemetry[32]; // Passage   32 variables

        // --- Inducteur 1 ---
        telemetry[0] = Position_c1 * 1000.0f;
        telemetry[1] = Position1 * 1000.0f;
        telemetry[2] = ep1 * 1000.0f;          // Erreur de position en mm
        telemetry[3] = ic1;
        telemetry[4] = Current1;
        telemetry[5] = fc1;
        telemetry[6] = xr1;
        telemetry[7] = vit_est1;

        // --- Inducteur 2 ---
        telemetry[8] = Position_c2 * 1000.0f;
        telemetry[9] = Position2 * 1000.0f;
        telemetry[10] = ep2 * 1000.0f;
        telemetry[11] = ic2;
        telemetry[12] = Current2;
        telemetry[13] = fc2;
        telemetry[14] = xr2;
        telemetry[15] = vit_est2;

        // --- Inducteur 3 ---
        telemetry[16] = Position_c3 * 1000.0f;
        telemetry[17] = Position3 * 1000.0f;
        telemetry[18] = ep3 * 1000.0f;
        telemetry[19] = ic3;
        telemetry[20] = Current3;
        telemetry[21] = fc3;
        telemetry[22] = xr3;
        telemetry[23] = vit_est3;

        // --- Inducteur 4 ---
        telemetry[24] = Position_c4 * 1000.0f;
        telemetry[25] = Position4 * 1000.0f;
        telemetry[26] = ep4 * 1000.0f;
        telemetry[27] = ic4;
        telemetry[28] = Current4;
        telemetry[29] = fc4;
        telemetry[30] = xr4;
        telemetry[31] = vit_est4;

        Send32FloatsAsCSV(telemetry);
    }

    /* --------------------------------------------------------------------- *
     * 4. MACHINE D'ETAT & REGULATION
     * --------------------------------------------------------------------- */

    // ================================================================= //
    // A. STATE MACHINE BEGINING
    // ================================================================= //

    // Permet de vÃ©rifier si l'on veut Ã©teindre la maquette Ã  n'importe quel moment
    if (!SystemOn && (state != STATE_0) && (state != STATE_5))
    {
        state = STATE_4;
    }

    switch(state)
    {
        // State 0 :
        // Attente de la pression du bouton
        case STATE_0 :

            if(SystemOn)
            {
                GPIO_writePin(LED_D2, 0); // Allumage LED2 (Indicateur sustentation)

                // V rifie si l'on veut uniquement lever l'arri re
                state = SKIP_FRONT ? STATE_4 : STATE_1;
            }

        break;

        // State 1 :
        // Rampe de courant sur les inducteur 1&2 jusqu'  atteindre les 3A
        case STATE_1:

            // Rampe vers 3A par pas de 0.04A
            if(ic1 < I_SP)
            {
                ic1 += TAKEOFF_CURRENT_STEP1;
            }
            else
            {
                ic1 = I_SP;
            }

            // Fixe le courant de consigne 2,3,4 par rapport   ic1
            ic2 = ic1;
            ic3 = ic1;
            ic4 = ic1;

            mean1 += Current1;
            mean2 += Current2;
            mean3 += Current3;
            mean4 += Current4;

            dt_mean++;

            if(dt_mean >= 200) // dt = 8ms
            {
                mean1 = mean1 / dt_mean;
                mean2 = mean2 / dt_mean;
                mean3 = mean3 / dt_mean;
                mean4 = mean4 / dt_mean;

                if((mean1 <= I_SP105 && mean1 >= I_SP095) && (mean2 <= I_SP105 && mean2 >= I_SP095)
                        && (mean3 <= I_SP105 && mean3 >= I_SP095) && (mean4 <= I_SP105 && mean4 >= I_SP095))
                {
                    // Reset des variables
                    mean1 = 0;
                    mean2 = 0;
                    mean3 = 0;
                    mean4 = 0;
                    dt_mean = 0;

                    // Changement d' tat
                    state = STATE_2;

                }
                else
                {
                    // Reset des variables pour mesurer   nouveau
                    mean1 = 0;
                    mean2 = 0;
                    mean3 = 0;
                    mean4 = 0;
                    dt_mean = 0;
                }
            }

            break;

        // State 2 :
        // Rampe de courant "infinie" pour atteindre un courant
        // assez grand pour faire d coller l'avant
        case STATE_2:

            if(Position1 <= Position_c1_dec && Position2 <= Position_c2_dec)
            {
                // Update des variables
                Position_c1 = Position1;
                Position_c2 = Position2;
                Position_c3 = Position3;
                Position_c4 = Position4;

                xr1 = 0.0f;
                xr2 = 0.0f;
                xr3 = 0.0f;
                xr4 = 0.0f;

                // Reset standard de l'état estimé
                pos_est1 = Position1; vit_est1 = 0.0f; for_est1 = 0.0f;
                pos_est2 = Position2; vit_est2 = 0.0f; for_est2 = 0.0f;
                pos_est3 = Position3; vit_est3 = 0.0f; for_est3 = 0.0f;
                pos_est4 = Position4; vit_est4 = 0.0f; for_est4 = 0.0f;

                // Changement d' tat
                state = STATE_3;
            }
            else if(ic1 < 7.82f)
            {
                ic1 += TAKEOFF_CURRENT_STEP1;
            }
            else
            {
                // Fixe le courant   une valeur max
                ic1 = 7.82f;
            }

            // Fixe le courant de ic2   ic1
            ic2 = ic1;
            ic3 = ic1;
            ic4 = ic1;

            break;

        // State 3 :
        // Activation de la r gulation de position pour atteindre les 2mm
        //   l'avant
        case STATE_3:

            // Activer la r gulation d' tat
            if(!PosRegFlag1) PosRegFlag1 = true;

            // Set point generator (Ramp from 3mm to 2mm)
            if(Position_c1 > DELTA_N)
            {
                Position_c1 -= 2.5 * 4e-7;
            }
            else
            {
                Position_c1 = DELTA_N;
            }

            if(Position_c2 > DELTA_N)
            {
                Position_c2 -= 2.5 * 4e-7;
            }
            else
            {
                Position_c2 = DELTA_N;
            }

            // Incr mentation du timer
            i_store++;

            // Set point generator (Ramp from 3mm to 2mm)
            if(Position_c3 > DELTA_N)
            {
                Position_c3 -= 2.5 * 4e-7;
            }
            else
            {
                Position_c3 = DELTA_N;
            }

            if(Position_c4 > DELTA_N)
            {
                Position_c4 -= 2.5 * 4e-7;
            }
            else
            {
                Position_c4 = DELTA_N;
            }

            // Timer pour attendre un peu pour  tre sur que le syst me   bien d coller
            if(i_store == I_STORE_2E_DECOLLAGE)
            {
                // Reset de la varibale
                i_store = 0;

                // V rifie si l'on veut uniquement lever l'avant
                state = STATE_4;
            }

            break;

        // State 8 :
        // Attends que le bouton soit pressÃ© Ã  nouveau pour Ã©teindre la maquette
        case STATE_4:

            if(!SystemOn)
            {
                GPIO_writePin(LED_D2, 1); // Eteindre LED

                // Forcer les PWM   50%
                for(i = 0; i < NUM_OF_PWM_CHANNEL; i++)
                {
                    dutyFine = ((float)(duty_cycle_table[i] * TIME_BASE_PERIOD) * INV_FACTOR);
                    compCount = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
                    HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
                    HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
                }

                // Reset de la machine d' tat
                state = STATE_1;
                PosRegFlag1 = false;
                PosRegFlag3 = false;

                // Reset des gains d' tat
                Kw = KW;
                Kd = KD;
                Kddot = KDDOT;
                Kr = KR;
                Kr_sans_int = KR_SANS_INT;

                // Reset timer
                i_store = 0;

                // 1. Reset des int grales du r gulateur de courant PI et de l'anti-windup
                integral_i1 = 0.0f; ue1 = 0.0f; ic1 = 0.0f;
                integral_i2 = 0.0f; ue2 = 0.0f; ic2 = 0.0f;
                integral_i3 = 0.0f; ue3 = 0.0f; ic3 = 0.0f;
                integral_i4 = 0.0f; ue4 = 0.0f; ic4 = 0.0f;

                // 2. Reset des int grateurs de la r gulation d' tat (position) et anti-windup de force
                xr1 = 0.0f; fce1 = 0.0f; ep1 = 0.0f;
                xr2 = 0.0f; fce2 = 0.0f; ep2 = 0.0f;
                xr3 = 0.0f; fce3 = 0.0f; ep3 = 0.0f;
                xr4 = 0.0f; fce4 = 0.0f; ep4 = 0.0f;

                // RESET DES OBSERVATEURS POUR LE PROCHAIN VOL
                pos_est1 = Position1; vit_est1 = 0.0f; for_est1 = 0.0f;
                pos_est2 = Position2; vit_est2 = 0.0f; for_est2 = 0.0f;
                pos_est3 = Position3; vit_est3 = 0.0f; for_est3 = 0.0f;
                pos_est4 = Position4; vit_est4 = 0.0f; for_est4 = 0.0f;

                status = SFO();
                if (status == SFO_ERROR) error();

                // Retour Ã  l'Ã©tat 0
                state = STATE_0;
            }

            break;

        // Default :
        // Something went wrong so let's just go to state 9 for an error
        default:
            break;
    }
    // ================================================================= //
    // STATE MACHINE END
    // ================================================================= //

    // Active la rÃ©gulation uniquement si l'Ã©tat 0 n'est pas active
    if(state != STATE_0)
    {

        // ================================================================= //
        // B. REGULATION DE POSITION (ESPACE D'ETAT)
        // ================================================================= //
        if(PosRegFlag1)
        {
            // --- Inducteur 1 (LQI 1 DDL) ---
            ep1 = Position_c1 - Position1;
            fc1_prim = FP + LQI1_Q*(Position1 - Position_c1) + LQI1_QD*vit_est1 - LQI1_EPS*xr1*I;

            // Saturation + anti-windup par gel de l'intégrateur
            fc1 = fc1_prim;
            bool sat1 = false;
            if (fc1_prim <= 0)      { fc1 = 0;     sat1 = true; }
            else if (fc1_prim >= FMAX1) { fc1 = FMAX1; sat1 = true; }
            if (!sat1) xr1 += ep1 * H;          // n'intègre que si pas saturé

            // Notch + transformée inverse : INCHANGÉS
//            IN1[0] = fc1;
//            fc1f = IIR_Filter(IN1, OUT1);
//            if (fc1f < 0) fc1f = 0; else if (fc1f > FMAX1) fc1f = FMAX1;
            ic1 = sqrtf(K_FC1 * fc1) * Position1;

            // --- Inducteur 2 (LQI 2 DDL) ---
            ep2 = Position_c2 - Position2;
            fc2_prim = FP + LQI2_Q*(Position2 - Position_c2) + LQI2_QD*vit_est2 - LQI2_EPS*xr2*I;

            // Saturation + anti-windup par gel de l'intégrateur
            fc2 = fc2_prim;
            bool sat2 = false;
            if (fc2_prim <= 0)      { fc2 = 0;     sat2 = true; }
            else if (fc2_prim >= FMAX2) { fc2 = FMAX2; sat2 = true; } // CORRIGÉ (fc1 -> fc2)
            if (!sat2) xr2 += ep2 * H;

            // Notch + transformée inverse : INCHANGÉS
//            IN2[0] = fc2;
//            fc2f = IIR_Filter(IN2, OUT2);
//            if (fc2f < 0) fc2f = 0; else if (fc2f > FMAX2) fc2f = FMAX2; // CORRIGÉ (FMAX1 -> FMAX2)
            ic2 = sqrtf(K_FC2 * fc2) * Position2;

            // --- Inducteur 3 (LQI 3 DDL) ---
            ep3 = Position_c3 - Position3;
            fc3_prim = FP + LQI3_Q*(Position3 - Position_c3) + LQI3_QD*vit_est3 - LQI3_EPS*xr3*I;

            // Saturation + anti-windup par gel de l'intégrateur
            fc3 = fc3_prim;
            bool sat3 = false;
            if (fc3_prim <= 0)      { fc3 = 0;     sat3 = true; }
            else if (fc3_prim >= FMAX3) { fc3 = FMAX3; sat3 = true; }
            if (!sat3) xr3 += ep3 * H;          // n'intègre que si pas saturé

            // Notch + transformée inverse : INCHANGÉS
//            IN3[0] = fc3;
//            fc3f = IIR_Filter(IN3, OUT3);
//            if (fc3f < 0) fc3f = 0; else if (fc3f > FMAX3) fc3f = FMAX3; // CORRIGÉ (fc1f -> fc3f)
            ic3 = sqrtf(K_FC3 * fc3) * Position3;

            // --- Inducteur 4 (LQI 4 DDL) ---
            ep4 = Position_c4 - Position4;
            fc4_prim = FP + LQI4_Q*(Position4 - Position_c4) + LQI4_QD*vit_est4 - LQI4_EPS*xr4*I;

            // Saturation + anti-windup par gel de l'intégrateur
            fc4 = fc4_prim;
            bool sat4 = false;
            if (fc4_prim <= 0)      { fc4 = 0;     sat4 = true; }
            else if (fc4_prim >= FMAX4) { fc4 = FMAX4; sat4 = true; }
            if (!sat4) xr4 += ep4 * H;          // n'intègre que si pas saturé

            // Notch + transformée inverse : INCHANGÉS
//            IN4[0] = fc4;
//            fc4f = IIR_Filter(IN4, OUT4);
//            if (fc4f < 0) fc4f = 0; else if (fc4f > FMAX4) fc4f = FMAX4;
            ic4 = sqrtf(K_FC4 * fc4) * Position4;
        }

        // ================================================================= //
        // C. REGULATION DE COURANT (PI) & RAPPORT CYCLIQUE
        // ================================================================= //
        PI_current_regulator(ic1, Current1, &integral_i1, &ue1, &uc1);
        PI_current_regulator(ic2, Current2, &integral_i2, &ue2, &uc2);
        PI_current_regulator(ic3, Current3, &integral_i3, &ue3, &uc3);
        PI_current_regulator(ic4, Current4, &integral_i4, &ue4, &uc4);

        dutyCycle1 = 0.5f + (CONV_DUTY_CYCLE * uc1) + DA;
        dutyCycle2 = 0.5f + (CONV_DUTY_CYCLE * uc2) + DA;
        dutyCycle3 = 0.5f + (CONV_DUTY_CYCLE * uc3) + DA;
        dutyCycle4 = 0.5f + (CONV_DUTY_CYCLE * uc4) + DA;

        /* --------------------------------------------------------------------- *
         * 5. MISE A JOUR DES PWM & CALIBRATION SFO
         * --------------------------------------------------------------------- */

        // Conversion en pourcentage
        duty_table[0] = dutyCycle1 * 100;
        duty_table[1] = dutyCycle2 * 100;
        duty_table[2] = dutyCycle3 * 100;
        duty_table[3] = dutyCycle4 * 100;

        for (i = 0; i < NUM_OF_PWM_CHANNEL; i++)
        {
            // V rification des limites MIN et MAX avec le bon index [i]
            if (duty_table[i] >= LIMITE_MAX_DUTY_FINE) {
                duty_table[i] = LIMITE_MAX_DUTY_FINE;
            }
            else if (duty_table[i] <= LIMITE_MIN_DUTY_FINE) {
                duty_table[i] = LIMITE_MIN_DUTY_FINE;
            }

            // Mise   jour des registres HRPWM
            dutyFine = ((float)(duty_table[i] * TIME_BASE_PERIOD) * INV_FACTOR);
            compCount = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
        }

        // SFO Calibration
        status = SFO();
        if (status == SFO_ERROR) error();
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

    /* --------------------------------------------------------------------- *
     * 7. DEBOUNCE BOUTON POUSSOIR EXTERNE
     * --------------------------------------------------------------------- */
    if(Ext_Int_Flag) // D tection flanc montant
    {

        count_ext_int++;

        if(count_ext_int >= COUNT_TO_REACH)
        {
            if(GPIO_readPin(Push_Button_Start) == 0)
            {
                // Toggle button state
                SystemOn = !SystemOn;
            }

            Ext_Int_Flag = false;
            count_ext_int = 0;
        }

    }

    // Eteindre LED de debug (fin de boucle de r gulation)
    //GPIO_writePin(LED_D5, 0);
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
            dataIndex = rxIndex; // D but de trame
        }

        if(c == '\x03'){ // Fin de trame
            if(dataIndex > 0 && dataIndex < rxIndex - 1) {
                // Utilisation d'une  valuation bool enne directe !
                SystemOn = (rxBuffer[dataIndex] == '1');
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
