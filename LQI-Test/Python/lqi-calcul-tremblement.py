#!/usr/bin/env python3
# =============================================================================
#  lqi_par_inducteur.py (RETOUR À LA VERSION DE BASE + OBSERVATEUR RAPIDE)
#  Synthese d'un LQI INDEPENDANT PAR INDUCTEUR + OBSERVATEUR D'ÉTAT (Luenberger)
# =============================================================================

import numpy as np
from scipy.linalg import expm
from scipy.signal import place_poles

# =============================== PARAMETRES ==================================
M_CORNER  = 4.513            # masse portee par un inducteur [kg]
H         = 1.0/25e3         # periode ISR [s] (40 us)
NOTCH_LAG = 4.0e-3           # CORRECTION : Le filtre coupe-bande est désactivé

# tau de boucle fermee de courant mesures (Kpi=2), par inducteur [s]
TAU_IBF = {1: 1.5495e-3, 2: 1.6263e-3, 3: 1.5495e-3, 4: 1.5495e-3}
# =============================================================================

def c2d(A, B, h):
    n = A.shape[0]; m = B.shape[1]
    Maug = np.zeros((n + m, n + m)); Maug[:n, :n] = A; Maug[:n, n:] = B
    Md = expm(Maug * h); return Md[:n, :n], Md[:n, n:]

def design_lqi_place(tau_act):
    """
    LQI 1 DDL par placement de pôles. 
    États : [delta, delta_point, F, eps]
    """
    # CORRECTION DU SIGNE : Pousser vers le haut (F>0) fait diminuer l'entrefer (delta)
    A = np.array([[0, 1, 0, 0],
                  [0, 0, -1.0/M_CORNER, 0],  
                  [0, 0, -1.0/tau_act, 0],
                  [-1, 0, 0, 0]])            # eps = intégrale de (Pos_c - Pos)
    B = np.array([[0], [0], [1.0/tau_act], [0]])
    
    Ad, Bd = c2d(A, B, H)
    
    # Placement de pôles vers 80-95 rad/s pour dominer l'instabilité (qui est à 99 rad/s)
    poles_cont = [-80.0, -85.0, -90.0, -8]
    poles_disc = np.exp(np.array(poles_cont) * H)
    
    res = place_poles(Ad, Bd, poles_disc)
    K = res.gain_matrix.flatten()
    return K

def design_observer(tau_act):
    """
    Observateur d'état (Position, Vitesse, Force)
    """
    # CORRECTION DU SIGNE : -1.0/M_CORNER
    A_obs = np.array([[0, 1, 0],
                      [0, 0, -1.0/M_CORNER],
                      [0, 0, -1.0/tau_act]])
    B_obs = np.array([[0], [0], [1.0/tau_act]])
    C_obs = np.array([[1, 0, 0]]) # On mesure uniquement la position (index 0)

    Ad_obs, Bd_obs = c2d(A_obs, B_obs, H)

    # CORRECTION : Pôles très rapides (700 à 820 rad/s)
    poles_cont = [-700.0, -760.0, -820.0]
    poles_disc = np.exp(np.array(poles_cont) * H)

    res = place_poles(Ad_obs.T, C_obs.T, poles_disc)
    L = res.gain_matrix.T  

    return Ad_obs, Bd_obs, L

def main():
    print("="*68)
    print(" GENERATION DES MATRICES : LQI + OBSERVATEUR (SANS NOTCH)")
    print("="*68)
    print(" A COLLER DANS FunctionHeader.h\n")
    
    for i in (1, 2, 3, 4):
        # CORRECTION : Le tau_act est recalculé DANS la boucle pour s'adapter à chaque inducteur
        ta = TAU_IBF[i] + NOTCH_LAG 
        
        K = design_lqi_place(ta)
        Ad_obs, Bd_obs, L = design_observer(ta)
        
        print(f"// ================= INDUCTEUR {i} =================")
        # On prend la valeur absolue car les signes (+/-) sont gérés manuellement dans le code C
        print(f"#define LQI{i}_Q    {abs(K[0]):.1f}f")
        print(f"#define LQI{i}_QD   {abs(K[1]):.2f}f")
        print(f"#define LQI{i}_EPS  {abs(K[3]):.1f}f")
        
        print(f"")
        print(f"#define AD11_{i}  {Ad_obs[0,0]:.6f}f")
        print(f"#define AD12_{i}  {Ad_obs[0,1]:.6f}f")
        print(f"#define AD22_{i}  {Ad_obs[1,1]:.6f}f")
        print(f"#define AD23_{i}  {Ad_obs[1,2]:.6f}f")  # Doit sortir NEGATIF (ex: -0.000009)
        print(f"#define AD33_{i}  {Ad_obs[2,2]:.6f}f")
        print(f"#define BD3_{i}   {Bd_obs[2,0]:.6f}f")
        
        print(f"#define L1_{i}    {L[0,0]:.4f}f")
        print(f"#define L2_{i}    {L[1,0]:.2f}f")
        print(f"#define L3_{i}    {L[2,0]:.2f}f")
        print(f"\n")

if __name__ == "__main__":
    main()