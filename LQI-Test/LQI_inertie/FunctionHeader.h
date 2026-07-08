/*
 * FunctionHeader.h
 *
 * Created on : 9 mars 2018 (Aymeric)
 * Modified by: Laucella (2019), Yersin (2022), Freyche (2025), Uluï¿½inar (2026)
 *
 * Description: Constants, system parameters, and function prototypes
 * for the magnetic levitation/inductor project.
 */

#ifndef FUNCTIONHEADER_H_
#define FUNCTIONHEADER_H_

#include <stdint.h>
#include <stdbool.h>

/* ========================================================================= *
 * PHYSICAL & SYSTEM PARAMETERS
 * ========================================================================= */
#define DELTA_N         ((float)2.0e-3)           // Position nominal de l'entrefer
#define DELTA_0         ((float)3.0e-3)         // Position max de l'entrefer
#define L_N1            ((float)7.742e-3)       // Inductance nominal au point nominal pour l'inducteur 1 (prise à 120 Hz)
#define L_N2            ((float)8.647e-3)       // Inductance nominal au point nominal pour l'inducteur 2 (prise à 120 Hz)
#define L_N3            ((float)7.699e-3)       // Inductance nominal au point nominal pour l'inducteur 3 (prise à 120 Hz)
#define L_N4            ((float)7.750e-3)       // Inductance nominal au point nominal pour l'inducteur 4 (prise à 120 Hz)
#define I_N             4.48f                   // Courant nominal par mï¿½thode inverse au point nominal
#define M               4.513f                  // Masse pour un seul inducteur
#define G               9.81f                   // Gravity
#define Distance        ((float)26e-3)          // Distance between inductor 1 and 2

/* ========================================================================= *
 * HARDWARE CONFIGURATION
 * ========================================================================= */
#define VDC_BUS         48

// --- ADC ---
#define N_BIT_ADC       12
#define N_MAX_ADC       4095.0f

// --- Position ---
#define POS_MAX         ((float)3.0e-3)
#define POS_MIN         0.0
#define CONV_POS2       ((float)(POS_MAX-POS_MIN)/(N_MAX_ADC))

// --- Current ---
#define I_MAX           30.0f
#define I_MIN           -30.0f

// --- PWM ---
#define F_PWM           ((float)25e3)
#define H               ((float)(1.0/F_PWM))
#define COUNT_PWM       ((float)(150e6/(2*F_PWM)))
#define DEADTIME        150.0f
#define T_EN            55.0f
#define T_DE            72.0f
#define DA              ((float)((DEADTIME + T_EN - T_DE)*1e-9*F_PWM))
#define CONV_DUTY_CYCLE ((float)(1.0/(2*VDC_BUS)))

/* ========================================================================= *
 * CONTROL & REGULATION
 * ========================================================================= */
// --- Current control ---
#define I_SP            3.0f
#define I_SP105         3.15f
#define I_SP095         2.85f
#define KP_I            2.0f // Avec Kui ï¿½ -45ï¿½ : Kpi = 90.964f et sans kui : Kpi = 1.88
#define TI_I            ((float)3.245633840657308e-4)
#define GI_I            (1.0/TI_I)
#define UMAX            VDC_BUS
#define ANTIWINDUP_EN   1
#define ANTIWINDUP_DIS  0

// --- Inverse transform method ---
#define K_FC1            ((float)(2.0/(L_N1*DELTA_N)))
#define K_FC2            ((float)(2.0/(L_N2*DELTA_N)))
#define K_FC3            ((float)(2.0/(L_N3*DELTA_N)))
#define K_FC4            ((float)(2.0/(L_N4*DELTA_N)))

#define FP              M*G

// --- State method regulation for inductor ---
//#define KW_SANS_INT      ((float)-3.6386e4)
//#define KDDOT_SANS_INT   -701.9f
//#define KD_SANS_INT      ((float)-3.6386e4)
//#define KR_SANS_INT      0.0f
//
//#define KW               ((float)-1.4862e4)
//#define KDDOT            -603.41f
//#define KD               ((float)-3.8998e4)
//#define KR               -35.63f
//
//#define KW_CHANGE        ((float)-8.8418e3)
//#define KDDOT_CHANGE     -663.35f
//#define KD_CHANGE        ((float)-3.9798e4)
//#define KR_CHANGE        -24.72f

#define I_STORE_CHANGE_POLES_PLACEMENT  10000
#define I_STORE_2E_DECOLLAGE            15000

