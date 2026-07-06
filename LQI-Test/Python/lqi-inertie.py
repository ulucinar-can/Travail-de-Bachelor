#!/usr/bin/env python3
# =============================================================================
#  lqi-inertie.py
#  Synthese MIMO : LQI modal (pompage Z / tangage T / roulis R)
#  + observateurs modaux (Luenberger)
#  + matrices T_MAT (mesures -> torseur), E_MAT (forces -> efforts),
#    W_MAT (allocation efforts -> forces, norme minimale)
#
#  Coordonnees d'entrefer : l'entrefer DIMINUE quand le corps monte,
#  d'ou le signe -1/inertie dans les modeles (identique au SISO valide).
#
#  Disposition : 1 = avant-gauche, 2 = avant-droit,
#                3 = arriere-gauche, 4 = arriere-droit
#
#  Sortie : bloc C a coller dans FunctionHeader.h
# =============================================================================

import numpy as np
from scipy.linalg import expm
from scipy.signal import place_poles

# =============================== PARAMETRES ==================================
M_TOT = 18.052               # masse totale sustentee [kg] (= 4 x 4.513)
L = 1.23
g = 9.81

DX = 0.385
BX = DX/2
TX = 5.833 - 2
JX = (M_TOT*g*BX**2 *TX**2)/(4*np.pi**2*L)

DY = 0.278
BY = DY/2
TY = 2.267
JY    = (M_TOT*g*BY**2 *TY**2)/(4*np.pi**2*L)

# --- Parametrage du vol 2 (la meilleure config volee), conserve tel quel ---
# On garde J_MID / K_FORCE = 1.15 : c'est la base experimentale validee.
# La correction se fait UNIQUEMENT par les poles (voir POLES_LQI ci-dessous).
# Pour memoire, identifie sur le vol 2 : k reel = 0.639 (Sfc = 277 N pour
# porter 177 N, integrateur Z a -135 N), J_eff = 28.2 / 0.57 / 3.1 (Z/T/R).
K_FORCE = 1.15
J_MID   = np.sqrt(JX * JY)
JX_SYN  = J_MID / K_FORCE
JY_SYN  = J_MID / K_FORCE
M_SYN   = M_TOT / K_FORCE

# Positions des inducteurs PAR RAPPORT AU CG [m]
# 1<->3 = 28 cm (longitudinal, X) ; 1<->2 = 26 cm (lateral, Y)
# CG quasi centre (offsets < 3 mm via estimate_cg_from_forces)
X_I = np.array([+0.140, +0.140, -0.140, -0.140])
Y_I = np.array([+0.130, -0.130, +0.130, -0.130])

PLACEHOLDER = False

H          = 1.0/25e3        # periode ISR [s]
NOTCH_LAG  = 4.0e-3          # retard equivalent du coupe-bande 75 Hz [s]
TAU_IBF    = np.mean([0.001276750704319, 0.001361865236756
, 0.001276750704319, 0.001276750704319])
TAU_ACT    = TAU_IBF + NOTCH_LAG
G_ACC      = 9.81

# Poles ralentis par loop-shaping sur la plante MESUREE du vol 2 (q/u_sat) :
# avec [-80,-85,-90,-15], la boucle etait au point critique |R|=1.0-1.15 a
# 14.8 Hz (Z), 3.3 Hz (R) et ~30 Hz (Z/T) -> croissance lente observee en vol.
# [-50,-55,-60,-11] ramene tous ces points a |R| = 0.34-0.49 (6-9 dB de marge).
POLES_LQI  = [-80.0, -85.0, -90.0, -15.0]
POLES_OBS  = [-700.0, -760.0, -820.0]

TAU_REF    = 0.3             # lissage des references modales [s]
AW_FRAC    = 0.02            # tolerance anti-windup (fraction de l'effort statique)

# Gains SISO du firmware actuel (pour l'identification d'inertie par resonance)
SISO_M     = 4.513
SISO_FP    = SISO_M * G_ACC
SISO_FMAX  = 0.5 * 7.742e-3 * 2e-3 * (12.0/3e-3)**2 * 0.535
# =============================================================================


