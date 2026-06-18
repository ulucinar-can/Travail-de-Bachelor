//###########################################################################
//
// FILE:    hrpwm_ex1_duty_sfo.c
//
// TITLE:   Power command and regulation for magnetic sustenance
//
// AUTHOR : Thomas Freyche - 2025
//
//###########################################################################
//
//
//###########################################################################
// Description :
//
// Algorithme de r�gulation repris de M. Franck Yersin et M.Isoz.
// Le code � �t� r�aliser pour correspondre au mieux aux besoin de
// la maquette dans sa derni�re version.
//
// Optimisation du compilateur (2025) :
// - Level : 3 - Interprocedure Optimization
// - Speed vs size trade : 5 (speed)
// - Floating point mode : relaxed
// - Allow Reassociation of FP arithmetic : on
//
// Code r�alis� :
//
//      - Optimisation du code pour s'approcher des 40us de temps
//        d'ex�cution souhait�
//      - Modification de calcul de conversion optimis�
//      - Correction de lignes PWM pour faire fonctionner la sustentation
//###########################################################################

//
// Included Files
//

#include "driverlib.h"
#include "device.h"

#include "board.h"
#include "sfo_v8.h"

#include "math.h"
#include "stdio.h"
#include "FunctionHeader.h"

//
// Defines
//

#define LIMITE_MAX_DUTY_FINE 80 // en %
#define LIMITE_MIN_DUTY_FINE 30

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

#define COUNT_TO_REACH              10000 //POUR ANTI REBOND   10'000 X 40 us = 400ms

// Rapport cyclique de base a 50% -> courant theorique de 0A dans les inducteurs
// (Forcer les PWMs � 50%, sinon de base � 0% et PWMA = 0 et PWMB = 1 (fort courant car 1 transistor toujours enclench�))
#define DUTY_CYCLE_1 50
#define DUTY_CYCLE_2 50
#define DUTY_CYCLE_3 50
#define DUTY_CYCLE_4 50

//
// Globals
//

// Global variables used
const float ADC_ZERO_CURRENT = 1861.36f;
float Position1 = DELTA_0, Position2 = DELTA_0, Position3 = DELTA_0, Position4 = DELTA_0;
float Current1 = 0, Current2 = 0, Current3 = 0, Current4 = 0;
float dutyCycle1 = 0.5, dutyCycle2 = 0.5, dutyCycle3 = 0.5, dutyCycle4 = 0.5;
float uc1 = 0, uc2 = 0, uc3 = 0, uc4 = 0;

float mean1 = 0, mean2 = 0, mean3 = 0, mean4 = 0;
unsigned int dt_mean = 0;

//###########################################################################
// Current control inductor 1
float ic1 = 0.0;                                      // Current set point
float ue1 = 0.0;                                      // voltage error from limitation
float integral_i1 = 0;
//float Kp_i = KP_I, Ti_i = TI_I, Gi_i = GI_I;
//float ie1 = 0.0;                                      // Current error = iw - Current
//float uc1_prim = 0;                                   // voltage before limitation
//float antiwindup = ANTIWINDUP_EN;                     // Antiwindup enable/disable
//float fperturb1 = 0;

// Inverse transform method  inductor 1
float fc1 = 0;                                      // force set point

// Position control inductor 1
float pos1Buff[FILTWINDOW] = {[0 ... 8] = 0};       //buffer for savitzky speed calculation
float v1 = 0;
float I = 1;                                        //enable disable integrators
float Kw = KW, Kd = KD, Kddot = KDDOT, Kr = KR, K_antiwindup = K_ANTIWINDUP;
float Kw_change = KW_CHANGE, Kd_change = KD_CHANGE, Kddot_change = KDDOT_CHANGE, Kr_change = KR_CHANGE;
float Kw_sans_int = KW_SANS_INT, Kd_sans_int = KD_SANS_INT, Kddot_sans_int = KDDOT_SANS_INT, Kr_sans_int = KR_SANS_INT;
float ep1 = 0;
float Position_c1 = DELTA_0, Position_c2 = DELTA_0;
float Position_c1_dec = 0, Position_c2_dec = 0, Position_c3_dec = 0;
float Position2_c3 = DELTA_0, Position2_c4 = DELTA_0;
float xr1 = 0;  //(DELTA_N * KW - DELTA_0 * KD + DELTA_0 - DELTA_N)*INV_KR;
float fce1 = 0;
float sum_vp1 = 0;
float fc1_prim = 0;
float antiwindup_pos = ANTIWINDUP_EN;
float Pos1_to_regul = DELTA_N, Pos2_to_regul = DELTA_N, Pos3_to_regul = DELTA_N, Pos4_to_regul = DELTA_N;
uint32_t i_store_change_poles_placement = I_STORE_CHANGE_POLES_PLACEMENT, i_store_2e_decollage = I_STORE_2E_DECOLLAGE;

// PID
float dpos1 = 0;
float kep1 = 0;
// Valeur rapport dutruel p 60 + modif
float Kp_pid = 2050;
float Ki_pid = 100;
float Kd_pid = -75e1;
float K_antiwindupPID = 1;

//###########################################################################
// Current control inductor 2
float ic2 = 0;                                      // Current set point
float ue2 = 0;                                      // voltage error from limitation
float integral_i2 = 0;
//float ie2 = 0;                                      // Current error = iw - Current
//float uc2_prim = 0;                                 // voltage before limitation

