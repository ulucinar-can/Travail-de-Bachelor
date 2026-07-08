# =============================================================================
#  notch_outil.py
#  Outil d'analyse du filtre coupe-bande (notch) du firmware.
#
#  - Choisis un jeu de coefficients predefini ("_0" a "_10") OU un notch
#    custom (F0_CUSTOM / BW_CUSTOM) -> il imprime les #define a coller.
#  - Trace |H| et la phase, avec un marqueur du retard equivalent.
#  - Imprime le NOTCH_LAG a mettre dans lqi-inertie.py : c'est le retard
#    equivalent moyen dans la bande de regulation (BANDE_REG), calcule par
#    tau_eq(f) = -phase(f) / omega  (PAS le point a -45 degres, qui ne vaut
#    que pour un filtre du 1er ordre).
# =============================================================================

import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import freqz

# ============================== A MODIFIER ===================================
CHOIX     = "_9"       # "_0".."_10" (jeux du FunctionHeader.h) ou "custom"
F0_CUSTOM = 85.3       # [Hz] centre du notch custom (si CHOIX = "custom")
BW_CUSTOM = 40.0       # [Hz] largeur de bande coupee du notch custom
F_RES     = 85.3       # [Hz] frequence de la resonance a attenuer (marqueur)
BANDE_REG = (5.0, 30.0)  # [Hz] bande de regulation pour le calcul du NOTCH_LAG
FS        = 25000.0    # [Hz] frequence de la boucle (ISR)
# =============================================================================

# Jeux de coefficients du FunctionHeader.h : (A1_B1, A2, B0_B2, description)
JEUX = {
    "_0":  (-1.980143772501098,     0.980339665209542,     0.9901698326047711,      "56 Hz, bande 47-90"),
    "_1":  (-1.98898514926503589,   0.9892508998390186,    0.9946254499195093,      "65 Hz, bande 47-90"),
    "_2":  (-1.97860927758394145,   0.97886214822594597,   0.989431074112973,       "65 Hz, bande 34-119"),
    "_3":  (-1.98719807873359366,   0.98751192990729475,   0.99375596495364737,     "70 Hz, bande 50-100"),
    "_4":  (-1.9770910357912317,    0.97738681058138155,   0.98869340529069083,     "70 Hz, bande 37-128"),
    "_5":  (-1.9896496113551767,    0.98999709932613533,   0.99499854966306767,     "75 Hz, bande 57-95"),
    "_6":  (-1.977307757140243538,  0.97763254912161579,   0.98881627456080790,     "75 Hz, bande 40-130 (actuel)"),
    "_7":  (-1.987853700192099483,  0.9882568325036572565, 0.99412841625182868,     "80 Hz, bande 60-108"),
    "_8":  (-1.9772418015560622,    0.97763254912162,      0.98881627456081,        "80 Hz, bande 46-136"),
    "_9":  (-1.988052149327327544,  0.9885052567179366534, 0.99425262835896833,     "85 Hz, bande 65-111"),
    "_10": (-1.988052149327327544,  0.9885052567179366534, 0.99425262835896833,     "85 Hz, bande 51-142 (= _9)"),
}


def notch_custom(f0, bw, fs):
    """Coefficients au format du firmware pour un notch (f0, bw)."""
    w0 = 2*np.pi*f0/fs
    t  = np.tan(np.pi*bw/fs)
    a2   = (1 - t)/(1 + t)
    b0b2 = (1 + a2)/2
    a1b1 = -2*b0b2*np.cos(w0)
    return a1b1, a2, b0b2