def c2d(A, B, h):
    n = A.shape[0]; m = B.shape[1]
    Maug = np.zeros((n + m, n + m)); Maug[:n, :n] = A; Maug[:n, n:] = B
    Md = expm(Maug * h)
    return Md[:n, :n], Md[:n, n:]


def design_lqi_place(inertia, tau_act, poles):
    """LQI 1 mode par placement de poles. Etats : [q, q_point, U, eps]"""
    A = np.array([[0, 1, 0, 0],
                  [0, 0, -1.0/inertia, 0],
                  [0, 0, -1.0/tau_act, 0],
                  [-1, 0, 0, 0]])
    B = np.array([[0], [0], [1.0/tau_act], [0]])
    Ad, Bd = c2d(A, B, H)
    poles_disc = np.exp(np.array(poles) * H)
    K = place_poles(Ad, Bd, poles_disc).gain_matrix.flatten()
    return K


def design_observer(inertia, tau_act, poles):
    """Observateur modal (q, q_point, effort U)"""
    A_obs = np.array([[0, 1, 0],
                      [0, 0, -1.0/inertia],
                      [0, 0, -1.0/tau_act]])
    B_obs = np.array([[0], [0], [1.0/tau_act]])
    C_obs = np.array([[1, 0, 0]])
    Ad_obs, Bd_obs = c2d(A_obs, B_obs, H)
    poles_disc = np.exp(np.array(poles) * H)
    L = place_poles(Ad_obs.T, C_obs.T, poles_disc).gain_matrix.T
    return Ad_obs, Bd_obs, L


