/*
 * FunctionHeader.h
 *
 *  Created on: 9 mars 2018
 *      Author: Aymeric
 *
 * Modified by: Laucella, 2019
 *              Yersin, 2022;
 *              Freyche, 2025;
 */

#ifndef FUNCTIONHEADER_H_
#define FUNCTIONHEADER_H_

// Define of the project

// System parameters
#define DELTA_N         ((float)2e-3)                                  // nominal air-gap
#define DELTA_0         ((float)3.0e-3)                                  // initial air-gap
#define L_N             ((float)4.9e-3)                                  // nominal inductance
#define I_N             5.218f                                  // nominal current
#define M               1.0f                                     // mass on each inductor (value p. default)
//#define M               3.4f                                     // mass for 1 inductor
#define G               9.81f                                    // gravitational constant
#define d               ((float)26e-3)                                   // Distance between inductor 1 and 2
//#define UN_SUR_d         38.461538f                               // 1/d, for win calculation time

// H-Bridge
//#define UNIDIRECTIONAL  0                                       // unidirectional H-Bridge mode
//#define BIDIRECTIONAL   1                                       // bidirectional H-Bridge mode
#define VDC_BUS         48                                      // 48V power supply on the DC BUS

// ADC
#define N_BIT_ADC       12                                         // 12 Bit ADC
#define N_MAX_ADC       4096.0f                                    // 2^12 = 4096
#define N_MAX_ADC_NEW   3722.7f                                     // 2^12 = 4096 -> 3V =  3722.7
//#define OFFSET_ADC_12B  ((float)(V_I0/V_I_ADC_MAX)*(N_MAX_ADC-1))  // Offset due to new ADC ref (TMDSCNCD280039C) 0..3,3V ; old one was 0..3V (TFE)
//#define OFFSET_HW       62                                         // Offset measured at 50mV -> 62 (12b) TFE

// Position
#define POS_MAX         ((float)3.0e-3)                                  // max measurable distance = 3mm
#define POS_MIN         0.0                                     // min measurable distance = 0mm
//#define V_POS_MAX       3                                       // max voltage from sensor at a distance of 3mm = 10V rescaled for ADC at 3V
//#define V_POS_MIN       0                                       // max voltage from sensor at a distance of 3mm = 0V
//#define CONV_POS        ((float)(POS_MAX-POS_MIN)/(N_MAX_ADC-1))       // position = Adcresultn * CONV_POS
#define CONV_POS2       ((float)(POS_MAX-POS_MIN)/(N_MAX_ADC_NEW))       // position = Adcresultn * CONV_POS

// Current
#define I_MAX           30.0f                                    // max measurable current = 30A
//#define I_MIN           -30.0f                                   // min measurable current = -30A
//#define V_I_MAX         3                                       // max voltage from sensor at a current of 30A = 2.5+(3*0.625) = 4.365 V rescaled for ADC at 3V
//#define V_I_ADC_MAX     3.3f                                    // Maximum scale for the adc (ref = 0..3.3V)
//#define V_I0            1.5f                                    // voltage from sensor at a current of 0A = 2.5 rescaled for ADC at 3/2 = 1.5V
//#define V_I_MIN         0                                       // min voltage from sensor at a current of -30A = 2.5-(3*0.625) = 0.625 V rescaled for ADC at 0V
//#define CONV_I          ((float)((I_MAX-I_MIN)/(N_MAX_ADC-1)))  // current = Adcresultn * CONV_I + I_MIN

// PWM
#define F_PWM           ((float)25e3)                                    // PWM frequency in Hz
#define H               ((float)(1.0/F_PWM))                             // PWM period
#define COUNT_PWM       ((float)(150e6/(2*F_PWM)))                       // Counter value for a symmetrical counter
#define DEADTIME        150.0f                                     // Deadtime in ns
#define T_EN            55.0f                                      // MOSFET switch on delay time + rise time (datasheet) in ns
#define T_DE            72.0f                                      // MOSFET switch off delay time + fall time (datasheet) in ns
#define DA              ((float)((DEADTIME + T_EN - T_DE)*1e-9*F_PWM))   // Dutycycle correction due to the deatime implementation
#define CONV_DUTY_CYCLE ((float)(1.0/(2*VDC_BUS)))                         // DutyCycle = 0.5 + Uc/2*VDC_BUS + Da

// Current control
#define I_SP            3.0f                                    // Current set point only for the current take off strategy, otherwise it will be the inverse method output
#define I_SP105         3.15f
#define I_SP095         2.85f
#define KP_I            1.853095f
#define TI_I            ((float)4.6366198e-4)
#define GI_I            (1.0/TI_I)
#define UMAX            VDC_BUS                                 // Maximum output voltage from the PWM
//#define UMIN            -VDC_BUS                                // Minimum output voltage from the PWM
#define ANTIWINDUP_EN   1
#define ANTIWINDUP_DIS  0

// Inverse transform method
#define K_FC            ((float)(2.0/(L_N*DELTA_N)))

// State method regulation
#define FP              M*G                                     // perturbation force

