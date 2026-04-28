/*
 * FunctionHeader2.h
 *
 * Created on : 9 mars 2018 (Aymeric)
 * Modified by: Laucella (2019), Yersin (2022), Freyche (2025)
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
#define DELTA_N         ((float)2e-3)
#define DELTA_0         ((float)3.0e-3)
#define L_N             ((float)4.9e-3)
#define I_N             5.218f
#define M               3.4f
#define G               9.81f
#define d               ((float)26e-3)

/* ========================================================================= *
 * HARDWARE CONFIGURATION
 * ========================================================================= */
#define VDC_BUS         48

// --- ADC ---
#define N_BIT_ADC       12
#define N_MAX_ADC       4096.0f
#define N_MAX_ADC_NEW   3722.7f

// --- Position ---
#define POS_MAX         ((float)3.0e-3)
#define POS_MIN         0.0
#define CONV_POS2       ((float)(POS_MAX-POS_MIN)/(N_MAX_ADC_NEW))

// --- Current ---
#define I_MAX           30.0f

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
#define KP_I            1.853095f
#define TI_I            ((float)4.6366198e-4)
#define GI_I            (1.0/TI_I)
#define UMAX            VDC_BUS
#define ANTIWINDUP_EN   1
#define ANTIWINDUP_DIS  0

// --- Inverse transform method ---
#define K_FC            ((float)(2.0/(L_N*DELTA_N)))

// --- State method regulation ---
#define FP              M*G

#define KW_SANS_INT      ((float)-2.8742e4)
#define KDDOT_SANS_INT   -541.5f
#define KD_SANS_INT      ((float)-2.8742e4)
#define KR_SANS_INT      0.0f

#define KW               ((float)-1.2082e4)
#define KDDOT            -458.3f
#define KD               ((float)-3.0413e4)
#define KR               -28.92f

#define KW_CHANGE        ((float)-6.6658e3)
#define KDDOT_CHANGE     -499.8f
#define KD_CHANGE        ((float)-2.9989e4)
#define KR_CHANGE        -18.64f

#define I_STORE_CHANGE_POLES_PLACEMENT  10000
#define I_STORE_2E_DECOLLAGE            15000

#define IMAX            12.0f
#define FMAX            ((float)(0.5*L_N*DELTA_N*(IMAX/DELTA_0)*(IMAX/DELTA_0)))
#define K_ANTIWINDUP    ((float)(1.0/KW))

/* ========================================================================= *
 * FILTER COEFFICIENTS
 * ========================================================================= */
#define A1_B1_0         -1.980143772501098
#define A2_0            0.980339665209542
#define B0_B2_0         0.9901698326047711

// --- Filters configurations ---
#define FILTWINDOW      9
#define Nb              3
#define Na              2

/* ========================================================================= *
 * SHARED GLOBALS & COMMUNICATION
 * ========================================================================= */
#define TX_BUF_LEN 64
extern volatile char txBuffer[TX_BUF_LEN];
extern volatile uint16_t txIndex;
extern volatile uint16_t txLength;

/* ========================================================================= *
 * FUNCTION PROTOTYPES
 * ========================================================================= */
void reverse(char* str, int len);
int ConvertIntToStr(int x, char str[], int p);
int ftoa(float n, char* res, int afterpoint);
void SendFloatAsText(float f0, float f1, float f2, float f3);
float savitzky_Filter(float *Buffer);
float IIR_Filter(float *entree, float *sortie);
void PI_current_regulator(float ic, float current, float *integral, float *ue, float *uc);

#endif /* FUNCTIONHEADER_H_ */