// Inverse transform method inductor 2
float fc2 = 0;                                      // force set point

// Position control inductor 2
float pos2Buff[FILTWINDOW] = {[0 ... 8] = 0};
float v2 = 0;
float ep2 = 0;
float xr2 = 0;
float fce2 = 0;
float sum_vp2 = 0;
float fc2_prim = 0;

// PID
float dpos2 = 0;
float kep2 = 0;

//###########################################################################
// Current control inductor 3
float ic3 = 0;                                      // Current set point
float ue3 = 0;                                      // voltage error from limitation
float integral_i3 = 0;
//float ie3 = 0;                                    // Current error = iw - Current
//float uc3_prim = 0;                               // voltage before limitation

// Inverse transform method inductor 3
float fc3 = 0;                                      // force set point

// Position control inductor 3
float pos3Buff[FILTWINDOW] = {[0 ... 8] = 0};
float v3 = 0;
float ep3 = 0;
float xr3 = 0;
float fce3 = 0;
float sum_vp3 = 0;
float fc3_prim = 0;

//###########################################################################
// Current control inductor 4
float ic4 = 0;                                      // Current set point
float ue4 = 0;                                      // voltage error from limitation
float integral_i4 = 0;
//float ie4 = 0;                                    // Current error = iw - Current
//float uc4_prim = 0;                               // voltage before limitation

// Inverse transform method inductor 4
float fc4 = 0;                                      // force set point

// Position control inductor 4
float pos4Buff[FILTWINDOW] = {[0 ... 8] = 0};
float v4 = 0;
float ep4 = 0;
float xr4 = 0;
float fce4 = 0;
float sum_vp4 = 0;
float fc4_prim = 0;

// Angle entre inducteur 1 et inducteur 2
//float theta = 0.0;

// Control enable/disable sustain
bool takeOff = 1;
bool takeOff2 = 1;
bool phase1 = 1;

// Variables for the regulation switch
uint32_t i_store = 0;

//savitzky filter coefficients, order = 2, window = 9
const float SAVITZKY[FILTWINDOW] = {-0.0667, -0.05, -0.0333, -0.0167, 0.0, 0.0167, 0.0333, 0.05, 0.0667};

//bandstop Filter (initialise to the 75Hz large bandstop filter design)
float a[Na] ={A1_B1_0,    A2_0};
float b[Nb] ={B0_B2_0,A1_B1_0, B0_B2_0};

float fc1f = 0.0;                   //buffers for inductor 1
float IN1[Nb] ={[0 ... 2] = 0};
float OUT1[Na] = {[0 ... 1] = 0};

float fc2f = 0.0;                   //buffers for inductor 2
float IN2[Nb] ={[0 ... 2] = 0};
float OUT2[Na] = {[0 ... 1] = 0};

float fc3f = 0.0;                   //buffers for inductor 3
float IN3[Nb] ={[0 ... 2] = 0};
float OUT3[Na] = {[0 ... 1] = 0};

float fc4f = 0.0;                   //buffers for inductor 4
float IN4[Nb] ={[0 ... 2] = 0};
float OUT4[Na] = {[0 ... 1] = 0};

//float i01 = 0,i02 = 0,i03 = 0,i04 = 0; // Pas utilis�

// Variable pour le rapport cyclique
float dutyFine = MIN_HRPWM_DUTY_PERCENT;
float duty1 = 50,duty2 = 50,duty3 = 50,duty4 = 50;
float duty_table[] = {0, DUTY_CYCLE_1, DUTY_CYCLE_2, DUTY_CYCLE_3, DUTY_CYCLE_4};
float duty_int = 50;
float count = 0;
uint32_t compCount = 0;
uint16_t i = 1;
uint16_t status;

// Variable pour les EPWMs
volatile uint32_t ePWM[] =
    {0, myEPWM1_BASE,myEPWM2_BASE,myEPWM3_BASE,myEPWM4_BASE};

// Variable pour le boutton poussoir (S2) pour la sustentation
uint16_t start_ISOZ = 0;
uint16_t stop_ISOZ = 0;
uint16_t count_ext_int = 0;
uint16_t Ext_Int_Flag = 0;
bool state_PIN = 0; // etat du bouton via l'interface utilisateur sur l'esp32-s2

// Variable pour l'offset du conditionnement au demarrage
bool Offset_stop = 0;
float Offset_count = 0;
float Offset_ADC1 = 0, Offset_ADC2 = 0, Offset_ADC3 = 0, Offset_ADC4 = 0;

// Variables globales pour la communication
volatile uint16_t UartCounter = 0;
#define TX_BUF_LEN 64
volatile char txBuffer[TX_BUF_LEN];
volatile uint16_t txIndex = 0;
volatile uint16_t txLength = 0;

#define RX_BUF_LEN 64
volatile char rxBuffer[RX_BUF_LEN];
volatile uint16_t rxIndex = 0;
volatile bool frameReady = false;
static uint16_t dataIndex = 0;

// buffer pour la lecture des adc de courant et position
uint16_t ADC_pos_1,ADC_pos_2,ADC_pos_3,ADC_pos_4,ADC_cur_1,ADC_cur_2,ADC_cur_3,ADC_cur_4;

