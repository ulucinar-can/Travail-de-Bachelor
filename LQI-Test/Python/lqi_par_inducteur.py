# #!/usr/bin/env python3
# # =============================================================================
# #  lqi_par_inducteur.py (CORRIGÉ)
# #  Synthese d'un LQI INDEPENDANT PAR INDUCTEUR + OBSERVATEUR D'ÉTAT (Luenberger)
# # =============================================================================

# import numpy as np
# from scipy.linalg import expm
# from scipy.signal import place_poles

# # =============================== PARAMETRES ==================================
# M_CORNER  = 4.513            # masse portee par un inducteur [kg]
# H         = 1.0/25e3         # periode ISR [s] (40 us)
# NOTCH_LAG = 4.0e-3              # CORRECTION : Le filtre coupe-bande est désactivé

# # tau de boucle fermee de courant mesures (Kpi=2), par inducteur [s]
# TAU_IBF = {1: 1.5495e-3, 2: 1.6263e-3, 3: 1.5495e-3, 4: 1.5495e-3}
# # =============================================================================

# def c2d(A, B, h):
#     n = A.shape[0]; m = B.shape[1]
#     Maug = np.zeros((n + m, n + m)); Maug[:n, :n] = A; Maug[:n, n:] = B
#     Md = expm(Maug * h); return Md[:n, :n], Md[:n, n:]

# def design_lqi_place(tau_act):
#     """
#     LQI 1 DDL par placement de pôles. 
#     États : [delta, delta_point, F, eps]
#     """
#     # CORRECTION DU SIGNE : Pousser vers le haut (F>0) fait diminuer l'entrefer (delta)
#     A = np.array([[0, 1, 0, 0],
#                   [0, 0, -1.0/M_CORNER, 0],  
#                   [0, 0, -1.0/tau_act, 0],
#                   [-1, 0, 0, 0]])            # eps = intégrale de (Pos_c - Pos)
#     B = np.array([[0], [0], [1.0/tau_act], [0]])
    
#     Ad, Bd = c2d(A, B, H)
    
#     # Placement de pôles vers 80-95 rad/s pour dominer l'instabilité (qui est à 99 rad/s)
#     poles_cont = [-80.0, -85.0, -90.0, -95.0]
#     poles_disc = np.exp(np.array(poles_cont) * H)
    
#     res = place_poles(Ad, Bd, poles_disc)
#     K = res.gain_matrix.flatten()
#     return K

# def design_observer(tau_act):
#     """
#     Observateur d'état (Position, Vitesse, Force)
#     """
#     # CORRECTION DU SIGNE : -1.0/M_CORNER
#     A_obs = np.array([[0, 1, 0],
#                       [0, 0, -1.0/M_CORNER],
#                       [0, 0, -1.0/tau_act]])
#     B_obs = np.array([[0], [0], [1.0/tau_act]])
#     C_obs = np.array([[1, 0, 0]]) # On mesure uniquement la position (index 0)

#     Ad_obs, Bd_obs = c2d(A_obs, B_obs, H)

#     # CORRECTION : Pôles très rapides (350 à 410 rad/s)
#     #poles_cont = [-350.0, -380.0, -410.0]
#     poles_cont = [-700.0, -760.0, -820.0]
#     poles_disc = np.exp(np.array(poles_cont) * H)

#     res = place_poles(Ad_obs.T, C_obs.T, poles_disc)
#     L = res.gain_matrix.T  

#     return Ad_obs, Bd_obs, L

# def main():
#     print("="*68)
#     print(" GENERATION DES MATRICES : LQI + OBSERVATEUR (SANS NOTCH)")
#     print("="*68)
#     print(" A COLLER DANS FunctionHeader.h\n")
    
#     for i in (1, 2, 3, 4):
#         # CORRECTION : Le tau_act est recalculé DANS la boucle pour s'adapter à chaque inducteur
#         ta = TAU_IBF[i] + NOTCH_LAG 
        
#         K = design_lqi_place(ta)
#         Ad_obs, Bd_obs, L = design_observer(ta)
        