//Reg without integrator
#define KW_SANS_INT      ((float)-2.8742e4)
#define KDDOT_SANS_INT   -541.5f
#define KD_SANS_INT      ((float)-2.8742e4)
#define KR_SANS_INT      0.0f

//Reg with integrator (value M. Laucella)
/*#define KW               -1.0490e4
#define KDDOT            -417.5256
#define KD               -2.5636e4
#define KR               -25.1468*/

//Reg with integrator (value M. Yersin, poles -50)
/*#define KW               -1.8278e4
#define KDDOT            -431.3005
#define KD               -3.2655e4
#define KR               -36.5202*/

//Reg with integrator (value M. Yersin, poles -60) avant premier decollage
#define KW               ((float)-1.2082e4)
#define KDDOT            -458.3f
//#define KD               ((float)-3.2413e4)
#define KD               ((float)-3.0413e4)
#define KR               -28.92f

//Reg with integrator (value M. Yersin, poles -70) apres premier decollage
#define KW_CHANGE        ((float)-6.6658e3)
#define KDDOT_CHANGE     -499.8f
#define KD_CHANGE        ((float)-2.9989e4)
#define KR_CHANGE        -18.64f

//Define of the coefficients filters bandstop (10 differents)
//bandstop Filter 56Hz 2nd order bande coupée de 47Hz à 90Hz (0) (test)
#define A1_B1_0         -1.980143772501098
#define A2_0            0.980339665209542
#define B0_B2_0         0.9901698326047711
//bandstop Filter 65Hz 2nd order bande coupée de 47Hz à 90Hz (1)
#define A1_B1_1         -1.98898514926503589
#define A2_1            0.9892508998390186
#define B0_B2_1         0.9946254499195093
//bandstop Filter 65Hz 2nd order bande coupée de 34Hz à 119Hz (2)
#define A1_B1_2         -1.97860927758394145
#define A2_2            0.97886214822594597
#define B0_B2_2         0.989431074112973
//bandstop Filter 70Hz 2nd order bande coupée de 50Hz à 100Hz (3)
#define A1_B1_3         -1.98719807873359366468
#define A2_3            0.98751192990729475
#define B0_B2_3         0.9937559649536473749265
//bandstop Filter 70Hz 2nd order bande coupée de 37Hz à 128Hz (4)
#define A1_B1_4         -1.9770910357912317
#define A2_4            0.97738681058138155
#define B0_B2_4         0.98869340529069083
//bandstop Filter 75Hz 2nd order bande coupée de 57Hz à 95Hz (5)
#define A1_B1_5         -1.9896496113551767
#define A2_5            0.98999709932613533
#define B0_B2_5         0.99499854966306767
//bandstop Filter 75Hz 2nd order bande coupée de 40Hz à 130Hz (6)
#define A1_B1_6         -1.977307757140243538
#define A2_6            0.97763254912161579
#define B0_B2_6         0.98881627456080789517756
//bandstop Filter 80Hz 2nd order bande coupée de 60Hz à 108Hz (7)
#define A1_B1_7         -1.987853700192099483
#define A2_7            0.9882568325036572565
#define B0_B2_7         0.99412841625182868377
//bandstop Filter 80Hz 2nd order bande coupée de 46Hz à 136Hz (8)
#define A1_B1_8         -1.9772418015560622
#define A2_8            0.97763254912162
#define B0_B2_8         0.98881627456081
//bandstop Filter 85Hz 2nd order bande coupée de 65Hz à 111Hz (9)
#define A1_B1_9         -1.988052149327327544
#define A2_9            0.9885052567179366534
#define B0_B2_9         0.9942526283589683267
//bandstop Filter 85Hz 2nd order bande coupée de 51Hz à 142Hz (10)
#define A1_B1_10        -1.988052149327327544
#define A2_10           0.9885052567179366534
#define B0_B2_10        0.9942526283589683267


// time "indic" to change poles placement and second take off
#define I_STORE_CHANGE_POLES_PLACEMENT  10000 // 10'000 de base = 400ms
#define I_STORE_2E_DECOLLAGE            15000 // 15'000 de base = 600ms

#define IMAX            12.0f // Valeur initiale 10.0f
#define FMAX            ((float)(0.5*L_N*DELTA_N*(IMAX/DELTA_0)*(IMAX/DELTA_0)))
#define K_ANTIWINDUP    ((float)(1.0/KW))


//savitzky(FIR)
#define FILTWINDOW      9
//IIR
#define Nb              3
#define Na              2

// Pragma code ----------------------------------------------------------------------------

// Prototype statements for interruption
__interrupt void adcA1ISR(void);

// Prototype statements for functions
void initSCI(void);
void reverse(char* str, int len);
int ConvertIntToStr(int x, char str[], int p);
int ftoa(float n, char* res, int afterpoint);
void SendFloatAsText(float f0, float f1, float f2, float f3);
float savitzky_Filter(float *Buffer);
float IIR_Filter(float *entree, float *sortie);
void PI_current_regulator(float ic, float current, float *integral, float *ue, float *uc);

#endif /* FUNCTIONHEADER_H_ */
