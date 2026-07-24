//###########################################################################
//
// FILE:   Sustentation_Main.c
//
// TITLE:  Power command and regulation for magnetic sustenance
//
// AUTHOR :
//          - Thomas Freyche - 2025
//          - Can UluÃ§inar   - 2026
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
#define STATE_7                     7

/* ========================================================================= *
 * GLOBAL VARIABLES
 * ========================================================================= */
// --- Sélection Inducteur (0 = Ind1, 1 = Ind2, 2 = Ind3, 3 = Ind4) ---
uint8_t ACTIVE_IND = 0;

// --- SFO Library ---
int MEP_ScaleFactor;

// --- ADC & Offset ---
const ADC_SOCNumber SOC_ARRAY[NUM_OF_PWM_CHANNEL] = {ADC_SOC_NUMBER0, ADC_SOC_NUMBER1, ADC_SOC_NUMBER2, ADC_SOC_NUMBER3};
bool Offset_stop = false;
float Offset_count = 0;
float Offset_ADC[NUM_OF_PWM_CHANNEL] = {0, 0, 0, 0};
uint16_t ADC_pos[NUM_OF_PWM_CHANNEL] = {0, 0, 0, 0};
uint16_t ADC_cur[NUM_OF_PWM_CHANNEL] = {0, 0, 0, 0};

// --- System States ---
float Position[NUM_OF_PWM_CHANNEL] = {DELTA_0, DELTA_0, DELTA_0, DELTA_0};
float Current[NUM_OF_PWM_CHANNEL] = {0, 0, 0, 0};
float dutyCycle[NUM_OF_PWM_CHANNEL] = {0.5f, 0.5f, 0.5f, 0.5f};
float uc[NUM_OF_PWM_CHANNEL] = {0, 0, 0, 0};
float mean[NUM_OF_PWM_CHANNEL] = {0, 0, 0, 0};
unsigned int dt_mean = 0;

// --- Shared Control Variables ---
float I = 1;
float Kw = KW, Kd = KD, Kddot = KDDOT, Kr = KR, Kr_sans_int = KR_SANS_INT;
uint32_t i_store = 0;

// --- Arrays for Inductors Control ---
float ic[NUM_OF_PWM_CHANNEL] = {0};
float ue[NUM_OF_PWM_CHANNEL] = {0};
float integral_i[NUM_OF_PWM_CHANNEL] = {0};
float fc[NUM_OF_PWM_CHANNEL] = {0};
float posBuff[NUM_OF_PWM_CHANNEL][FILTWINDOW] = {0};
float v[NUM_OF_PWM_CHANNEL] = {0};
float ep[NUM_OF_PWM_CHANNEL] = {0};
float xr[NUM_OF_PWM_CHANNEL] = {0};
float fce[NUM_OF_PWM_CHANNEL] = {0};
float sum_vp[NUM_OF_PWM_CHANNEL] = {0};
float fc_prim[NUM_OF_PWM_CHANNEL] = {0};
float Position_c[NUM_OF_PWM_CHANNEL] = {DELTA_0, DELTA_0, DELTA_0, DELTA_0};
float Position_c_dec[NUM_OF_PWM_CHANNEL] = {0};

// --- Filter Buffers ---
float fc_f[NUM_OF_PWM_CHANNEL] = {0};
float IN[NUM_OF_PWM_CHANNEL][Nb] = {0};
float OUT[NUM_OF_PWM_CHANNEL][Na] = {0};

// --- PWM Management ---
float dutyFine = MIN_HRPWM_DUTY_PERCENT;
float duty_table[NUM_OF_PWM_CHANNEL] = {50, 50, 50, 50};
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
bool PosRegFlag = false;

// --- Variable for value sending ---
float Pos_filt[NUM_OF_PWM_CHANNEL] = {DELTA_0, DELTA_0, DELTA_0, DELTA_0};
float Cur_filt[NUM_OF_PWM_CHANNEL] = {0, 0, 0, 0};

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
    init();
    while(1);
}