#         print(f"// ================= INDUCTEUR {i} =================")
#         # On prend la valeur absolue car les signes (+/-) sont gérés manuellement dans le code C
#         print(f"#define LQI{i}_Q    {abs(K[0]):.1f}f")
#         print(f"#define LQI{i}_QD   {abs(K[1]):.2f}f")
#         print(f"#define LQI{i}_EPS  {abs(K[3]):.1f}f")
        
#         print(f"")
#         print(f"#define AD11_{i}  {Ad_obs[0,0]:.6f}f")
#         print(f"#define AD12_{i}  {Ad_obs[0,1]:.6f}f")
#         print(f"#define AD22_{i}  {Ad_obs[1,1]:.6f}f")
#         print(f"#define AD23_{i}  {Ad_obs[1,2]:.6f}f")  # Doit sortir NEGATIF (ex: -0.000009)
#         print(f"#define AD33_{i}  {Ad_obs[2,2]:.6f}f")
#         print(f"#define BD3_{i}   {Bd_obs[2,0]:.6f}f")
        
#         print(f"#define L1_{i}    {L[0,0]:.4f}f")
#         print(f"#define L2_{i}    {L[1,0]:.2f}f")
#         print(f"#define L3_{i}    {L[2,0]:.2f}f")
#         print(f"\n")

# if __name__ == "__main__":
#     main()

#!/usr/bin/env python3
# =============================================================================
#  lqi_par_inducteur.py (CORRIGÉ ET SÉPARÉ)
#  Synthese d'un LQI INDEPENDANT PAR INDUCTEUR + OBSERVATEUR D'ÉTAT (Luenberger)
# =============================================================================

# import numpy as np
# from scipy.linalg import expm
# from scipy.signal import place_poles

# # =============================== PARAMETRES ==================================
# M_CORNER  = 4.513            # masse portee par un inducteur [kg]
# H         = 1.0/25e3         # periode ISR [s] (40 us)
# NOTCH_LAG = 4.0e-3           # Retard effectif du filtre coupe-bande

# # tau de boucle fermee de courant mesures (Kpi=2), par inducteur [s]
# TAU_IBF = {1: 1.5495e-3, 2: 1.6263e-3, 3: 1.5495e-3, 4: 1.5495e-3}
# # =============================================================================

# def c2d(A, B, h):
#     n = A.shape[0]; m = B.shape[1]
#     Maug = np.zeros((n + m, n + m)); Maug[:n, :n] = A; Maug[:n, n:] = B
#     Md = expm(Maug * h); return Md[:n, :n], Md[:n, n:]

# def design_lqi_place(tau_act, wd=50.0):
#     """LQI par placement sur 3 etats [delta, delta_point, eps] (KF non requis),
#        puis VERIFICATION sur le modele 4 etats avec le retard actionneur.
#        wd = pulsation dominante visee [rad/s]. 50 rad/s ~ les gains eprouves."""
#     A3 = np.array([[0, 1, 0], [0, 0, 0], [-1., 0, 0]])
#     B3 = np.array([[0], [1.0/M_CORNER], [0.]])
#     Ad, Bd = c2d(A3, B3, H)
#     pc = [-wd+0.6j*wd, -wd-0.6j*wd, -0.8*wd]          # paire amortie + integrateur
#     K = place_poles(Ad, Bd, np.exp(np.array(pc)*H)).gain_matrix.flatten()
#     # --- verification avec le retard actionneur (notch inclus) ---
#     A4 = np.array([[0,1,0,0],[0,0,1.0/M_CORNER,0],[0,0,-1.0/tau_act,0],[-1.,0,0,0]])
#     B4 = np.array([[0],[0],[1.0/tau_act],[0.]])
#     Ad4, Bd4 = c2d(A4, B4, H)
#     K4 = np.array([[K[0], K[1], 0.0, K[2]]])
#     stable = np.all(np.abs(np.linalg.eigvals(Ad4 - Bd4@K4)) < 1.0)
#     assert stable, f"INSTABLE avec le retard {tau_act*1e3:.1f}ms : baisser wd !"
#     return K   # [Q, QD, EPS] -> memes conventions de signes qu'avant