#define IMAX            12.0f
#define FMAX1           ((float)(0.5*L_N1*DELTA_N*(IMAX/DELTA_0)*(IMAX/DELTA_0)))
#define FMAX2           ((float)(0.5*L_N2*DELTA_N*(IMAX/DELTA_0)*(IMAX/DELTA_0)))
#define FMAX3           ((float)(0.5*L_N3*DELTA_N*(IMAX/DELTA_0)*(IMAX/DELTA_0)))
#define FMAX4           ((float)(0.5*L_N4*DELTA_N*(IMAX/DELTA_0)*(IMAX/DELTA_0)))
#define K_ANTIWINDUP    ((float)(1.0/KW))

/* ========================================================================= *
 * IIR FILTER COEFFICIENTS
 * ========================================================================= */
//Define of the coefficients filters bandstop (10 differents)
//bandstop Filter 56Hz 2nd order bande coupï¿½e de 47Hz ï¿½ 90Hz (0) (test)
#define A1_B1_0         -1.980143772501098
#define A2_0            0.980339665209542
#define B0_B2_0         0.9901698326047711
//bandstop Filter 65Hz 2nd order bande coupï¿½e de 47Hz ï¿½ 90Hz (1)
#define A1_B1_1         -1.98898514926503589
#define A2_1            0.9892508998390186
#define B0_B2_1         0.9946254499195093
//bandstop Filter 65Hz 2nd order bande coupï¿½e de 34Hz ï¿½ 119Hz (2)
#define A1_B1_2         -1.97860927758394145
#define A2_2            0.97886214822594597
#define B0_B2_2         0.989431074112973
//bandstop Filter 70Hz 2nd order bande coupï¿½e de 50Hz ï¿½ 100Hz (3)
#define A1_B1_3         -1.98719807873359366468
#define A2_3            0.98751192990729475
#define B0_B2_3         0.9937559649536473749265
//bandstop Filter 70Hz 2nd order bande coupï¿½e de 37Hz ï¿½ 128Hz (4)
#define A1_B1_4         -1.9770910357912317
#define A2_4            0.97738681058138155
#define B0_B2_4         0.98869340529069083
//bandstop Filter 75Hz 2nd order bande coupï¿½e de 57Hz ï¿½ 95Hz (5)
#define A1_B1_5         -1.9896496113551767
#define A2_5            0.98999709932613533
#define B0_B2_5         0.99499854966306767
//bandstop Filter 75Hz 2nd order bande coupï¿½e de 40Hz ï¿½ 130Hz (6)
#define A1_B1_6         -1.977307757140243538
#define A2_6            0.97763254912161579
#define B0_B2_6         0.98881627456080789517756
//bandstop Filter 80Hz 2nd order bande coupï¿½e de 60Hz ï¿½ 108Hz (7)
#define A1_B1_7         -1.987853700192099483
#define A2_7            0.9882568325036572565
#define B0_B2_7         0.99412841625182868377
//bandstop Filter 80Hz 2nd order bande coupï¿½e de 46Hz ï¿½ 136Hz (8)
#define A1_B1_8         -1.9772418015560622
#define A2_8            0.97763254912162
#define B0_B2_8         0.98881627456081
//bandstop Filter 85Hz 2nd order bande coupï¿½e de 65Hz ï¿½ 111Hz (9)
#define A1_B1_9         -1.988052149327327544
#define A2_9            0.9885052567179366534
#define B0_B2_9         0.9942526283589683267
//bandstop Filter 85Hz 2nd order bande coupï¿½e de 51Hz ï¿½ 142Hz (10)
#define A1_B1_10        -1.988052149327327544
#define A2_10           0.9885052567179366534
#define B0_B2_10        0.9942526283589683267

// --- Filters configurations ---
#define FILTWINDOW      9
#define Nb              3
#define Na              2

/* ========================================================================= *
 * SHARED GLOBALS & COMMUNICATION
 * ========================================================================= */
#define TX_BUF_LEN 512
extern volatile char txBuffer[TX_BUF_LEN];
extern volatile uint16_t txIndex;
extern volatile uint16_t txLength;

extern const float POS_COR_1[6];
extern const float POS_COR_2[6];
extern const float POS_COR_3[6];
extern const float POS_COR_4[6];

// ================= INDUCTEUR 1 =================
#define LQI1_Q    63377.6f
#define LQI1_QD   1090.23f
#define LQI1_EPS  1380109.7f