void init(void)
{
    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();

    while(status == SFO_INCOMPLETE)
    {
        status = SFO();
        if(status == SFO_ERROR) error();
    }

    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    Board_init();
    SCI_enableInterrupt(mySCI0_BASE, SCI_INT_RXFF);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    EINT;
    ERTM;

    // Affection des PWMs (Toutes initialisées à 50%)
    for(i = 0; i < NUM_OF_PWM_CHANNEL; i++)
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
    GPIO_writePin(LED_D5, 1);

    /* --------------------------------------------------------------------- *
     * 1. LECTURE ADC & CALIBRATION (Canal Actif Uniquement)
     * --------------------------------------------------------------------- */
    ADC_pos[ACTIVE_IND] = ADC_readResult(ADCARESULT_BASE, SOC_ARRAY[ACTIVE_IND]);
    ADC_cur[ACTIVE_IND] = ADC_readResult(ADCBRESULT_BASE, SOC_ARRAY[ACTIVE_IND]);

    if(Offset_count <= 999 && !Offset_stop)
    {
        Offset_ADC[ACTIVE_IND] += ((float)ADC_cur[ACTIVE_IND]) - ADC_ZERO_CURRENT;
        Offset_count++;

        if(Offset_count > 999)
        {
            Offset_ADC[ACTIVE_IND] = Offset_ADC[ACTIVE_IND] * OFFSET_COUNT_INV;
            Offset_stop = true;
            Position_c_dec[ACTIVE_IND] = Position[ACTIVE_IND] * POS_DETECT;
        }
    }

    /* --------------------------------------------------------------------- *
     * 2. CONVERSIONS PHYSIQUES (Canal Actif Uniquement)
     * --------------------------------------------------------------------- */
    Position[ACTIVE_IND] = ((float)(ADC_pos[ACTIVE_IND]) * CONV_POS2);
    Pos_filt[ACTIVE_IND] = (ALPHA * Position[ACTIVE_IND]) + (ALPHA_INV * Pos_filt[ACTIVE_IND]);

    Current[ACTIVE_IND] = (((float)(ADC_cur[ACTIVE_IND]) - ADC_ZERO_CURRENT - Offset_ADC[ACTIVE_IND]) / (ADC_ZERO_CURRENT + Offset_ADC[ACTIVE_IND])) * I_MAX;
    Cur_filt[ACTIVE_IND] = (0.0001f * Current[ACTIVE_IND]) + (0.9999f * Cur_filt[ACTIVE_IND]);

    /* --------------------------------------------------------------------- *
     * 3. COMMUNICATION (TELEMETRIE)
     * --------------------------------------------------------------------- */
    UartCounter++;
    if (UartCounter >= 25000)
    {
        UartCounter = 0;
        SendFloatAsText(Pos_filt[0]*1000.0f, Pos_filt[1]*1000.0f, Pos_filt[2]*1000.0f, Pos_filt[3]*1000.0f);
    }

    /* --------------------------------------------------------------------- *
     * 4. MACHINE D'ETAT & REGULATION
     * --------------------------------------------------------------------- */
    if(ButtonS2 || state_PIN)
    {
        GPIO_writePin(LED_D2, 0);

        // --- Filtres de vitesse Savitzky-Golay ---
        posBuff[ACTIVE_IND][FILTWINDOW-1] = Position[ACTIVE_IND];
        v[ACTIVE_IND] = savitzky_Filter(posBuff[ACTIVE_IND]);

        // ================================================================= //
        // A. STATE MACHINE BEGINING
        // ================================================================= //
        switch(state)
        {
            case STATE_1:
                if(ic[ACTIVE_IND] < I_SP) ic[ACTIVE_IND] += TAKEOFF_CURRENT_STEP1;
                else ic[ACTIVE_IND] = I_SP;

                mean[ACTIVE_IND] += Current[ACTIVE_IND];
                dt_mean++;

                if(dt_mean >= 200)
                {
                    mean[ACTIVE_IND] = mean[ACTIVE_IND] / dt_mean;

                    if((mean[ACTIVE_IND] <= I_SP105 && mean[ACTIVE_IND] >= I_SP095))
                    {
                        mean[ACTIVE_IND] = 0;
                        dt_mean = 0;
                        state = STATE_2;
                    }
                    else
                    {
                        mean[ACTIVE_IND] = 0;
                        dt_mean = 0;
                    }
                }
                break;

            case STATE_2:
                if(Position[ACTIVE_IND] <= Position_c_dec[ACTIVE_IND])
                {
                    Position_c[ACTIVE_IND] = Position[ACTIVE_IND];
                    state = STATE_3;
                }
                else if(ic[ACTIVE_IND] < 7.82f)
                {
                    ic[ACTIVE_IND] += TAKEOFF_CURRENT_STEP1;
                }
                else
                {
                    ic[ACTIVE_IND] = 7.82f;
                }
                break;

            case STATE_3:
                if(!PosRegFlag) PosRegFlag = true;

                if(Position_c[ACTIVE_IND] > DELTA_N) Position_c[ACTIVE_IND] -= 2.5 * 4e-7;
                else Position_c[ACTIVE_IND] = DELTA_N;

                i_store++;

                if(i_store == I_STORE_2E_DECOLLAGE)
                {
                    i_store = 0;
                    state = STATE_7;
                }
                break;

            case STATE_7:
                Kr = KR_CHANGE;
                Kw = KW_CHANGE;
                Kd = KD_CHANGE;
                Kddot = KDDOT_CHANGE;
                break;

            default:
                break;
        }

        // ================================================================= //
        // B. REGULATION DE POSITION (ESPACE D'ETAT)
        // ================================================================= //
        if(PosRegFlag)
        {
            IN[ACTIVE_IND][0] = fc[ACTIVE_IND];
            fc_f[ACTIVE_IND] = IIR_Filter(IN[ACTIVE_IND], OUT[ACTIVE_IND]);
            if (fc_f[ACTIVE_IND] <= 0)   fc_f[ACTIVE_IND] = 0;
            if (fc_f[ACTIVE_IND] >= FMAX) fc_f[ACTIVE_IND] = FMAX;

            ep[ACTIVE_IND] = Position_c[ACTIVE_IND] - Position[ACTIVE_IND];
            xr[ACTIVE_IND] += (ep[ACTIVE_IND] - fce[ACTIVE_IND]);
            sum_vp[ACTIVE_IND] = v[ACTIVE_IND] * Kddot + Position[ACTIVE_IND] * Kd;
            fc_prim[ACTIVE_IND] = Kw * Position_c[ACTIVE_IND] + Kr * xr[ACTIVE_IND] * I - sum_vp[ACTIVE_IND] + FP;

            fc[ACTIVE_IND] = fc_prim[ACTIVE_IND];
            if (fc_prim[ACTIVE_IND] <= 0)   fc[ACTIVE_IND] = 0;
            if (fc_prim[ACTIVE_IND] >= FMAX) fc[ACTIVE_IND] = FMAX;
            fce[ACTIVE_IND] = (fc_prim[ACTIVE_IND] - fc[ACTIVE_IND]) * K_ANTIWINDUP * ANTIWINDUP_EN;

            ic[ACTIVE_IND] = (sqrtf(K_FC * fc_f[ACTIVE_IND])) * Position[ACTIVE_IND];
        }

        // ================================================================= //
        // C. REGULATION DE COURANT (PI) & RAPPORT CYCLIQUE
        // ================================================================= //
        PI_current_regulator(ic[ACTIVE_IND], Current[ACTIVE_IND], &integral_i[ACTIVE_IND], &ue[ACTIVE_IND], &uc[ACTIVE_IND]);

        dutyCycle[ACTIVE_IND] = 0.5f + (CONV_DUTY_CYCLE * uc[ACTIVE_IND]) + DA;

        /* --------------------------------------------------------------------- *
         * 5. MISE A JOUR DES PWM & CALIBRATION SFO
         * --------------------------------------------------------------------- */
        duty_table[ACTIVE_IND] = dutyCycle[ACTIVE_IND] * 100;

        if (duty_table[ACTIVE_IND] >= LIMITE_MAX_DUTY_FINE) {
            duty_table[ACTIVE_IND] = LIMITE_MAX_DUTY_FINE;
        }
        else if (duty_table[ACTIVE_IND] <= LIMITE_MIN_DUTY_FINE) {
            duty_table[ACTIVE_IND] = LIMITE_MIN_DUTY_FINE;
        }

        // Mise à jour de la PWM active uniquement (les autres restent à 50% de l'init)
        dutyFine = ((float)(duty_table[ACTIVE_IND] * TIME_BASE_PERIOD) * INV_FACTOR);
        compCount = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
        HRPWM_setCounterCompareValue(ePWM[ACTIVE_IND], HRPWM_COUNTER_COMPARE_A, compCount);
        HRPWM_setCounterCompareValue(ePWM[ACTIVE_IND], HRPWM_COUNTER_COMPARE_B, compCount);

        status = SFO();
        if (status == SFO_ERROR) error();
    }
    else if(!ButtonS2 || !state_PIN) // --- ArrÃªt Sustentation ---
    {
        GPIO_writePin(LED_D2, 1);

        // Forçage de toutes les PWMs à 50%
        for(i = 0; i < NUM_OF_PWM_CHANNEL; i++)
        {
            dutyFine = ((float)(duty_cycle_table[i] * TIME_BASE_PERIOD) * INV_FACTOR);
            compCount = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
        }

        state = STATE_1;
        PosRegFlag = false;

        Kw = KW;
        Kd = KD;
        Kddot = KDDOT;
        Kr = KR;
        Kr_sans_int = KR_SANS_INT;

        i_store = 0;

        // Optionnel : Forcer le reset du flag d'offset si besoin de recalibrer au changement d'inducteur
        // Offset_stop = false;
        // Offset_count = 0;

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
    if(Ext_Int_Flag)
    {
        count_ext_int = 0;
        while(count_ext_int < COUNT_TO_REACH)
            count_ext_int++;

        if(GPIO_readPin(Push_Button_Start) == 0)
        {
            ButtonS2 = !ButtonS2;
        }
        Ext_Int_Flag = false;
    }

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
            dataIndex = rxIndex;
        }

        if(c == '\x03'){
            if(dataIndex > 0 && dataIndex < rxIndex - 1) {
                state_PIN = (rxBuffer[dataIndex] == '1');
            }
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

__interrupt void INT_mySCI0_TX_ISR(void)
{
    while (SCI_getTxFIFOStatus(SCIA_BASE) < SCI_FIFO_TX16 && txIndex < txLength)
    {
        SCI_writeCharBlockingFIFO(SCIA_BASE, txBuffer[txIndex++]);
    }

    if (txIndex >= txLength)
    {
        SCI_disableInterrupt(mySCI0_BASE, SCI_INT_TXFF);
    }

    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