# # ====================== OBSERVATEUR MODAL ROULIS ======================
# JXX      = 0.406      # kg.m2 - MESURE en vol (roulis 8.9Hz avec Kq=18780)
# Y_HALF   = 0.13       # demi-entraxe gauche-droite [m]
# ALLOC_MX = 1.0/(4*Y_HALF)
# ZETA_ROLL = 0.5

# def design_roll_observer(Kq, tau_act):
#     """Observateur [theta_x, theta_x_point, Mx] + gain d'amortissement modal.
#        Kq = gain de position par coin REELLEMENT flashe (la raideur modale en depend).
#        tau_act = tau_iBF seul si l'observateur lit le couple POST-notch."""
#     A = np.array([[0, 1, 0], [0, 0, -1.0/JXX], [0, 0, -1.0/tau_act]])
#     B = np.array([[0], [0], [1.0/tau_act]])
#     C = np.array([[1., 0, 0]])
#     Ad, Bd = c2d(A, B, H)
#     L = place_poles(Ad.T, C.T, np.exp(np.array([-400., -440., -480.])*H)).gain_matrix.T
#     Ktheta   = 4*Y_HALF**2*Kq                       # raideur modale fournie par les 4 coins
#     c        = 2*ZETA_ROLL*np.sqrt(Ktheta*JXX)      # amortissement modal vise
#     K_ROLL_D = c*ALLOC_MX*4*Y_HALF                  # = c*1/(...) -> par coin, cf. allocation
#     K_ROLL_D = c*1.923077                           # forme explicite identique
#     return Ad, Bd, L, K_ROLL_D

# def design_observer(tau_act):
#     """
#     Observateur d'état (Position, Vitesse, Force)
#     """
#     A_obs = np.array([[0, 1, 0],
#                       [0, 0, -1.0/M_CORNER],
#                       [0, 0, -1.0/tau_act]])
#     B_obs = np.array([[0], [0], [1.0/tau_act]])
#     C_obs = np.array([[1, 0, 0]]) 

#     Ad_obs, Bd_obs = c2d(A_obs, B_obs, H)

#     # Observateur très rapide
#     poles_cont = [-700.0, -760.0, -820.0]
#     poles_disc = np.exp(np.array(poles_cont) * H)

#     res = place_poles(Ad_obs.T, C_obs.T, poles_disc)
#     L = res.gain_matrix.T  

#     return Ad_obs, Bd_obs, L

# def main():
#     print("="*68)
#     print(" GENERATION DES MATRICES : LQI + OBSERVATEUR")
#     print("="*68)
#     print(" A COLLER DANS FunctionHeader.h\n")
    
#     for i in (1, 2, 3, 4):
        
#         # --- LA MODIFICATION EST ICI ---
#         # 1. Le LQI calcule fc (avant le notch), donc il DOIT prévoir le retard du notch
#         ta_lqi = TAU_IBF[i] + NOTCH_LAG 
        
#         # 2. L'observateur lira fcf (après le notch), il n'y a plus de NOTCH_LAG !
#         ta_obs = TAU_IBF[i] 
        
#         K = design_lqi_place(ta_lqi)
#         Ad_obs, Bd_obs, L = design_observer(ta_obs)
        
#         print(f"// ================= INDUCTEUR {i} =================")
#         print(f"#define LQI{i}_Q    {abs(K[0]):.1f}f")
#         print(f"#define LQI{i}_QD   {abs(K[1]):.2f}f")
#         print(f"#define LQI{i}_EPS  {abs(K[2]):.1f}f")
        
#         print(f"")
#         print(f"#define AD11_{i}  {Ad_obs[0,0]:.6f}f")
#         print(f"#define AD12_{i}  {Ad_obs[0,1]:.6f}f")
#         print(f"#define AD22_{i}  {Ad_obs[1,1]:.6f}f")
#         print(f"#define AD23_{i}  {Ad_obs[1,2]:.6f}f")  
#         print(f"#define AD33_{i}  {Ad_obs[2,2]:.6f}f")
#         print(f"#define BD3_{i}   {Bd_obs[2,0]:.6f}f")
        