#define AD11_1  1.000000f
#define AD12_1  0.000040f
#define AD22_1  1.000000f
#define AD23_1  -0.000009f
#define AD33_1  0.992448f
#define BD3_1   0.007552f
#define L1_1    0.0823f
#define L2_1    51.43f
#define L3_1    -31551.06f


// ================= INDUCTEUR 2 =================
#define LQI2_Q    64395.9f
#define LQI2_QD   1107.75f
#define LQI2_EPS  1402283.1f

#define AD11_2  1.000000f
#define AD12_2  0.000040f
#define AD22_2  1.000000f
#define AD23_2  -0.000009f
#define AD33_2  0.992568f
#define BD3_2   0.007432f
#define L1_2    0.0824f
#define L2_2    51.65f
#define L3_2    -32058.53f

// ======== MIMO — modes [Z, T, R] ========
static const float T_MAT[3][4] = {{2.50000000e-01f, 2.50000000e-01f, 2.50000000e-01f, 2.50000000e-01f},
                                  {1.78571429e+00f, 1.78571429e+00f, -1.78571429e+00f, -1.78571429e+00f},
                                  {1.92307692e+00f, -1.92307692e+00f, 1.92307692e+00f, -1.92307692e+00f}};
static const float E_MAT[3][4] = {{1.00000000e+00f, 1.00000000e+00f, 1.00000000e+00f, 1.00000000e+00f},
                                  {1.40000000e-01f, 1.40000000e-01f, -1.40000000e-01f, -1.40000000e-01f},
                                  {1.30000000e-01f, -1.30000000e-01f, 1.30000000e-01f, -1.30000000e-01f}};
static const float W_MAT[4][3] = {{2.50000000e-01f, 1.78571429e+00f, 1.92307692e+00f},
                                  {2.50000000e-01f, 1.78571429e+00f, -1.92307692e+00f},
                                  {2.50000000e-01f, -1.78571429e+00f, 1.92307692e+00f},
                                  {2.50000000e-01f, -1.78571429e+00f, -1.92307692e+00f}};
static const float F_STAT[4]   = {3.84978522e+01f, 3.84978522e+01f, 3.84978522e+01f, 3.84978522e+01f};
static const float U_STAT[3]   = {1.53991409e+02f, 0.00000000e+00f, 0.00000000e+00f};

static const float LQI_Q[3]       = {1.09992139e+05f, 5.16656124e+03f, 5.16656124e+03f};
static const float LQI_QD[3]      = {2.55616924e+03f, 1.20068634e+02f, 1.20068634e+02f};
static const float LQI_EPS[3]     = {1.60037075e+06f, 7.51727636e+04f, 7.51727636e+04f};
static const float LQI_EPS_INV[3] = {6.24855211e-07f, 1.33026904e-05f, 1.33026904e-05f};
static const float AW_TOL[3]      = {3.07982817e+00f, 4.31175944e-01f, 4.00377663e-01f};

static const float OBS_AD12[3] = {4.00000000e-05f, 4.00000000e-05f, 4.00000000e-05f};
static const float OBS_AD23[3] = {-2.53905745e-06f, -5.40545797e-05f, -5.40545797e-05f};
static const float OBS_AD33[3] = {9.92837497e-01f, 9.92837497e-01f, 9.92837497e-01f};
static const float OBS_BD3[3]  = {7.16250336e-03f, 7.16250336e-03f, 7.16250336e-03f};
static const float OBS_L1[3]   = {8.26596051e-02f, 8.26596100e-02f, 8.26596100e-02f};
static const float OBS_L2[3]   = {5.21497898e+01f, 5.21497962e+01f, 5.21497962e+01f};
static const float OBS_L3[3]   = {-1.15564501e+05f, -5.42830916e+03f, -5.42830916e+03f};

#define REF_SMOOTH 1.333333e-04f

/* ========================================================================= *
 * FUNCTION PROTOTYPES
 * ========================================================================= */
void reverse(char* str, int len);
int ConvertIntToStr(int x, char str[], int p);
int ftoa(float n, char* res, int afterpoint);
void SendFloatAsText(float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7);
float savitzky_Filter(float *Buffer);
float IIR_Filter(float *entree, float *sortie);
void PI_current_regulator(float ic, float current, float *integral, float *ue, float *uc);
float apply_poly5(float x, const float* coeffs);
void Send32FloatsAsCSV(float v[32]);
#endif /* FUNCTIONHEADER_H_ */