def analyse(a1b1, a2, b0b2, titre=""):
    b = [b0b2, a1b1, b0b2]
    a = [1.0, a1b1, a2]
    f = np.linspace(0.5, 200.0, 8000)
    w, Hc = freqz(b, a, worN=2*np.pi*f/FS)
    mag = 20*np.log10(np.abs(Hc))
    ph  = np.unwrap(np.angle(Hc))          # [rad]

    # retard equivalent tau(f) = -phase/omega (equivalent 1er ordre basse freq)
    tau_eq = -ph / (2*np.pi*f)             # [s]

    # NOTCH_LAG recommande : moyenne de tau_eq sur la bande de regulation
    mreg = (f >= BANDE_REG[0]) & (f <= BANDE_REG[1])
    lag = float(np.mean(tau_eq[mreg]))

    # attenuation et retard au marqueur F_RES
    kres = int(np.argmin(np.abs(f - F_RES)))
    k15  = int(np.argmin(np.abs(f - 15.0)))

    print(f"=== {titre} ===")
    print(f"attenuation a {F_RES:.1f} Hz : {mag[kres]:6.1f} dB")
    print(f"phase a 15 Hz : {np.degrees(ph[k15]):+6.2f} deg  ->  tau_eq = {tau_eq[k15]*1e3:.2f} ms")
    print(f"NOTCH_LAG recommande (moyenne {BANDE_REG[0]:.0f}-{BANDE_REG[1]:.0f} Hz) : "
          f"{lag*1e3:.2f} ms   ->   NOTCH_LAG = {lag:.2e}")

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
    ax1.plot(f, mag)
    ax1.axvline(F_RES, color='r', ls='--', alpha=0.6)
    ax1.plot(F_RES, mag[kres], 'ro')
    ax1.annotate(f"{mag[kres]:.1f} dB @ {F_RES:.1f} Hz", (F_RES, mag[kres]),
                 textcoords="offset points", xytext=(8, -12), color='r')
    ax1.axvspan(*BANDE_REG, color='g', alpha=0.10)
    ax1.set_ylabel("|H| [dB]"); ax1.grid(True, alpha=0.4)
    ax1.set_title(f"Notch {titre}   (zone verte = bande de regulation)")

    ax2.plot(f, np.degrees(ph))
    ax2.plot(15.0, np.degrees(ph[k15]), 'go')
    ax2.annotate(f"{np.degrees(ph[k15]):+.1f} deg @ 15 Hz\n-> NOTCH_LAG = {lag*1e3:.2f} ms",
                 (15.0, np.degrees(ph[k15])), textcoords="offset points",
                 xytext=(10, -30), color='g')
    ax2.axvspan(*BANDE_REG, color='g', alpha=0.10)
    ax2.set_xlabel("Frequence [Hz]"); ax2.set_ylabel("Phase [deg]")
    ax2.grid(True, alpha=0.4)
    plt.tight_layout()
    return lag, mag[kres]


def tableau_comparatif():
    print(f"\n{'jeu':5s} | {'description':26s} | {'att@'+format(F_RES,'.1f')+'Hz':>11s} | {'NOTCH_LAG':>10s}")
    print("-" * 63)
    f = np.linspace(0.5, 200.0, 8000)
    for nom, (a1b1, a2, b0b2, desc) in JEUX.items():
        w, Hc = freqz([b0b2, a1b1, b0b2], [1.0, a1b1, a2], worN=2*np.pi*f/FS)
        mag = 20*np.log10(np.abs(Hc))
        ph = np.unwrap(np.angle(Hc))
        tau = -ph/(2*np.pi*f)
        mreg = (f >= BANDE_REG[0]) & (f <= BANDE_REG[1])
        kres = int(np.argmin(np.abs(f - F_RES)))
        print(f"{nom:5s} | {desc:26s} | {mag[kres]:8.1f} dB | {np.mean(tau[mreg])*1e3:7.2f} ms")


if __name__ == "__main__":
    if CHOIX == "custom":
        a1b1, a2, b0b2 = notch_custom(F0_CUSTOM, BW_CUSTOM, FS)
        print("Coefficients custom a coller dans FunctionHeader.h :")
        print(f"#define A1_B1_C   {a1b1:.15f}")
        print(f"#define A2_C      {a2:.15f}")
        print(f"#define B0_B2_C   {b0b2:.15f}\n")
        analyse(a1b1, a2, b0b2, f"custom {F0_CUSTOM:.1f} Hz, bande {BW_CUSTOM:.0f} Hz")
    else:
        a1b1, a2, b0b2, desc = JEUX[CHOIX]
        analyse(a1b1, a2, b0b2, f"{CHOIX} ({desc})")

    tableau_comparatif()
    plt.show()
