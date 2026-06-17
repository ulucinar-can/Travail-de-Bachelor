/*
 * FunctionHeader2.c
 *
 * Description: Implementation of control, filtering, and utility functions
 */

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "FunctionHeader2.h"

/* ========================================================================= *
 * FILTER COEFFICIENTS
 * ========================================================================= */
// savitzky filter coefficients, order = 2, window = 9
const float SAVITZKY[FILTWINDOW] = {-0.0667, -0.05, -0.0333, -0.0167, 0.0, 0.0167, 0.0333, 0.05, 0.0667};

// bandstop Filter (initialise to the 75Hz large bandstop filter design)
float a[Na] = {A1_B1_6, A2_6};
float b[Nb] = {B0_B2_6, A1_B1_6, B0_B2_6};

/* ========================================================================= *
 * FILTER COEFFICIENTS
 * ========================================================================= */
const float POS_COR_1[6] = {
    -1.6985e-4f,    // a0
    1.4975e-6f,     // a1
    -1.7114e-10f,    // a2
    0,              // a3
    0,              // a4
    0               // a5
};

const float POS_COR_2[6] = {
    -1.0884e-4f,    // a0
     1.4901e-6f,    // a1
    -1.7409e-10f,    // a2
     0,   // a3
     0,   // a4
     0    // a5
};

const float POS_COR_3[6] = {
    -1.1760e-4f,    // a0
     1.4626e-6f,    // a1
    -1.8421e-10f,    // a2
     0,   // a3
     0,   // a4
     0    // a5
};

const float POS_COR_4[6] = {
    -1.0360e-4f,    // a0
     1.4620e-6f,    // a1
    -1.8211e-10f,    // a2
     0,   // a3
     0,   // a4
     0    // a5
};

/* ========================================================================= *
 * CONTROL & REGULATION
 * ========================================================================= */
void PI_current_regulator(float ic, float current, float *integral, float *ue, float *uc)
{
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

float savitzky_Filter(float *Buffer)
{
    float v = 0.0f;
    uint16_t m = 0;

    // savitzky algorithm
    for(m = 0; m < FILTWINDOW; m++)
        v += F_PWM*Buffer[m]*SAVITZKY[m];

    // update buffer for next position
    for(m = 0; m < FILTWINDOW-1; m++)
       Buffer[m] = Buffer[(m+1)];

    return v;
}

float IIR_Filter(float *entree, float *sortie)
{
    float somme = 0.0;
    float somme2 = 0.0;
    uint16_t k = 0;

    // difference equation
    for(k = 0; k < Nb; k++) // Num.
        somme += b[k]*entree[k];
    for(k = 0; k < Na; k++) // Den.
        somme2 += a[k]*sortie[k];

    // update buffers
    for(k = (Na-1); k > 0; k--)
        sortie[k] = sortie[(k-1)];
    sortie[0] = (somme-somme2);

    for(k = (Nb-1); k > 0; k--)
        entree[k] = entree[(k-1)];

    return (somme-somme2);
}

/* ========================================================================= *
 * COMMUNICATION & STRING UTILS
 * ========================================================================= */
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

//void SendFloatAsText(float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7)
//{
//    char str[TX_BUF_LEN];
//    int i = 0, j = 0;
//
//    str[i++] = '\x02'; // start bit
//
//    i += ftoa(f0, &str[i], 3); str[i++] = ',';
//    i += ftoa(f1, &str[i], 3); str[i++] = ',';
//    i += ftoa(f2, &str[i], 3); str[i++] = ',';
//    i += ftoa(f3, &str[i], 3); str[i++] = ',';
//    i += ftoa(f4, &str[i], 3); str[i++] = ',';
//    i += ftoa(f5, &str[i], 3); str[i++] = ',';
//    i += ftoa(f6, &str[i], 3); str[i++] = ',';
//    i += ftoa(f7, &str[i], 3);
//
//    str[i++] = '\x03'; // stop bit
//
//    for (j = 0; j < i && j < TX_BUF_LEN; j++) {
//        txBuffer[j] = (uint8_t)str[j];
//    }
//    txLength = i;
//    txIndex = 0;
//
//    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXFF);
//}

float apply_poly5(float x, const float* coeffs)
{
    // Calcul hyper rapide (1 cycle par multiplication/addition)
    float result = ((((coeffs[5] * x + coeffs[4]) * x + coeffs[3]) * x + coeffs[2]) * x + coeffs[1]) * x + coeffs[0];

    // Sécurité : empêche l'entrefer de devenir mathématiquement négatif
    if (result < 0.0f) {
        result = 0.0f;
    }

    return result;
}

void SendFloatAsText(float f0, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9)
{
    char str[TX_BUF_LEN];
    int i = 0, j = 0;

    str[i++] = '\x02'; // start bit
    i += ftoa(f0, &str[i], 3); str[i++] = ','; // Pos Consigne
    i += ftoa(f1, &str[i], 3); str[i++] = ','; // Pos Mesure
    i += ftoa(f2, &str[i], 3); str[i++] = ','; // Cur Consigne
    i += ftoa(f3, &str[i], 3); str[i++] = ','; // Cur Mesure
    i += ftoa(f4, &str[i], 3); str[i++] = ','; // Force Consigne
    i += ftoa(f5, &str[i], 3); str[i++] = ','; // Xr1
    i += ftoa(f6, &str[i], 3); str[i++] = ','; // Tension Consigne
    i += ftoa(f7, &str[i], 3); str[i++] = ','; // Integrale
    i += ftoa(f8, &str[i], 0); str[i++] = ','; // State (sans virgule)
    i += ftoa(f9, &str[i], 3);                 // Vitesse
    str[i++] = '\x03'; // stop bit

    for (j = 0; j < i && j < TX_BUF_LEN; j++) {
        txBuffer[j] = (uint8_t)str[j];
    }
    txLength = i;
    txIndex = 0;
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_TXFF);
}