def simulate_mode(inertia, Q, QD, EPS, Ado, Bdo, L, x0=1e-4, Tsim=4.0,
                  saturated=False, lo=None, hi=None):
    """Simulation discrete fidele a l'ISR : observateur -> loi -> notch -> plante.
    Retourne (stable, frequence_dominante, amplitude_finale)."""
    b0, b1, b2 = 0.98881627456080789517756, -1.977307757140243538, 0.98881627456080789517756
    a1, a2 = -1.977307757140243538, 0.97763254912161579

    Ap = np.array([[0., 1., 0.], [0., 0., -1.0/inertia], [0., 0., -1/1.5687e-3]])
    Bp = np.array([[0.], [0.], [1/1.5687e-3]])
    Adp, Bdp = c2d(Ap, Bp, H)
    p11, p12, p13 = Adp[0]; p21, p22, p23 = Adp[1]; p31, p32, p33 = Adp[2]
    g1, g2, g3 = Bdp[:, 0]
    AD12, AD23, AD33 = Ado[0, 1], Ado[1, 2], Ado[2, 2]
    BD3 = Bdo[2, 0]
    L1, L2, L3 = L[0, 0], L[1, 0], L[2, 0]

    n = int(Tsim / H)
    d, v, F = x0, 0.0, 0.0
    pe, ve, fe = x0, 0.0, 0.0
    xr = 0.0
    ufp = 0.0
    in1, in2, o1, o2 = 0.0, 0.0, 0.0, 0.0
    dec = 25
    out = np.empty(n // dec + 1); j = 0
    for k in range(n):
        e = d - pe
        pe_n = pe + AD12 * ve + L1 * e
        ve_n = ve + AD23 * fe + L2 * e
        fe_n = AD33 * fe + BD3 * ufp + L3 * e
        pe, ve, fe = pe_n, ve_n, fe_n
        u = Q * d + QD * ve - EPS * xr
        s = False
        if saturated:
            if u <= lo: u = lo; s = True
            elif u >= hi: u = hi; s = True
        if not s:
            xr += (-d) * H
        y = b0 * u + b1 * in1 + b2 * in2 - (a1 * o1 + a2 * o2)
        in2, in1 = in1, u
        o2, o1 = o1, y
        if saturated:
            if y < lo: y = lo
            elif y > hi: y = hi
        ufp = y
        d_n = p11 * d + p12 * v + p13 * F + g1 * y
        v_n = p21 * d + p22 * v + p23 * F + g2 * y
        F_n = p31 * d + p32 * v + p33 * F + g3 * y
        d, v, F = d_n, v_n, F_n
        if abs(d) > 1e6 * abs(x0):
            return False, 0.0, abs(d)
        if k % dec == 0:
            out[j] = d; j += 1
    x = out[:j]
    half = x[j // 2:]
    zc = np.where(np.diff(np.sign(half - half.mean())) > 0)[0]
    freq = (len(zc) - 1) / ((zc[-1] - zc[0]) * H * dec) if len(zc) > 2 else 0.0
    a1q = np.std(x[:j // 4]); a4q = np.std(x[-j // 4:])
    stable = a4q < 0.5 * a1q or a4q < 1e-3 * abs(x0)
    return stable, freq, (half.max() - half.min()) / 2


def inertia_from_resonance(f_meas_hz, axis, saturated=True):
    """Inverse la relation frequence de resonance -> inertie, pour une mesure
    faite avec le firmware SISO ACTUEL (gains LQI1). axis = 'X' (roulis) ou 'Y'."""
    poles_actuels = [-80.0, -85.0, -90.0, -95.0]
    K = design_lqi_place(SISO_M, TAU_ACT, poles_actuels)
    Q, QD, EPS = abs(K[0]), abs(K[1]), abs(K[3])
    Ado, Bdo, L = design_observer(SISO_M, TAU_ACT, POLES_OBS)

    def freq_of(m_eff):
        _, f, _ = simulate_mode(m_eff, Q, QD, EPS, Ado, Bdo, L,
                                x0=5e-4, Tsim=6.0, saturated=saturated,
                                lo=-SISO_FP, hi=SISO_FMAX - SISO_FP)
        return f

    m_lo, m_hi = 3.0, 300.0
    for _ in range(40):
        m_mid = 0.5 * (m_lo + m_hi)
        if freq_of(m_mid) > f_meas_hz:
            m_lo = m_mid
        else:
            m_hi = m_mid
    m_eff = 0.5 * (m_lo + m_hi)
    levers2 = np.sum(Y_I**2) if axis.upper() == 'X' else np.sum(X_I**2)
    return m_eff * levers2, m_eff


def estimate_cg_from_forces(f_mean):
    """Estime le decalage du CG depuis les forces statiques moyennes mesurees
    (ex. [32.6, 44.8, 46.5, 31.8] N releves dans lqi_tremblement.csv)."""
    f = np.asarray(f_mean, dtype=float)
    return float(np.dot(X_I, f) / f.sum()), float(np.dot(Y_I, f) / f.sum())


def fmt_row(v):
    return "{" + ", ".join(f"{x:.8e}f" for x in v) + "}"


def main():
    G = np.column_stack([np.ones(4), X_I, Y_I])          # entrefers = G @ q
    T = np.linalg.pinv(G)                                # q = T @ entrefers
    E = G.T                                              # efforts = E @ forces
    W = G @ np.linalg.inv(G.T @ G)                       # forces = W @ efforts
    assert np.allclose(E @ W, np.eye(3), atol=1e-12)

    U_STAT = np.array([M_TOT * G_ACC / K_FORCE, 0.0, 0.0])
    F_STAT = W @ U_STAT

    modes = [("Z (pompage)", M_SYN),
             ("T (tangage)", JY_SYN),
             ("R (roulis) ", JX_SYN)]

    print("=" * 70)
    print(" SYNTHESE MIMO : LQI MODAL + OBSERVATEURS + ALLOCATION")
    print("=" * 70)
    if PLACEHOLDER:
        print(" !!! ATTENTION : JX / JY / X_I / Y_I sont des PLACEHOLDERS !!!")
        print(" !!! Mesurer la geometrie et les inerties avant tout essai  !!!")
    print(f" tau_act = {TAU_ACT*1e3:.4f} ms | poles LQI = {POLES_LQI} | poles obs = {POLES_OBS}")
    print(f" F_STAT = {np.round(F_STAT, 2)} N (somme = {F_STAT.sum():.2f} N)")
    print(f" Noyau de E (torsion, jamais excite par W) : "
          f"{np.round(np.linalg.svd(E)[2][-1], 3)}")
    print()

    KQ, KQD, KEPS = [], [], []
    OAD12, OAD23, OAD33, OBD3, OL1, OL2, OL3 = [], [], [], [], [], [], []
    AWTOL = [AW_FRAC * U_STAT[0],
             AW_FRAC * U_STAT[0] * np.mean(np.abs(X_I)),
             AW_FRAC * U_STAT[0] * np.mean(np.abs(Y_I))]

    print("--- Verification par mode ---")
    for name, inertia in modes:
        K = design_lqi_place(inertia, TAU_ACT, POLES_LQI)
        Q, QD, EPS = abs(K[0]), abs(K[1]), abs(K[3])
        Ado, Bdo, L = design_observer(inertia, TAU_ACT, POLES_OBS)
        KQ.append(Q); KQD.append(QD); KEPS.append(EPS)
        OAD12.append(Ado[0, 1]); OAD23.append(Ado[1, 2]); OAD33.append(Ado[2, 2])
        OBD3.append(Bdo[2, 0])
        OL1.append(L[0, 0]); OL2.append(L[1, 0]); OL3.append(L[2, 0])

        wstar = np.sqrt(EPS / QD)
        marge = Q * QD / (inertia * EPS)
        stable, freq, _ = simulate_mode(inertia, Q, QD, EPS, Ado, Bdo, L)
        verdict = "STABLE" if stable else "INSTABLE <<< PROBLEME"
        warn = "" if marge > 8.0 else "  <<< MARGE FAIBLE"
        print(f" {name}: inertie = {inertia:7.3f} | Q = {Q:10.1f} | QD = {QD:8.2f} | "
              f"EPS = {EPS:11.1f}")
        print(f"     w* = {wstar:5.1f} rad/s ({wstar/2/np.pi:.2f} Hz) | "
              f"|L(jw*)| = {marge:5.1f}{warn} | sim : {verdict} (f_dom = {freq:.1f} Hz)")
    print()

    print("=" * 70)
    print(" A COLLER DANS FunctionHeader.h")
    print("=" * 70)
    print()
    print("// ======== MIMO genere par lqi-inertie.py — modes [Z, T, R] ========")
    print(f"static const float T_MAT[3][4] = {{{fmt_row(T[0])},")
    print(f"                                  {fmt_row(T[1])},")
    print(f"                                  {fmt_row(T[2])}}};")
    print(f"static const float E_MAT[3][4] = {{{fmt_row(E[0])},")
    print(f"                                  {fmt_row(E[1])},")
    print(f"                                  {fmt_row(E[2])}}};")
    print(f"static const float W_MAT[4][3] = {{{fmt_row(W[0])},")
    print(f"                                  {fmt_row(W[1])},")
    print(f"                                  {fmt_row(W[2])},")
    print(f"                                  {fmt_row(W[3])}}};")
    print(f"static const float F_STAT[4]   = {fmt_row(F_STAT)};")
    print(f"static const float U_STAT[3]   = {fmt_row(U_STAT)};")
    print()
    print(f"static const float LQI_Q[3]       = {fmt_row(KQ)};")
    print(f"static const float LQI_QD[3]      = {fmt_row(KQD)};")
    print(f"static const float LQI_EPS[3]     = {fmt_row(KEPS)};")
    print(f"static const float LQI_EPS_INV[3] = {fmt_row([1.0/e for e in KEPS])};")
    print(f"static const float AW_TOL[3]      = {fmt_row(AWTOL)};")
    print()
    print(f"static const float OBS_AD12[3] = {fmt_row(OAD12)};")
    print(f"static const float OBS_AD23[3] = {fmt_row(OAD23)};")
    print(f"static const float OBS_AD33[3] = {fmt_row(OAD33)};")
    print(f"static const float OBS_BD3[3]  = {fmt_row(OBD3)};")
    print(f"static const float OBS_L1[3]   = {fmt_row(OL1)};")
    print(f"static const float OBS_L2[3]   = {fmt_row(OL2)};")
    print(f"static const float OBS_L3[3]   = {fmt_row(OL3)};")
    print()
    print(f"#define REF_SMOOTH {H/TAU_REF:.6e}f")


if __name__ == "__main__":
    main()
    # Exemple d'identification une fois la geometrie mesuree :
    # J, m_eff = inertia_from_resonance(5.72, axis='X')
    # print(f"JX estime depuis la resonance roulis 5.72 Hz : {J:.3f} kg.m^2")
    # print("CG estime :", estimate_cg_from_forces([32.56, 44.76, 46.52, 31.81]))