int MEP_ScaleFactor; // Global variable used by the SFO library
                     // Result can be used for all HRPWM channels
                     // This variable is also copied to HRMSTEP
                     // register by SFO() function.

//
// Function Prototypes
//
void error(void);

//////////////////////////////////////////////////////////////////////////////
//                Current regulation function prototype                     //
//////////////////////////////////////////////////////////////////////////////
void PI_current_regulator(float ic, float current, float *integral, float *ue, float *uc){
    float ie = (ic - current) * KP_I;
    *integral += (ie - *ue) * GI_I * H;
    float uc_prim = ie + *integral;

    *uc = uc_prim;
    if (uc_prim > UMAX) {
        *uc = UMAX;
    }
    if (uc_prim < 0) {
        *uc = 0;
    }

    *ue = (uc_prim - *uc) * ANTIWINDUP_EN;
}

//////////////////////////////////////////////////////////////////////////////
//                Savitzky filter for speed calculation(FIR)                //
//////////////////////////////////////////////////////////////////////////////
float savitzky_Filter(float *Buffer)
{
    float v = 0.0f;
    uint16_t m = 0;

    //savitzky algorithm
        for(m = 0; m < FILTWINDOW; m++)
            v += F_PWM*Buffer[m]*SAVITZKY[m];

    //update buffer for next position
        for(m = 0; m < FILTWINDOW-1; m++)
           Buffer[m] = Buffer[(m+1)];

    return v;
}
//////////////////////////////////////////////////////////////////////////////
//                          IIR filter 2nd order                            //
//////////////////////////////////////////////////////////////////////////////
float IIR_Filter(float *entree, float *sortie)
{
    float somme = 0.0;
    float somme2 = 0.0;

    uint16_t k = 0;

    //difference equation
        for(k = 0; k < Nb; k++) //Num.
            somme += b[k]*entree[k];
        for(k = 0; k < Na; k++) //Den.
            somme2 += a[k]*sortie[k];

    //update buffers
        for(k = (Na-1); k > 0; k--)
            sortie[k] = sortie[(k-1)];
        sortie[0] = (somme-somme2);

        for(k = (Nb-1); k > 0; k--)
                entree[k] = entree[(k-1)];

        return (somme-somme2);
}

// Test 1 - "Hello World"
//void sendString(const char *msg)
//{
//    uint16_t n = 0;
//    while (msg[n] != '\0' && n < TX_BUF_LEN) {
//        txBuffer[n] = msg[n];
//        n++;
//    }
//
//    txLength = n;
//    txIndex = 0;
//
//    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXFF); // Lance l'envoi
//}

// Test 2 - floats
// Source : https://www.geeksforgeeks.org/cpp/convert-floating-point-number-string
// Reverses a string 'str' of length 'len'
void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

// Converts a given integer x to string str[].
// d is the number of digits required in the output.
// If d is more than the number of digits in x,
// then 0s are added at the beginning.
int ConvertIntToStr(int x, char str[], int p)
{
    int i = 0;
    if (x == 0) str[i++] = '0';
    else {
        while (x) {
            str[i++] = (x % 10) + '0';
            x /= 10;
        }
        while (i < p) str[i++] = '0';
        reverse(str, i);
    }
    str[i] = '\0';
    return i;
}

int ftoa(float n, char* res, int afterpoint)
{
    int i = 0;

    if (n < 0) {
        res[i++] = '-';
        n = -n;
    }

    int ipart = (int)n;
    float fpart = n - (float)ipart;

    i += ConvertIntToStr(ipart, res + i, 0);

    if (afterpoint > 0) {
        res[i++] = '.';
        fpart *= powf(10.0f, (float)afterpoint);
        i += ConvertIntToStr((int)(fpart + 0.5f), res + i, afterpoint);
    }

    res[i] = '\0';
    return i;
}

void SendFloatAsText(float f0, float f1, float f2, float f3)
{
    char str[TX_BUF_LEN];
    int i = 0, j = 0;

// Mise en forme de la trame envoy�e
    str[i++] = '\x02'; // start bit

    i += ftoa(f0, &str[i], 3);
    str[i++] = ',';

    i += ftoa(f1, &str[i], 3);
    str[i++] = ',';

    i += ftoa(f2, &str[i], 3);
    str[i++] = ',';

    i += ftoa(f3, &str[i], 3);

    str[i++] = '\x03'; // stop bit

    for (j = 0; j < i && j < TX_BUF_LEN; j++) {
        txBuffer[j] = (uint8_t)str[j];
    }

    txLength = i;
    txIndex = 0;

    // Envoi de la trame
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXFF);
}

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pull ups.
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Calling SFO() updates the HRMSTEP register with calibrated MEP_ScaleFactor.
    // HRMSTEP must be populated with a scale factor value prior to enabling
    // high resolution period control.
    //
    while(status == SFO_INCOMPLETE)
    {
        status = SFO();
        if(status == SFO_ERROR)
        {
            error();   // SFO function returns 2 if an error occurs & # of MEP
        }              // steps/coarse step exceeds maximum of 255.
    }

    //
    // Disable sync(Freeze clock to PWM as well)
    //
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // Initialize the EPWM GPIO Pins, the SCI and change the XBAR inputs from using GPIO0
    //
    Board_init();
    // SCI_enableInterrupt(mySCI0_BASE, SCI_INT_RXFF); // Autorise l'interruption Rx

    //
    // Enable sync and clock to PWM
    //
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    //
    EINT;
    ERTM;

    //Affection des PWMs sur les x inducteurs (en fonction de NUM_OF_PWM_CHANNEL, 4 par default)
    for(i = 1;i<1+NUM_OF_PWM_CHANNEL;i++)
    {
        dutyFine = ((float)(duty_table[i]*TIME_BASE_PERIOD) * INV_FACTOR);
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
    for(;;)
    {

    }
}