#         print(f"#define L1_{i}    {L[0,0]:.4f}f")
#         print(f"#define L2_{i}    {L[1,0]:.2f}f")
#         print(f"#define L3_{i}    {L[2,0]:.2f}f")
#         print(f"\n")

#     Ad_r, Bd_r, L_r, KRD = design_roll_observer(Kq=18780.5, tau_act=1.5495e-3)
#     print(f"#define K_ROLL_D   {KRD:.1f}f")
#     print(f"#define ROLL_AD12  {Ad_r[0,1]:.6f}f")
#     print(f"#define ROLL_AD23  {Ad_r[1,2]:.6f}f")
#     print(f"#define ROLL_AD33  {Ad_r[2,2]:.6f}f")
#     print(f"#define ROLL_BD3   {Bd_r[2,0]:.6f}f")
#     print(f"#define ROLL_L1    {L_r[0,0]:.4f}f")
#     print(f"#define ROLL_L2    {L_r[1,0]:.2f}f")
#     print(f"#define ROLL_L3    {L_r[2,0]:.2f}f")

# if __name__ == "__main__":
#     main()

#!/usr/bin/env python3
# =============================================================================
#  lqi_par_inducteur.py (VERSION STABILISÉE + MODAL)
# =============================================================================

import numpy as np
from scipy.linalg import expm
from scipy.signal import place_poles

# =============================== PARAMETRES ==================================
M_CORNER  = 4.513            # masse portee par un inducteur [kg]
H         = 1.0/25e3         # periode ISR [s] (40 us)
NOTCH_LAG = 4.0e-3           # Retard effectif du filtre coupe-bande

# tau de boucle fermee de courant mesures (Kpi=2), par inducteur [s]
TAU_IBF = {1: 1.5495e-3, 2: 1.6263e-3, 3: 1.5495e-3, 4: 1.5495e-3}
# =============================================================================

def c2d(A, B, h):
    n = A.shape[0]; m = B.shape[1]
    Maug = np.zeros((n + m, n + m)); Maug[:n, :n] = A; Maug[:n, n:] = B
    Md = expm(Maug * h); return Md[:n, :n], Md[:n, n:]

def design_lqi_place(tau_act, wd=50.0):
    """LQI par placement sur 3 etats [delta, delta_point, eps] (KF non requis),
       puis VERIFICATION sur le modele 4 etats avec le retard actionneur.
       wd = pulsation dominante visee [rad/s]. 50 rad/s ~ les gains eprouves."""
    A3 = np.array([[0, 1, 0], [0, 0, 0], [-1., 0, 0]])
    B3 = np.array([[0], [1.0/M_CORNER], [0.]])
    Ad, Bd = c2d(A3, B3, H)
    
    pc = [-wd+0.6j*wd, -wd-0.6j*wd, -0.8*wd]          # paire amortie + integrateur
    K = place_poles(Ad, Bd, np.exp(np.array(pc)*H)).gain_matrix.flatten()
    
    # --- verification avec le retard actionneur (notch inclus) ---
    A4 = np.array([[0,1,0,0],[0,0,1.0/M_CORNER,0],[0,0,-1.0/tau_act,0],[-1.,0,0,0]])
    B4 = np.array([[0],[0],[1.0/tau_act],[0.]])
    Ad4, Bd4 = c2d(A4, B4, H)
    K4 = np.array([[K[0], K[1], 0.0, K[2]]])
    
    stable = np.all(np.abs(np.linalg.eigvals(Ad4 - Bd4@K4)) < 1.0)
    assert stable, f"INSTABLE avec le retard {tau_act*1e3:.1f}ms : baisser wd !"
    return K   # [Q, QD, EPS] -> memes conventions de signes qu'avant

def design_observer(tau_act):
    """Observateur d'état (Position, Vitesse, Force) par coin"""
    A_obs = np.array([[0, 1, 0], [0, 0, -1.0/M_CORNER], [0, 0, -1.0/tau_act]])
    B_obs = np.array([[0], [0], [1.0/tau_act]])
    C_obs = np.array([[1, 0, 0]]) 

    Ad_obs, Bd_obs = c2d(A_obs, B_obs, H)
    poles_cont = [-700.0, -760.0, -820.0]
    poles_disc = np.exp(np.array(poles_cont) * H)

    res = place_poles(Ad_obs.T, C_obs.T, poles_disc)
    L = res.gain_matrix.T  
    return Ad_obs, Bd_obs, L

