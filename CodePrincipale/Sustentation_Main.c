//###########################################################################
//
// FILE:    Sustentation_Main.c
//
// TITLE:   Power command and regulation for magnetic sustenance (MIMO)
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

#define TAKEOFF_CURRENT_STEP1       0.04f

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
float I = 1; // Conserv e en variable pour pouvoir couper l'int grateur en direct via debugger
uint32_t i_store = 0;

// --- Inductor 1 Control (SISO avant, actif jusqu'a la bascule MIMO) ---
float ic1 = 0.0, ue1 = 0.0, integral_i1 = 0;
float fc1 = 0;
float ep1 = 0, xr1 = 0, fc1_prim = 0;
float Position_c1 = DELTA_0, Position_c1_dec = 2.85e-3f;

// --- Inductor 2 Control (SISO avant, actif jusqu'a la bascule MIMO) ---
float ic2 = 0, ue2 = 0, integral_i2 = 0;
float fc2 = 0;
float ep2 = 0, xr2 = 0, fc2_prim = 0;
float Position_c2 = DELTA_0, Position_c2_dec = 2.85e-3f;

// --- Inductor 3 ---
float ic3 = 0, ue3 = 0, integral_i3 = 0;
float fc3 = 0;
float Position_c3_dec = 2.85e-3f;

// --- Inductor 4 ---
float ic4 = 0, ue4 = 0, integral_i4 = 0;
float fc4 = 0;
float Position_c4_dec = 2.85e-3f;

// --- MIMO : torseur d'etat, references, observateurs modaux, integrateurs ---
bool MimoFlag = false;
float frameCnt = 0;
float qm[3]     = {DELTA_0, 0, 0};
float qm_ref[3] = {DELTA_N, 0, 0};
float qm_est[3] = {DELTA_0, 0, 0};
float vm_est[3] = {0, 0, 0};
float um_est[3] = {0, 0, 0};
float eps_m[3]  = {0, 0, 0};
float u_cmd[3]  = {0, 0, 0};
float u_sat[3]  = {0, 0, 0};
float u_ach[3]  = {0, 0, 0};
static const float qm_target[3] = {DELTA_N, 0.0f, 0.0f};

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

// --- Variable for filtrered value sending ---
float Pos1_filt = DELTA_0;
float Pos2_filt = DELTA_0;
float Pos3_filt = DELTA_0;
float Pos4_filt = DELTA_0;

float Cur1_filt = 0;
float Cur2_filt = 0;
float Cur3_filt = 0;
float Cur4_filt = 0;

// --- Variables Observateurs SISO avant (inducteurs 1 & 2) ---
float pos_est1 = DELTA_0, vit_est1 = 0.0f, for_est1 = 0.0f, err_obs1 = 0.0f;
float pos_est2 = DELTA_0, vit_est2 = 0.0f, for_est2 = 0.0f, err_obs2 = 0.0f;

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
#pragma CODE_SECTION(adcA1ISR, ".TI.ramfunc")
__interrupt void adcA1ISR(void)
{
    /* --------------------------------------------------------------------- *
     * 1. LECTURE ADC & CALIBRATION
     * --------------------------------------------------------------------- */
    ADC_pos_1 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
    ADC_pos_2 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1);
    ADC_pos_3 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER2);
    ADC_pos_4 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER3);

    ADC_cur_1 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER0);
    ADC_cur_2 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER1);
    ADC_cur_3 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER2);
    ADC_cur_4 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER3);

    if(Offset_count <= 999 && !Offset_stop)
    {
        Offset_ADC1 += ((float)ADC_cur_1) - ADC_ZERO_CURRENT;
        Offset_ADC2 += ((float)ADC_cur_2) - ADC_ZERO_CURRENT;
        Offset_ADC3 += ((float)ADC_cur_3) - ADC_ZERO_CURRENT;
        Offset_ADC4 += ((float)ADC_cur_4) - ADC_ZERO_CURRENT;
        Offset_count++;

        if(Offset_count > 999)
        {
            Offset_ADC1 = Offset_ADC1 * OFFSET_COUNT_INV;
            Offset_ADC2 = Offset_ADC2 * OFFSET_COUNT_INV;
            Offset_ADC3 = Offset_ADC3 * OFFSET_COUNT_INV;
            Offset_ADC4 = Offset_ADC4 * OFFSET_COUNT_INV;

            Offset_stop = true;
        }
    }

    /* --------------------------------------------------------------------- *
     * 2. CONVERSIONS PHYSIQUES & TORSEUR D'ETAT
     * --------------------------------------------------------------------- */
    Position1 = ((float)(ADC_pos_1) * CONV_POS2);
    Position2 = ((float)(ADC_pos_2) * CONV_POS2);
    Position3 = ((float)(ADC_pos_3) * CONV_POS2);
    Position4 = ((float)(ADC_pos_4) * CONV_POS2);