//
// error - Halt debugger when called
//
void error (void)
{
    ESTOP0;         // Stop here and handle error
}

// Interruption contenant les conversions, l'algorithme de r�gulation, les PWM et
// la lecture du bouton poussoir
__interrupt void adcA1ISR(void)
{

    //////////////////////////////////////////////////////////////////////////////
    //                          Lecture ADC                                     //
    //////////////////////////////////////////////////////////////////////////////

    // Lecture de la position
        ADC_pos_1 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
        ADC_pos_2 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1);
        ADC_pos_3 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER2);
        ADC_pos_4 = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER3);

    // Lecture du courant, pour pinout, voir syscfig SOCx configurations - Device Pin Name
        ADC_cur_1 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER0);
        ADC_cur_2 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER1);
        ADC_cur_3 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER2);
        ADC_cur_4 = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER3);

     // Lecture de l'offset du conditionnement par rapport a la valeur theorique 0A au d�but de l'interruption (1 fois)
     // Adaptation de l'offset : 1.5 / 3.3 * 4095 = 1861.36 (1.5V correspond a un courant de 0A = point de reference)
    if(Offset_count <= 999 && Offset_stop == 0)
         {
             Offset_ADC1 += ((float)ADC_cur_1) - ADC_ZERO_CURRENT;
             Offset_ADC2 += ((float)ADC_cur_2) - ADC_ZERO_CURRENT;
             Offset_ADC3 += ((float)ADC_cur_3) - ADC_ZERO_CURRENT;
             Offset_ADC4 += ((float)ADC_cur_4) - ADC_ZERO_CURRENT;
             Offset_count++;

             // Moyenne pour definir l'offset de mesure de courant
             if(Offset_count > 999){

                 Offset_ADC1 = Offset_ADC1 * OFFSET_COUNT_INV;
                 Offset_ADC2 = Offset_ADC2 * OFFSET_COUNT_INV;
                 Offset_ADC3 = Offset_ADC3 * OFFSET_COUNT_INV;
                 Offset_ADC4 = Offset_ADC4 * OFFSET_COUNT_INV;

                 Offset_stop = 1; // Arret de l'echantillonnage

                 Position_c1_dec = Position1 * POS_DETECT; // Mesure de position une fois stabilis�e
                 Position_c3_dec = Position3 * POS_DETECT;

                 // Test pour 1 inducteur
                 Position_c2_dec = Position2 * POS_DETECT;
             }
         }

     /////////////////////////////////////////////
     // Conversion 12 bits -> position          //
     /////////////////////////////////////////////

     Position1 = (float)(ADC_pos_1) * CONV_POS2 * POS_CORRECTION_1; // 3722.7 / 3500 = 1.06, valeur 12 bit pour 3mm / valeur mesuree ADC (2.8mm) = ecart
     Position2 = (float)(ADC_pos_2) * CONV_POS2 * POS_CORRECTION_2; // 3722.7 / 3480 = 1.07, valeur 12 bit pour 3mm / valeur mesuree ADC (2.8mm) = ecart
     Position3 = (float)(ADC_pos_3) * CONV_POS2 * POS_CORRECTION_1;
     Position4 = (float)(ADC_pos_4) * CONV_POS2 * POS_CORRECTION_2;


     /////////////////////////////////////////////
     // Conversion 12 bits -> courant           //
     /////////////////////////////////////////////

      // Calcul du courant version la plus avancee : TFE 2025
      Current1     = (((float)(ADC_cur_1) - ADC_ZERO_CURRENT - Offset_ADC1) / (ADC_ZERO_CURRENT + Offset_ADC1)) * I_MAX;
      Current2     = (((float)(ADC_cur_2) - ADC_ZERO_CURRENT - Offset_ADC2) / (ADC_ZERO_CURRENT + Offset_ADC2)) * I_MAX;
      Current3     = (((float)(ADC_cur_3) - ADC_ZERO_CURRENT - Offset_ADC3) / (ADC_ZERO_CURRENT + Offset_ADC3)) * I_MAX;
      Current4     = (((float)(ADC_cur_4) - ADC_ZERO_CURRENT - Offset_ADC4) / (ADC_ZERO_CURRENT + Offset_ADC4)) * I_MAX;



      // Envoi des donn�es UART toute les 40*10e-6 * 25000 = 1 s
//      UartCounter++;
//
//      if (UartCounter >= 25000) {
//          UartCounter = 0; // Reset du compteur
//          SendFloatAsText(Position1*1000.0f,Position2*1000.0f,Position3*1000.0f,Position4*1000.0f);
//
////          SendFloatAsText(1.234f,2.567f,3.891,4.234f); // Trame de test envoy�e : "\x02float1,float2,float3float4\x03" avec les float sous cette forme x.xxx
////          sendString("\x02Hello World!\r\x03");
//      }