# ====================== OBSERVATEUR MODAL ROULIS ======================
JXX      = 0.406      # kg.m2 - MESURE en vol (roulis 8.9Hz avec Kq=18780)
Y_HALF   = 0.13       # demi-entraxe gauche-droite [m]
ALLOC_MX = 1.0/(4*Y_HALF)
ZETA_ROLL = 0.5

def design_roll_observer(Kq, tau_act):
    """Observateur [theta_x, theta_x_point, Mx] + gain d'amortissement modal."""
    A = np.array([[0, 1, 0], [0, 0, -1.0/JXX], [0, 0, -1.0/tau_act]])
    B = np.array([[0], [0], [1.0/tau_act]])
    C = np.array([[1., 0, 0]])
    Ad, Bd = c2d(A, B, H)
    
    L = place_poles(Ad.T, C.T, np.exp(np.array([-400., -440., -480.])*H)).gain_matrix.T
    Ktheta   = 4*Y_HALF**2*Kq                       # raideur modale fournie par les 4 coins
    c        = 2*ZETA_ROLL*np.sqrt(Ktheta*JXX)      # amortissement modal vise
    K_ROLL_D = c*1.923077                           # forme explicite identique
    return Ad, Bd, L, K_ROLL_D

def main():
    print("="*68)
    print(" GENERATION DES MATRICES : LQI + OBSERVATEURS")
    print("="*68)
    print(" A COLLER DANS FunctionHeader.h\n")
    
    for i in (1, 2, 3, 4):
        ta_lqi = TAU_IBF[i] + NOTCH_LAG 
        ta_obs = TAU_IBF[i] 
        
        # wd=50.0 force le retour aux gains stables ~18780
        K = design_lqi_place(ta_lqi, wd=50.0)
        Ad_obs, Bd_obs, L = design_observer(ta_obs)
        
        print(f"// ================= INDUCTEUR {i} =================")
        print(f"#define LQI{i}_Q    {abs(K[0]):.1f}f")
        print(f"#define LQI{i}_QD   {abs(K[1]):.2f}f")
        print(f"#define LQI{i}_EPS  {abs(K[2]):.1f}f")
        
        print(f"")
        print(f"#define AD11_{i}  {Ad_obs[0,0]:.6f}f")
        print(f"#define AD12_{i}  {Ad_obs[0,1]:.6f}f")
        print(f"#define AD22_{i}  {Ad_obs[1,1]:.6f}f")
        print(f"#define AD23_{i}  {Ad_obs[1,2]:.6f}f")  
        print(f"#define AD33_{i}  {Ad_obs[2,2]:.6f}f")
        print(f"#define BD3_{i}   {Bd_obs[2,0]:.6f}f")
        
        print(f"#define L1_{i}    {L[0,0]:.4f}f")
        print(f"#define L2_{i}    {L[1,0]:.2f}f")
        print(f"#define L3_{i}    {L[2,0]:.2f}f")
        print(f"\n")

    # Génération de l'amortisseur de roulis avec le Kq stable
    Ad_r, Bd_r, L_r, KRD = design_roll_observer(Kq=18780.5, tau_act=1.5495e-3)
    print(f"// ================= OBSERVATEUR MODAL ===================")
    print(f"#define K_ROLL_D   {KRD:.1f}f")
    print(f"#define ROLL_AD12  {Ad_r[0,1]:.6f}f")
    print(f"#define ROLL_AD23  {Ad_r[1,2]:.6f}f")
    print(f"#define ROLL_AD33  {Ad_r[2,2]:.6f}f")
    print(f"#define ROLL_BD3   {Bd_r[2,0]:.6f}f")
    print(f"#define ROLL_L1    {L_r[0,0]:.4f}f")
    print(f"#define ROLL_L2    {L_r[1,0]:.2f}f")
    print(f"#define ROLL_L3    {L_r[2,0]:.2f}f")

if __name__ == "__main__":
    main()