//    Position1 = apply_poly5((float)(ADC_pos_1), POS_COR_1);
//    Position2 = apply_poly5((float)(ADC_pos_2), POS_COR_2);
//    Position3 = apply_poly5((float)(ADC_pos_3), POS_COR_3);
//    Position4 = apply_poly5((float)(ADC_pos_4), POS_COR_4);

    qm[0] = T_MAT[0][0]*Position1 + T_MAT[0][1]*Position2 + T_MAT[0][2]*Position3 + T_MAT[0][3]*Position4;
    qm[1] = T_MAT[1][0]*Position1 + T_MAT[1][1]*Position2 + T_MAT[1][2]*Position3 + T_MAT[1][3]*Position4;
    qm[2] = T_MAT[2][0]*Position1 + T_MAT[2][1]*Position2 + T_MAT[2][2]*Position3 + T_MAT[2][3]*Position4;

    /* --------------------------------------------------------------------- *
     * 3. OBSERVATEURS
     * --------------------------------------------------------------------- */
    if(PosRegFlag1 && !MimoFlag)
    {
        err_obs1 = Position1 - pos_est1;
        float pos_est1_new = AD11_1 * pos_est1 + AD12_1 * vit_est1                     + L1_1 * err_obs1;
        float vit_est1_new =                     AD22_1 * vit_est1 + AD23_1 * for_est1 + L2_1 * err_obs1;
        float for_est1_new =                                         AD33_1 * for_est1 + BD3_1 * (fc1f - FP) + L3_1 * err_obs1;
        pos_est1 = pos_est1_new; vit_est1 = vit_est1_new; for_est1 = for_est1_new;

        err_obs2 = Position2 - pos_est2;
        float pos_est2_new = AD11_2 * pos_est2 + AD12_2 * vit_est2                     + L1_2 * err_obs2;
        float vit_est2_new =                     AD22_2 * vit_est2 + AD23_2 * for_est2 + L2_2 * err_obs2;
        float for_est2_new =                                         AD33_2 * for_est2 + BD3_2 * (fc2f - FP) + L3_2 * err_obs2;
        pos_est2 = pos_est2_new; vit_est2 = vit_est2_new; for_est2 = for_est2_new;
    }

    if(MimoFlag)
    {
        uint16_t j;
        for(j = 0; j < 3; j++)
        {
            float e_obs = qm[j] - qm_est[j];
            float q_new = qm_est[j] + OBS_AD12[j] * vm_est[j] + OBS_L1[j] * e_obs;
            float v_new = vm_est[j] + OBS_AD23[j] * um_est[j] + OBS_L2[j] * e_obs;
            float u_new = OBS_AD33[j] * um_est[j] + OBS_BD3[j] * u_ach[j] + OBS_L3[j] * e_obs;
            qm_est[j] = q_new; vm_est[j] = v_new; um_est[j] = u_new;
        }
    }

    Pos1_filt = (ALPHA * Position1) + (ALPHA_INV * Pos1_filt);
    Pos2_filt = (ALPHA * Position2) + (ALPHA_INV * Pos2_filt);
    Pos3_filt = (ALPHA * Position3) + (ALPHA_INV * Pos3_filt);
    Pos4_filt = (ALPHA * Position4) + (ALPHA_INV * Pos4_filt);

    Current1  = (((float)(ADC_cur_1) - ADC_ZERO_CURRENT - Offset_ADC1) / (ADC_ZERO_CURRENT + Offset_ADC1)) * I_MAX;
    Current2  = (((float)(ADC_cur_2) - ADC_ZERO_CURRENT - Offset_ADC2) / (ADC_ZERO_CURRENT + Offset_ADC2)) * I_MAX;
    Current3  = (((float)(ADC_cur_3) - ADC_ZERO_CURRENT - Offset_ADC3) / (ADC_ZERO_CURRENT + Offset_ADC3)) * I_MAX;
    Current4  = (((float)(ADC_cur_4) - ADC_ZERO_CURRENT - Offset_ADC4) / (ADC_ZERO_CURRENT + Offset_ADC4)) * I_MAX;

    Cur1_filt = (0.0001f * Current1) + (0.9999f * Cur1_filt);
    Cur2_filt = (0.0001f * Current2) + (0.9999f * Cur2_filt);
    Cur3_filt = (0.0001f * Current3) + (0.9999f * Cur3_filt);
    Cur4_filt = (0.0001f * Current4) + (0.9999f * Cur4_filt);

    /* --------------------------------------------------------------------- *
     * 4. COMMUNICATION (TELEMETRIE)
     * --------------------------------------------------------------------- */
    UartCounter++;
    if (UartCounter >= 250)
    {
        UartCounter = 0;

        SendFloatAsText(Pos1_filt*1000, Pos2_filt*1000, Pos3_filt*1000, Pos4_filt*1000, Cur1_filt, Cur2_filt, Cur3_filt, Cur4_filt);
    }

    /* --------------------------------------------------------------------- *
     * 5. MACHINE D'ETAT
     * --------------------------------------------------------------------- */

    if (!SystemOn && (state != STATE_0) && (state != STATE_6))
    {
        state = STATE_6;
    }

    switch(state)
       {
           // State 0 :
           // Attente de la pression du bouton
           case STATE_0 :

               if(SystemOn)
               {
                   GPIO_writePin(LED_D2, 0);

                   state = SKIP_FRONT ? STATE_4 : STATE_1;
               }

           break;

           // State 1 :
           // Rampe de courant sur les inducteurs 1&2 jusqu'a atteindre les 3A
           case STATE_1:

               if(ic1 < I_SP)
               {
                   ic1 += TAKEOFF_CURRENT_STEP1;
               }
               else
               {
                   ic1 = I_SP;
               }

               ic2 = ic1;

               mean1 += Current1;
               mean2 += Current2;
               dt_mean++;

               if(dt_mean >= 200)
               {
                   mean1 = mean1 / dt_mean;
                   mean2 = mean2 / dt_mean;

                   if((mean1 <= I_SP105 && mean1 >= I_SP095) && (mean2 <= I_SP105 && mean2 >= I_SP095))
                   {
                       mean1 = 0;
                       mean2 = 0;
                       dt_mean = 0;

                       state = STATE_2;
                   }
                   else
                   {
                       mean1 = 0;
                       mean2 = 0;
                       dt_mean = 0;
                   }
               }

               break;

           // State 2 :
           // Rampe de courant "infinie" jusqu'au decollage de l'avant
           case STATE_2:

               if(Position1 <= Position_c1_dec && Position2 <= Position_c2_dec)
               {
                   Position_c1 = Position1;
                   Position_c2 = Position2;

                   xr1 = 0.0f;
                   xr2 = 0.0f;
                   pos_est1 = Position1; vit_est1 = 0.0f; for_est1 = 0.0f;
                   pos_est2 = Position2; vit_est2 = 0.0f; for_est2 = 0.0f;

                   state = STATE_3;
               }
               else if(ic1 < 9.5f)
               {
                   ic1 += TAKEOFF_CURRENT_STEP1;
               }
               else
               {
                   ic1 = 9.5f;
               }

               ic2 = ic1;

               break;

           // State 3 :
           // Regulation SISO de l'avant, rampe de consigne 3mm -> 2mm
           case STATE_3:

               if(!PosRegFlag1) PosRegFlag1 = true;

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

               i_store++;

               if(i_store == I_STORE_2E_DECOLLAGE)
               {
                   i_store = 0;

                   state = SKIP_BACK ? STATE_7 : STATE_4;
               }

               break;

           // State 4 :
           // Rampe de courant sur les inducteurs 3&4 jusqu'a atteindre les 3A
           case STATE_4:

               if(ic3 < I_SP)
               {
                   ic3 += TAKEOFF_CURRENT_STEP1;
               }
               else
               {
                   ic3 = I_SP;
               }

               ic4 = ic3;

               mean3 += Current3;
               mean4 += Current4;
               dt_mean++;

               if(dt_mean >= 200)
               {
                   mean3 = mean3 / dt_mean;
                   mean4 = mean4 / dt_mean;

                   if((mean3 <= I_SP105 && mean3 >= I_SP095) && (mean4 <= I_SP105 && mean4 >= I_SP095))
                   {
                       mean3 = 0;
                       mean4 = 0;
                       dt_mean = 0;

                       state = STATE_5;
                   }
                   else
                   {
                       mean3 = 0;
                       mean4 = 0;
                       dt_mean = 0;
                   }
               }

               break;

           // State 5 :
           // Rampe de courant "infinie" jusqu'au decollage de l'arriere,
           // puis bascule bumpless en regulation MIMO
           case STATE_5:

               if(Position3 <= Position_c3_dec && Position4 <= Position_c4_dec)
               {
                   uint16_t j;
                   float f_init0 = fc1f;
                   float f_init1 = fc2f;
                   float f_init2 = F_STAT[2];
                   float f_init3 = F_STAT[3];

                   for(j = 0; j < 3; j++)
                   {
                       float up = E_MAT[j][0]*f_init0 + E_MAT[j][1]*f_init1
                                + E_MAT[j][2]*f_init2 + E_MAT[j][3]*f_init3 - U_STAT[j];
                       qm_ref[j] = qm[j];
                       qm_est[j] = qm[j];
                       vm_est[j] = 0.0f;
                       um_est[j] = up;
                       eps_m[j]  = -up * LQI_EPS_INV[j];
                       u_cmd[j]  = up;
                       u_sat[j]  = up;
                       u_ach[j]  = up;
                   }

                   IN3[1] = F_STAT[2]; IN3[2] = F_STAT[2];
                   OUT3[0] = F_STAT[2]; OUT3[1] = F_STAT[2];
                   IN4[1] = F_STAT[3]; IN4[2] = F_STAT[3];
                   OUT4[0] = F_STAT[3]; OUT4[1] = F_STAT[3];

                   MimoFlag = true;

                   state = STATE_6;
               }
               else if(ic3 < 9.5f)
               {
                   ic3 += TAKEOFF_CURRENT_STEP1;
               }
               else
               {
                   ic3 = 9.5f;
               }

               ic4 = ic3;


               break;

           // State 6 :
           // Attend que le bouton soit presse a nouveau pour eteindre la maquette
           case STATE_6:

               if(!SystemOn)
               {
                   uint16_t j;

                   GPIO_writePin(LED_D2, 1);

                   for(i = 0; i < NUM_OF_PWM_CHANNEL; i++)
                   {
                       dutyFine = ((float)(duty_cycle_table[i] * TIME_BASE_PERIOD) * INV_FACTOR);
                       compCount = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
                       HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
                       HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
                   }

                   PosRegFlag1 = false;
                   MimoFlag = false;

                   i_store = 0;

                   integral_i1 = 0.0f; ue1 = 0.0f; ic1 = 0.0f;
                   integral_i2 = 0.0f; ue2 = 0.0f; ic2 = 0.0f;
                   integral_i3 = 0.0f; ue3 = 0.0f; ic3 = 0.0f;
                   integral_i4 = 0.0f; ue4 = 0.0f; ic4 = 0.0f;

                   xr1 = 0.0f; ep1 = 0.0f;
                   xr2 = 0.0f; ep2 = 0.0f;

                   for(j = 0; j < 3; j++)
                   {
                       eps_m[j] = 0.0f;
                       u_cmd[j] = 0.0f;
                       u_sat[j] = 0.0f;
                       u_ach[j] = 0.0f;
                       vm_est[j] = 0.0f;
                       um_est[j] = 0.0f;
                       qm_est[j] = qm[j];
                       qm_ref[j] = qm_target[j];
                   }

                   status = SFO();
                   if (status == SFO_ERROR) error();

                   state = STATE_0;
               }

               break;

           default:
               break;
       }

    /* --------------------------------------------------------------------- *
     * 6. REGULATION
     * --------------------------------------------------------------------- */
    if(state != STATE_0)
    {

        // ================================================================= //
        // B1. REGULATION SISO AVANT (etats 3 a 5, avant bascule MIMO)
        // ================================================================= //
        if(PosRegFlag1 && !MimoFlag)
        {
            ep1 = Position_c1 - Position1;
            fc1_prim = FP + LQI1_Q*(Position1 - Position_c1) + LQI1_QD*vit_est1 - LQI1_EPS*xr1*I;

            fc1 = fc1_prim;
            bool sat1 = false;
            if (fc1_prim <= 0)      { fc1 = 0;     sat1 = true; }
            else if (fc1_prim >= FMAX1) { fc1 = FMAX1; sat1 = true; }
            if (!sat1) xr1 += ep1 * H;

            IN1[0] = fc1;
            fc1f = IIR_Filter(IN1, OUT1);
            if (fc1f < 0) fc1f = 0; else if (fc1f > FMAX1) fc1f = FMAX1;
            ic1 = sqrtf(K_FC1 * fc1f) * Position1;

            ep2 = Position_c2 - Position2;
            fc2_prim = FP + LQI2_Q*(Position2 - Position_c2) + LQI2_QD*vit_est2 - LQI2_EPS*xr2*I;

            fc2 = fc2_prim;
            bool sat2 = false;
            if (fc2_prim <= 0)      { fc2 = 0;     sat2 = true; }
            else if (fc2_prim >= FMAX2) { fc2 = FMAX2; sat2 = true; }
            if (!sat2) xr2 += ep2 * H;

            IN2[0] = fc2;
            fc2f = IIR_Filter(IN2, OUT2);
            if (fc2f < 0) fc2f = 0; else if (fc2f > FMAX2) fc2f = FMAX2;
            ic2 = sqrtf(K_FC2 * fc2f) * Position2;
        }

        // ================================================================= //
        // B2. REGULATION MIMO (torseur LQI + allocation)
        // ================================================================= //
        if(MimoFlag)
        {
            uint16_t j;

            for(j = 0; j < 3; j++)
            {
                qm_ref[j] += (qm_target[j] - qm_ref[j]) * REF_SMOOTH;
                u_cmd[j] = LQI_Q[j]*(qm[j] - qm_ref[j]) + LQI_QD[j]*vm_est[j] - LQI_EPS[j]*eps_m[j]*I;
            }

            fc1 = F_STAT[0] + W_MAT[0][0]*u_cmd[0] + W_MAT[0][1]*u_cmd[1] + W_MAT[0][2]*u_cmd[2];
            fc2 = F_STAT[1] + W_MAT[1][0]*u_cmd[0] + W_MAT[1][1]*u_cmd[1] + W_MAT[1][2]*u_cmd[2];
            fc3 = F_STAT[2] + W_MAT[2][0]*u_cmd[0] + W_MAT[2][1]*u_cmd[1] + W_MAT[2][2]*u_cmd[2];
            fc4 = F_STAT[3] + W_MAT[3][0]*u_cmd[0] + W_MAT[3][1]*u_cmd[1] + W_MAT[3][2]*u_cmd[2];

            if (fc1 < 0) fc1 = 0; else if (fc1 > FMAX1) fc1 = FMAX1;
            if (fc2 < 0) fc2 = 0; else if (fc2 > FMAX2) fc2 = FMAX2;
            if (fc3 < 0) fc3 = 0; else if (fc3 > FMAX3) fc3 = FMAX3;
            if (fc4 < 0) fc4 = 0; else if (fc4 > FMAX4) fc4 = FMAX4;

            for(j = 0; j < 3; j++)
            {
                u_sat[j] = E_MAT[j][0]*fc1 + E_MAT[j][1]*fc2
                         + E_MAT[j][2]*fc3 + E_MAT[j][3]*fc4 - U_STAT[j];

                float def = u_cmd[j] - u_sat[j];
                if (def > -AW_TOL[j] && def < AW_TOL[j])
                {
                    eps_m[j] += (qm_ref[j] - qm[j]) * H;
                }
            }

            IN1[0] = fc1;
            fc1f = IIR_Filter(IN1, OUT1);
            if (fc1f < 0) fc1f = 0; else if (fc1f > FMAX1) fc1f = FMAX1;
            ic1 = sqrtf(K_FC1 * fc1f) * Position1;

            IN2[0] = fc2;
            fc2f = IIR_Filter(IN2, OUT2);
            if (fc2f < 0) fc2f = 0; else if (fc2f > FMAX2) fc2f = FMAX2;
            ic2 = sqrtf(K_FC2 * fc2f) * Position2;

            IN3[0] = fc3;
            fc3f = IIR_Filter(IN3, OUT3);
            if (fc3f < 0) fc3f = 0; else if (fc3f > FMAX3) fc3f = FMAX3;
            ic3 = sqrtf(K_FC3 * fc3f) * Position3;

            IN4[0] = fc4;
            fc4f = IIR_Filter(IN4, OUT4);
            if (fc4f < 0) fc4f = 0; else if (fc4f > FMAX4) fc4f = FMAX4;
            ic4 = sqrtf(K_FC4 * fc4f) * Position4;

            for(j = 0; j < 3; j++)
            {
                u_ach[j] = E_MAT[j][0]*fc1f + E_MAT[j][1]*fc2f
                         + E_MAT[j][2]*fc3f + E_MAT[j][3]*fc4f - U_STAT[j];
            }
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
         * 7. MISE A JOUR DES PWM & CALIBRATION SFO
         * --------------------------------------------------------------------- */

        duty_table[0] = dutyCycle1 * 100;
        duty_table[1] = dutyCycle2 * 100;
        duty_table[2] = dutyCycle3 * 100;
        duty_table[3] = dutyCycle4 * 100;

        for (i = 0; i < NUM_OF_PWM_CHANNEL; i++)
        {
            if (duty_table[i] >= LIMITE_MAX_DUTY_FINE) {
                duty_table[i] = LIMITE_MAX_DUTY_FINE;
            }
            else if (duty_table[i] <= LIMITE_MIN_DUTY_FINE) {
                duty_table[i] = LIMITE_MIN_DUTY_FINE;
            }

            dutyFine = ((float)(duty_table[i] * TIME_BASE_PERIOD) * INV_FACTOR);
            compCount = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
            HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
        }

        status = SFO();
        if (status == SFO_ERROR) error();
    }

    /* --------------------------------------------------------------------- *
     * 8. ACQUITTEMENTS & FLAGS (ADC)
     * --------------------------------------------------------------------- */
    ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);
    if(ADC_getInterruptOverflowStatus(myADC0_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(myADC0_BASE, ADC_INT_NUMBER1);
        ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);
    }
    Interrupt_clearACKGroup(INT_myADC0_1_INTERRUPT_ACK_GROUP);

    /* --------------------------------------------------------------------- *
     * 9. DEBOUNCE BOUTON POUSSOIR EXTERNE
     * --------------------------------------------------------------------- */
    if(Ext_Int_Flag)
    {

        count_ext_int++;

        if(count_ext_int >= COUNT_TO_REACH)
        {
            if(GPIO_readPin(Push_Button_Start) == 0)
            {
                SystemOn = !SystemOn;
            }

            Ext_Int_Flag = false;
            count_ext_int = 0;
        }

    }

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
            dataIndex = rxIndex;
        }

        if(c == '\x03'){
            if(dataIndex > 0 && dataIndex < rxIndex - 1) {
                SystemOn = (rxBuffer[dataIndex] == '1');
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

// --- UART Transmit (TX) ---
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