//--------------------------------------------------------------------------------------------------------------------------
     if(start_ISOZ == 1 || state_PIN == 1) // SI bouton poussoir press� physique ou sur page web -> debut de la sustentation
     {


         // allumage lors de la sustentation, led2 d'indication sur DSP
         // GPIO_writePin(LED_D2,0);

         //////////////////////////////////////////////////////////////////////////////
         //                          Savitzky speed calulation                       //
         //////////////////////////////////////////////////////////////////////////////

         pos1Buff[FILTWINDOW-1] = Position1;
         v1 = savitzky_Filter(pos1Buff);

         pos2Buff[FILTWINDOW-1] = Position2;
         v2 = savitzky_Filter(pos2Buff);

         pos3Buff[FILTWINDOW-1] = Position3;
         v3 = savitzky_Filter(pos3Buff);

         pos4Buff[FILTWINDOW-1] = Position4;
         v4 = savitzky_Filter(pos4Buff);

         //////////////////////////////////////////////////////////////////////////////
         //                          Take off strategy 1                             //
         //////////////////////////////////////////////////////////////////////////////

         if(takeOff == 1)
         {
             //////////////////////////PHASE 1 : INDUCTOR 1 & 2 //////////////////////////////////////////

            // Premiere phase de decollage : rampe de courant sur chaque inducteur jusqu'a 3A.
            if(phase1 == 1) //mean calculation
            {
                mean1 += Current1;
                mean2 += Current2;
//                mean3 += Current3;
//                mean4 += Current4;
                dt_mean++;

              // if current has reached final value of initial current input(I_SP)
              // phase 1 end and phase2 begins
                if(dt_mean == 200)//dt = 8ms
                {
                   mean1 = mean1 / dt_mean;
                   mean2 = mean2 / dt_mean;
//                   mean3 = mean3 / dt_mean;
//                   mean4 = mean4 / dt_mean;

                   // Rampe generale de courant pour les 4 inducteurs
                   if(mean1 <= I_SP105 && mean1 >= I_SP095 && mean2 <= I_SP105 && mean2 >= I_SP095){

                          phase1 = 0;
                   }
                   else
                   {
                       mean1 = 0;
                       mean2 = 0;
//                       mean3 = 0;
//                       mean4 = 0;
                       dt_mean = 0;
                   }
                }

              // Set point generator(Current)
                if(ic1 < I_SP)
                {
                    ic1 += TAKEOFF_CURRENT_STEP1; // 0.04 ramp from 0A to 3A (4e-7 [s])
                }
                else
                {
                    ic1 = I_SP;
                    ic2 = ic1;
//                    ic3 = ic1;
//                    ic4 = ic1;
                }
            }
            else // phase2 for inductor 1,2
            {
               if(Position1 < Position_c1_dec) // if inductor starts to move
               {
                   takeOff = 0;
                   Position_c1 = Position1;
               }
               // Current limiting at value necessary to take off inductor at Delta_0 (3mm)
               else if(ic1 < 7.82f){
                   ic1 += TAKEOFF_CURRENT_STEP1; //"infinite" ramp until inductor starts to move
               }
               else
               {
//                   if(ic1 < 9.0f)
//                       ic1 += 0.004f; // rampe plus douce, pour assurer la levitation du sustentateur
//                   else
//                       ic1 = 9.0f;
                   ic1 = 7.82;
               }
               ic2 = ic1;
            }
         }
         // Start takeoff 1 & 2//
         else
         {

         //SET POINT GENERATOR FOR INDUCTOR 1 & 2                                       //
         // Ce bloc permet de baisser petit � petit la consigne de position pour que    //
         // la regulation puisse suivre et s'adapter                                    //
             if(Position_c1 > Pos1_to_regul)
             {
                 Position_c1 -= 2.5 * 4e-7; // ramp from 3mm to 2mm with a 0.25 mm step
             }
             else
             {
                 Position_c1 = Pos1_to_regul;
             }

             if(Position_c2 > Pos2_to_regul)
             {
                 Position_c2 -= 2.5 * 4e-7; // ramp from 3mm to 2mm
             }
             else
             {
                 Position_c2 = Pos2_to_regul;
             }

        //////////////////////////////////////////////////////////////////////////////
        //                STATE METHOD REGULATION inductor 1 & 2                    //
        //           With embedded system, all integrators must be removed          //
        //////////////////////////////////////////////////////////////////////////////

        //Fc1 bandstop filter
             IN1[0] = fc1;
             fc1f = IIR_Filter(IN1,OUT1);
             //fc1f = fc1;
             //introducing limit because IIR filter may cause overtaking
             if (fc1f <= 0)
                 fc1f = 0;
             if (fc1f >= FMAX)
                 fc1f = FMAX;

        // Position control inductor 1 - state regulation
             ep1 = Position_c1 - Position1;
             xr1 += (ep1 - fce1);
             sum_vp1 = v1 * Kddot + Position1 * Kd;
             fc1_prim = Kw * Position_c1 + Kr * xr1 * I - sum_vp1 + FP;
//             fc1_prim = fc1_prim - fperturb1;
             fc1 = fc1_prim;

             if (fc1_prim <= 0){
                 fc1 = 0;
             }
             if (fc1_prim >= FMAX){
                 fc1 = FMAX;
             }
             fce1 = (fc1_prim - fc1) * K_antiwindup * antiwindup_pos;

        //Fc2 bandstop filter
             IN2[0] = fc2;
             fc2f = IIR_Filter(IN2,OUT2);
             //introducing limit because IIR filter may cause overtaking
             if (fc2f <= 0)
                 fc2f = 0;
             if (fc2f >= FMAX)
                 fc2f = FMAX;

        // Position control inductor 2
             ep2 = Position_c2 - Position2;
             xr2 += (ep2 - fce2);
             sum_vp2 = v2 * Kddot + Position2 * Kd;
             fc2_prim = Kw * Position_c2 + Kr * xr2 * I - sum_vp2 + FP;
             fc2 = fc2_prim;

             if (fc2_prim <= 0){
                fc2 = 0;
             }
             if (fc2_prim >= FMAX){
                fc2 = FMAX;
             }
             fce2 = (fc2_prim - fc2) * K_antiwindup * antiwindup_pos;

          // Position control inductor 2 - PID regulation
              // Erreur + composante P
//              ep2 = (Position_c2 - Position2);
//              kep2 = ep2 * Kp_pid;
//              // Composante I
//              xr2 += Ki_pid * H * (ep2 - (fc2_prim - fc2) * K_antiwindupPID * antiwindup_pos);
//
//              // Composante D
//              dpos2 = v2 * Kd_pid;
//
//              // Assemblage des 3 composantes
//              fc2_prim = (ep2 + xr2 - dpos2);
//
//              fc2 = fc2_prim;
//              if (fc2_prim <= 0){
//                 fc2 = 0;
//              }
//              if (fc2_prim >= FMAX){
//                 fc2 = FMAX;
//              }

             // Inverse fourier transform
             ic1 = (sqrtf(K_FC * fc1f)) * Position1;
             ic2 = (sqrtf(K_FC * fc2f)) * Position2;

         }

         // end takeoff 1 & 2//

         /////////////////// CHANGEMENT POLES PLACEMENT ///////////////////////////////////
         if(i_store >= i_store_change_poles_placement) // sert pour le passage de regulation de 2 � 4 inducteurs (stabilit�)
         {
             Kr = Kr_change;
             Kw = Kw_change;
             Kd = Kd_change;
             Kddot = Kddot_change;
         }

         //////////////////////////INDUCTOR 3 & 4 /////////////////////////////////////

         //////////////////////////////////////////////////////////////////////////////
         //                          Take off strategy 2                             //
         //////////////////////////////////////////////////////////////////////////////

//         if((takeOff == 0) && (takeOff2 == 1) && (i_store >= i_store_2e_decollage)) // phase 2 for inductors 3,4
//         {
//             if(Position3 < Position_c3_dec) // if inductor starts to move
//               {
//                   takeOff2 = 0;
//                   Position2_c3 = Position3;
//               }
//
//           //current limiting at value necessary to take off inductor at Delta_0
//               else if(ic3 < 7.82f)
//               {
//                   ic3 += TAKEOFF_CURRENT_STEP1; //"infinite" current ramp until inductor starts to move
//               }
//               else
//               {
//                   ic3 = 7.82;
//               }
//               ic4 = ic3;
//         }
//         else if(takeOff2 == 0)
//         {
//
//         //SET POINT GENERATOR FOR INDUCTOR 3 & 4 //
//             if(Position2_c3 > Pos3_to_regul)
//             {
//                 Position2_c3 -= 2.5 * 4e-7; // ramp from 3mm to 2mm (4e-7 [s])
//             }
//             else
//             {
//                 Position2_c3 = Pos3_to_regul;
//             }
//
//             if(Position2_c4 > Pos4_to_regul)
//             {
//                 Position2_c4 -= 2.5 * 4e-7;
//             }
//             else
//             {
//                 Position2_c4 = Pos4_to_regul;
//             }
//
//         //////////////////////////////////////////////////////////////////////////////
//         //                STATE METHOD REGULATION inductor 3 & 4                    //
//         //           With embedded system, all integrators must be removed          //
//         //////////////////////////////////////////////////////////////////////////////
//
//             //Fc3 bandstop filter
//             IN3[0] = fc3;
//             fc3f = IIR_Filter(IN3,OUT3);
//             //introducing limit because IIR filter may cause overtaking
//             if (fc3f <= 0)
//                 fc3f = 0;
//             if (fc3f >= FMAX)
//                 fc3f = FMAX;
//
//             // Position control inductor 3
//             ep3 = Position2_c3 - Position3;
//             xr3 += (ep3 - fce3);
//             sum_vp3 = (v3 * Kddot_sans_int) + (Position3 * Kd_sans_int);
//             fc3_prim = (Kw_sans_int * Position2_c3) + (Kr_sans_int * xr3 * I) - sum_vp3 + FP;
//             //fc3_prim = fc3_prim - fperturb1;
//             fc3 = fc3_prim;
//             if (fc3_prim <= 0){
//                 fc3 = 0;
//             }
//             if (fc3_prim >= FMAX){
//                 fc3 = FMAX;
//             }
//             fce3 = (fc3_prim - fc3) * K_antiwindup * antiwindup_pos;
//
//             // Fc bandstop filter
//             IN4[0] = fc4;
//             fc4f = IIR_Filter(IN4,OUT4);
//             //introducing limit because IIR filter may cause overtaking
//             if (fc4f <= 0)
//                 fc4f = 0;
//             if (fc4f >= FMAX)
//                 fc4f = FMAX;
//
//             // Position control inductor 4
//             ep4 = Position2_c4 - Position4;
//             xr4 += (ep4 - fce4);
//             sum_vp4 = (v4 * Kddot) + (Position4 * Kd);
//             fc4_prim = (Kw * Position2_c4) + (Kr * xr4 * I) - sum_vp4 + FP;
//             fc4 = fc4_prim;
//             if (fc4_prim <= 0){
//                 fc4 = 0;
//             }
//             if (fc4_prim >= FMAX){
//                 fc4 = FMAX;
//             }
//             fce4 = (fc4_prim - fc4) * K_antiwindup * antiwindup_pos;
//
//             // Inverse fourier transform
//             ic3 = sqrtf(K_FC * fc3f) * Position3;
//             ic4 = sqrtf(K_FC * fc4f) * Position4;
//         }

//        Modify Kr of the regulator "without integrator" for stay the position
//        at 2 mm slowly after 40ms (not immediatly, cause to second take off "agressivity")
           if(i_store >= (i_store_2e_decollage + 1000))
           {
               Kr_sans_int = -1.0; // valeur initiale -1.0
           }

        //////////////////////////////////////////////////////////////////////////////
        //                          CURRENT REGULATION                              //
        //////////////////////////////////////////////////////////////////////////////

           // Inductor 1
           PI_current_regulator(ic1, Current1, &integral_i1, &ue1, &uc1);

           // Inductor 2
           PI_current_regulator(ic2, Current2, &integral_i2, &ue2, &uc2);

//           // Inductor 3
//           PI_current_regulator(ic3, Current3, &integral_i3, &ue3, &uc3);
//
//           // Inductor 4
//           PI_current_regulator(ic4, Current4, &integral_i4, &ue4, &uc4);



        //////////////////////////////////////////////////////////////////////////////
        //                           END OF REGULATION                              //
        //////////////////////////////////////////////////////////////////////////////

         // Calcul de l'angle theta
    //            theta = ((Position1)-(Position2))*UN_SUR_d;
                // Convert Uc1,...,Uc3 into dutycycle and add DA (dutycycle correction) as ucx will always be >0 in our application, otherwise
                // DA would have been subtracted for ucx <0

        dutyCycle1 = 0.5f + (CONV_DUTY_CYCLE * uc1) + DA;
        dutyCycle2 = 0.5f + (CONV_DUTY_CYCLE * uc2) + DA;
//        dutyCycle3 = 0.5f + (CONV_DUTY_CYCLE * uc3) + DA;
//        dutyCycle4 = 0.5f + (CONV_DUTY_CYCLE * uc4) + DA;
     }

    if(start_ISOZ == 1 || state_PIN == 1)
    {

        GPIO_writePin(LED_D5, 1); // Allumer LED pendant sustentation
        //////////////////////////////////////////////////////////////////////////////
        //                           CALCUL DES PWMs                                //
        //////////////////////////////////////////////////////////////////////////////

    // Travail avec DC en pourcent, de 0 � 100 [%]
        duty1 = dutyCycle1 * 100;
        duty2 = dutyCycle2 * 100;
//        duty3 = dutyCycle3 * 100;
//        duty4 = dutyCycle4 * 100;

    // Limite min et max des rapports cycliques
        if(duty1 >= LIMITE_MAX_DUTY_FINE)
            duty1 = LIMITE_MAX_DUTY_FINE;
        else if(duty1 <= LIMITE_MIN_DUTY_FINE)
            duty1 = LIMITE_MIN_DUTY_FINE;

        if(duty2 >= LIMITE_MAX_DUTY_FINE)
            duty2 = LIMITE_MAX_DUTY_FINE;
        else if(duty2 <= LIMITE_MIN_DUTY_FINE)
            duty2 = LIMITE_MIN_DUTY_FINE;

//        if(duty3 >= LIMITE_MAX_DUTY_FINE)
//            duty3 = LIMITE_MAX_DUTY_FINE;
//        else if(duty3 <= LIMITE_MIN_DUTY_FINE)
//            duty3 = LIMITE_MIN_DUTY_FINE;
//
//        if(duty4 >= LIMITE_MAX_DUTY_FINE)
//            duty4 = LIMITE_MAX_DUTY_FINE;
//        else if(duty4 <= LIMITE_MIN_DUTY_FINE)
//            duty4 = LIMITE_MIN_DUTY_FINE;

         // Affection des compteurs PWM : //
//             Plus propre de faire une boucle et un tableau de duty[4]

        dutyFine = ((float)(duty1*TIME_BASE_PERIOD) * INV_FACTOR);
        count = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
        compCount = (count);
        i = 1;
        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);


        dutyFine = ((float)(duty2*TIME_BASE_PERIOD) * INV_FACTOR);
        count = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
        compCount = (count);
        i = 2;
        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);


