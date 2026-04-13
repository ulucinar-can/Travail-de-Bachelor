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

#define POS_CORRECTION_1            1.143f
#define POS_CORRECTION_2            1.131f
#define POS_CORRECTION_3            1.187f
#define POS_CORRECTION_4            1.159f
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
    // --- Conversion 12 bits -> Position (Ecart intégré) ---
    Position1 = (float)(ADC_pos_1) * CONV_POS2 * POS_CORRECTION_1;
    Position2 = (float)(ADC_pos_2) * CONV_POS2 * POS_CORRECTION_2;
    Position3 = (float)(ADC_pos_3) * CONV_POS2 * POS_CORRECTION_3;
    Position4 = (float)(ADC_pos_4) * CONV_POS2 * POS_CORRECTION_4;

    // --- Conversion 12 bits -> Courant (TFE 2025) ---
    Current1  = (((float)(ADC_cur_1) - ADC_ZERO_CURRENT - Offset_ADC1) / (ADC_ZERO_CURRENT + Offset_ADC1)) * I_MAX;
    Current2  = (((float)(ADC_cur_2) - ADC_ZERO_CURRENT - Offset_ADC2) / (ADC_ZERO_CURRENT + Offset_ADC2)) * I_MAX;
    Current3  = (((float)(ADC_cur_3) - ADC_ZERO_CURRENT - Offset_ADC3) / (ADC_ZERO_CURRENT + Offset_ADC3)) * I_MAX;
    Current4  = (((float)(ADC_cur_4) - ADC_ZERO_CURRENT - Offset_ADC4) / (ADC_ZERO_CURRENT + Offset_ADC4)) * I_MAX;

    /* --------------------------------------------------------------------- *
     * 3. COMMUNICATION (TELEMETRIE)
     * --------------------------------------------------------------------- */
    UartCounter++;
    if (UartCounter >= 25000)
    {
        UartCounter = 0; // Reset du compteur (~1s)
        SendFloatAsText(Position1*1000.0f, Position2*1000.0f, Position3*1000.0f, Position4*1000.0f);
    }

    /* --------------------------------------------------------------------- *
     * 4. MACHINE D'ETAT & REGULATION (Si activée)
     * --------------------------------------------------------------------- */

    // State 0 :
    // Vérification de l'enclenchement de la régulation
    if(ButtonS2 || state_PIN)
    {
        GPIO_writePin(LED_D2, 0); // Allumage LED2 (Indicateur sustentation)

        // Vérife pour passer le devant ou non
        if(SKIP_FRONT && state == STATE_1) state = STATE_4;

        // --- Filtres de vitesse Savitzky-Golay ---
        pos1Buff[FILTWINDOW-1] = Position1;
        v1 = savitzky_Filter(pos1Buff);

        pos2Buff[FILTWINDOW-1] = Position2;
        v2 = savitzky_Filter(pos2Buff);

        pos3Buff[FILTWINDOW-1] = Position3;
        v3 = savitzky_Filter(pos3Buff);

        pos4Buff[FILTWINDOW-1] = Position4;
        v4 = savitzky_Filter(pos4Buff);

        // ================================================================= //
        // A. STATE MACHINE BEGINING
        // ================================================================= //
        switch(state)
        {
            // State 1 :
            // Rampe de courant sur les inducteur 1&2 jusqu'à atteindre les 3A
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

                // Fixe le courant de consigne 2 par rapport à ic1
                ic2 = ic1;

                mean1 += Current1;
                mean2 += Current2;
                dt_mean++;

                if(dt_mean >= 200) // dt = 8ms
                {
                    mean1 = mean1 / dt_mean;
                    mean2 = mean2 / dt_mean;

                    if((mean1 <= I_SP105 && mean1 >= I_SP095) && (mean2 <= I_SP105 && mean2 >= I_SP095))
                    {
                        // Reset des variables
                        mean1 = 0;
                        mean2 = 0;
                        dt_mean = 0;

                        // Changement d'état
                        state = STATE_2;

                    }
                    else
                    {
                        // Reset des variables pour mesurer à nouveau
                        mean1 = 0;
                        mean2 = 0;
                        dt_mean = 0;
                    }
                }

                break;

            // State 2 :
            // Rampe de courant "infinie" pour atteindre un courant
            // assez grand pour faire décoller l'avant
            case STATE_2:

                if(Position1 <= Position_c1_dec && Position2 <= Position_c2_dec)
                {
                    // Update des variables
                    Position_c1 = Position1;
                    Position_c2 = Position2;

                    // Changement d'état
                    state = STATE_3;
                }
                else if(ic1 < 7.82f)
                {
                    ic1 += TAKEOFF_CURRENT_STEP1;
                }
                else
                {
                    // Fixe le courant à une valeur max
                    ic1 = 7.82f;
                }

                // Fixe le courant de ic2 à ic1
                ic2 = ic1;

                break;

            // State 3 :
            // Activation de la régulation de position pour atteindre les 2mm
            // à l'avant
            case STATE_3:

                // Activer la régulation d'état
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

                // Incrémentation du timer
                i_store++;

                // Timer pour attendre un peu pour être sur que le système à bien décoller
                if(i_store == I_STORE_2E_DECOLLAGE)
                {
                    // Reset de la varibale
                    i_store = 0;

                    if(SKIP_BACK)
                    {
                        state = STATE_7;
                    }
                    else
                    {
                        state = STATE_4;
                    }
                }

                break;

            // State 4 :
            // Rampe de courant sur les inducteur 3&4 jusqu'à atteindre les 3A
            case STATE_4:

                // Rampe vers 3A par pas de 0.04A
                if(ic3 < I_SP)
                {
                    ic3 += TAKEOFF_CURRENT_STEP1;
                }
                else
                {
                    ic3 = I_SP;
                }

                // Fixe le courant de consigne 4 par rapport à ic3
                ic4 = ic3;

                mean3 += Current3;
                mean4 += Current4;
                dt_mean++;

                if(dt_mean >= 200) // dt = 8ms
                {
                    mean3 = mean3 / dt_mean;
                    mean4 = mean4 / dt_mean;

                    if((mean3 <= I_SP105 && mean3 >= I_SP095) && (mean4 <= I_SP105 && mean4 >= I_SP095))
                    {
                        // Reset des variables
                        mean3 = 0;
                        mean4 = 0;
                        dt_mean = 0;

                        state = STATE_5;
                    }
                    else
                    {
                        // Reset des variables pour mesurer à nouveau
                        mean3 = 0;
                        mean4 = 0;
                        dt_mean = 0;
                    }
                }

                break;

            // State 5 :
            // Rampe de courant "infinie" pour atteindre un courant
            // assez grand pour faire décoller l'arrière
            case STATE_5:

                if(Position3 <= Position_c3_dec && Position4 <= Position_c4_dec)
                {
                    // Update des variables
                    Position2_c3 = Position3;
                    Position2_c4 = Position4;

                    // Changement d'état
                    state = STATE_6;
                }
                else if(ic3 < 7.82f)
                {
                    ic3 += TAKEOFF_CURRENT_STEP1;
                }
                else
                {
                    // Fixe le courant à une valeur max
                    ic3 = 7.82f;
                }

                // Fixe le courant de ic4 à ic3
                ic4 = ic3;


                break;

            // State 6 :
            // Activation de la régulation de position pour atteindre les 2mm
            // à l'arrière
            case STATE_6:

                // Activer la régulation d'état
                if(!PosRegFlag3) PosRegFlag3 = true;

                // Set point generator (Ramp from 3mm to 2mm)
                if(Position2_c3 > DELTA_N)
                {
                    Position2_c3 -= 2.5 * 4e-7;
                }
                else
                {
                    Position2_c3 = DELTA_N;
                }

                if(Position2_c4 > DELTA_N)
                {
                    Position2_c4 -= 2.5 * 4e-7;
                }
                else
                {
                    Position2_c4 = DELTA_N;
                }

                // Incrémentation du timer
                i_store++;

                // Timer pour attendre un peu pour être sur que le système à bien décoller
                if(i_store == I_STORE_2E_DECOLLAGE)
                {
                    // Reset de la varibale
                    i_store = 0;

                    // Changement du gain statique de l'inducteur 3
                    Kr_sans_int = -1.0;

                    // Changement d'état
                    state = STATE_7;
                }

                break;

            // State 7 :
            // Régulation de maintien pour tenir la maquette en l'air
            case STATE_7:

                // Changement des variables pour une régulation plus douce
                Kr = KR_CHANGE;
                Kw = KW_CHANGE;
                Kd = KD_CHANGE;
                Kddot = KDDOT_CHANGE;

                break;

            // Default :
            // Something went wrong so let's just go to state 9 for an error
            default:
                break;

        }
        // ================================================================= //
        // STATE MACHINE END
        // ================================================================= //

        // ================================================================= //
        // B. REGULATION DE POSITION (ESPACE D'ETAT)
        // ================================================================= //

        if(PosRegFlag1)
        {
            // Inductor 1 : Filtre Bandstop + State Regulation
            IN1[0] = fc1;
            fc1f = IIR_Filter(IN1, OUT1);
            if (fc1f <= 0)   fc1f = 0;
            if (fc1f >= FMAX) fc1f = FMAX;

            ep1 = Position_c1 - Position1;
            xr1 += (ep1 - fce1);
            sum_vp1 = v1 * Kddot + Position1 * Kd;
            fc1_prim = Kw * Position_c1 + Kr * xr1 * I - sum_vp1 + FP;

            fc1 = fc1_prim;
            if (fc1_prim <= 0)   fc1 = 0;
            if (fc1_prim >= FMAX) fc1 = FMAX;
            fce1 = (fc1_prim - fc1) * K_ANTIWINDUP * ANTIWINDUP_EN;

            // Inductor 2 : Filtre Bandstop + State Regulation
            IN2[0] = fc2;
            fc2f = IIR_Filter(IN2, OUT2);
            if (fc2f <= 0)   fc2f = 0;
            if (fc2f >= FMAX) fc2f = FMAX;

            ep2 = Position_c2 - Position2;
            xr2 += (ep2 - fce2);
            sum_vp2 = v2 * Kddot + Position2 * Kd;
            fc2_prim = Kw * Position_c2 + Kr * xr2 * I - sum_vp2 + FP;

            fc2 = fc2_prim;
            if (fc2_prim <= 0)   fc2 = 0;
            if (fc2_prim >= FMAX) fc2 = FMAX;
            fce2 = (fc2_prim - fc2) * K_ANTIWINDUP * ANTIWINDUP_EN;

            // Transformée inverse pour calculer le courant à partir de la force
            ic1 = (sqrtf(K_FC * fc1f)) * Position1;
            ic2 = (sqrtf(K_FC * fc2f)) * Position2;
        }

        if(PosRegFlag3)
        {
            // Inductor 3 : Filtre Bandstop + State Regulation
            IN3[0] = fc3;
            fc3f = IIR_Filter(IN3, OUT3);
            if (fc3f <= 0)   fc3f = 0;
            if (fc3f >= FMAX) fc3f = FMAX;

            ep3 = Position2_c3 - Position3;
            xr3 += (ep3 - fce3);
            sum_vp3 = (v3 * KDDOT_SANS_INT) + (Position3 * KD_SANS_INT);
            fc3_prim = (KW_SANS_INT * Position2_c3) + (Kr_sans_int * xr3 * I) - sum_vp3 + FP;

            fc3 = fc3_prim;
            if (fc3_prim <= 0)   fc3 = 0;
            if (fc3_prim >= FMAX) fc3 = FMAX;
            fce3 = (fc3_prim - fc3) * K_ANTIWINDUP * ANTIWINDUP_EN;

            // Inductor 4 : Filtre Bandstop + State Regulation
            IN4[0] = fc4;
            fc4f = IIR_Filter(IN4, OUT4);
            if (fc4f <= 0)   fc4f = 0;
            if (fc4f >= FMAX) fc4f = FMAX;

            ep4 = Position2_c4 - Position4;
            xr4 += (ep4 - fce4);
            sum_vp4 = (v4 * Kddot) + (Position4 * Kd);
            fc4_prim = (Kw * Position2_c4) + (Kr * xr4 * I) - sum_vp4 + FP;

            fc4 = fc4_prim;
            if (fc4_prim <= 0)   fc4 = 0;
            if (fc4_prim >= FMAX) fc4 = FMAX;
            fce4 = (fc4_prim - fc4) * K_ANTIWINDUP * ANTIWINDUP_EN;

            // Transformée inverse pour calculer le courant à partir de la force
            ic3 = sqrtf(K_FC * fc3f) * Position3;
            ic4 = sqrtf(K_FC * fc4f) * Position4;
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
            // Vérification des limites MIN et MAX avec le bon index [i]
            if (duty_table[i] >= LIMITE_MAX_DUTY_FINE) {
                duty_table[i] = LIMITE_MAX_DUTY_FINE;
            }
            else if (duty_table[i] <= LIMITE_MIN_DUTY_FINE) {
                duty_table[i] = LIMITE_MIN_DUTY_FINE;
            }

            // Mise à jour des registres HRPWM
            dutyFine = ((float)(duty_table[i] * TIME_BASE_PERIOD) * INV_FACTOR);
            compCount = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
        }

        // SFO Calibration
        status = SFO();
        if (status == SFO_ERROR) error();
    }
    // State 8 :
    // Arrêt de la sustentation
    else if(!ButtonS2 || !state_PIN) // --- Arrêt Sustentation ---
    {
        GPIO_writePin(LED_D2, 1); // Eteindre LED

        // Forcer les PWM à 50%
        for(i = 0; i < NUM_OF_PWM_CHANNEL; i++)
        {
            dutyFine = ((float)(duty_cycle_table[i] * TIME_BASE_PERIOD) * INV_FACTOR);
            compCount = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
        }

        // Reset de la machine d'état
        state = STATE_1;
        PosRegFlag1 = false;
        PosRegFlag3 = false;

        Kw = KW;
        Kd = KD;
        Kddot = KDDOT;
        Kr = KR;
        Kr_sans_int = KR_SANS_INT;

        i_store = 0;

        status = SFO();
        if (status == SFO_ERROR) error();

        // State 9 :
        // Arrêt de la sustentation avec erreur
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
    if(Ext_Int_Flag) // Détection flanc montant
    {
        count_ext_int = 0;
        // ATTENTION : Ce 'while' crée un délai bloquant dans l'ISR !
        while(count_ext_int < COUNT_TO_REACH)
            count_ext_int++;

        if(GPIO_readPin(Push_Button_Start) == 0)
        {
            // Toggle button state
            ButtonS2 = !ButtonS2;
        }
        Ext_Int_Flag = false;
    }

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