//        dutyFine = ((float)(duty3*TIME_BASE_PERIOD) * INV_FACTOR);
//        count = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
//        compCount = (count);
//        i = 3;
//        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
//        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
//
//
//        dutyFine = ((float)(duty4 * TIME_BASE_PERIOD) * INV_FACTOR);
//        count = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
//        compCount = (count);
//        i = 4;
//        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
//        HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);

        if(takeOff == 0){
            i_store++;
        }

        //
        // Call the scale factor optimizer lib function SFO()
        // periodically to track for any change due to temp/voltage.
        // This function generates MEP_ScaleFactor by running the
        // MEP calibration module in the HRPWM logic. This scale
        // factor can be used for all HRPWM channels. The SFO()
        // function also updates the HRMSTEP register with the
        // scale factor value.
        //
        status = SFO(); // in background, MEP calibration module
                        // continuously updates MEP_ScaleFactor
        if (status == SFO_ERROR)
        {
            error();   // SFO function returns 2 if an error occurs & #
                       // of MEP steps/coarse step
        }              // exceeds maximum of 255.

        // Led de debug
        GPIO_writePin(LED_D5, 0); // Led de debug pour mesurer la frequence de l'interruption
    }
    // Arret de la sustentation
    else if(stop_ISOZ == 1 || state_PIN == 0)
    {
        GPIO_writePin(LED_D2,1); // Eteindre la LED2 apres sustentation

        for(i = 1;i<1+NUM_OF_PWM_CHANNEL;i++)
            {
                dutyFine = ((float)(duty_table[i]*TIME_BASE_PERIOD) * INV_FACTOR);
                count = (dutyFine * (float32_t)(EPWM_TIMER_TBPRD << 8)) * INV_FACTOR;
                compCount = (count);

                HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_A, compCount);
                HRPWM_setCounterCompareValue(ePWM[i], HRPWM_COUNTER_COMPARE_B, compCount);
            }

        i_store = 0;

        //
        // Call the scale factor optimizer lib function SFO()
        // periodically to track for any change due to temp/voltage.
        // This function generates MEP_ScaleFactor by running the
        // MEP calibration module in the HRPWM logic. This scale
        // factor can be used for all HRPWM channels. The SFO()
        // function also updates the HRMSTEP register with the
        // scale factor value.
        //
        status = SFO(); // in background, MEP calibration module
                        // continuously updates MEP_ScaleFactor
        if (status == SFO_ERROR)
        {
            error();   // SFO function returns 2 if an error occurs & #
                       // of MEP steps/coarse step
        }              // exceeds maximum of 255.
    }

    //
    // Clear the interrupt flag
    //
    ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);

    //
    // Check if overflow has occurred
    //
    if(true == ADC_getInterruptOverflowStatus(myADC0_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(myADC0_BASE, ADC_INT_NUMBER1);
        ADC_clearInterruptStatus(myADC0_BASE, ADC_INT_NUMBER1);
    }

    //
    // Acknowledge the interrupt
    //
    Interrupt_clearACKGroup(INT_myADC0_1_INTERRUPT_ACK_GROUP);

//************************************//
//          BOUTON EXTINT --->
//************************************//

    if(Ext_Int_Flag == 1) // Detection passage INTERRUPT car flanc montant
    {
        count_ext_int = 0;
        while(count_ext_int < COUNT_TO_REACH)
            count_ext_int++;
        if(GPIO_readPin(Push_Button_Start) == 0)
        {
            // APPUI
            if(start_ISOZ == 1)
            {
                start_ISOZ = 0;
                stop_ISOZ = 1;
            }
            else if(start_ISOZ == 0)
            {
                start_ISOZ = 1;
                stop_ISOZ = 0;
            }
        }
        Ext_Int_Flag = 0;
    }

//************************************//
//      --->  BOUTON EXTINT
//************************************//



}

__interrupt void INT_Push_Button_Start_XINT_ISR(void)
{
    Ext_Int_Flag = 1;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

__interrupt void INT_mySCI0_RX_ISR(void) // .xxx
{
    uint16_t c;

    while (SCI_getRxFIFOStatus(SCIA_BASE) > 0) {
        c = SCI_readCharBlockingFIFO(SCIA_BASE);
        if (rxIndex < RX_BUF_LEN) {
             rxBuffer[rxIndex++] = c;
         }
         if(c == '\x02'){
            // Marque le d�but de trame
            dataIndex = rxIndex; // Sauvegarde la position pour les donn�es
         }
         if(c == '\x03'){
            // Fin de trame - traite les donn�es
            if(dataIndex > 0 && dataIndex < rxIndex-1) {
                state_PIN = (rxBuffer[dataIndex] == '1') ? 1 : 0;
            }
            // Reset du buffer
            rxIndex = 0;
            int b;
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
    while (SCI_getTxFIFOStatus(SCIA_BASE) < SCI_FIFO_TX16 && txIndex < txLength) {
        SCI_writeCharBlockingFIFO(SCIA_BASE, txBuffer[txIndex++]);
    }

    if (txIndex >= txLength) {
        SCI_disableInterrupt(mySCI0_BASE, SCI_INT_TXFF); // Fin d�envoi
    }

    // Acquitter l'interruption
    SCI_clearOverflowStatus(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9); // For the PIE acknowledge
}
