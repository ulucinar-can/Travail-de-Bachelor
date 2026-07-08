# =============================================================================
#  spectre_outil.py
#  Affiche le contenu frequentiel d'un signal de la telemetrie (CSV a 100 Hz).
#
#  - Choisis le fichier et la colonne (p1..p4, qz/qt/qr, c1..c4, fc1..fc4).
#  - Trace le signal temporel + son spectre (Welch), avec les 3 pics annotes.
#  - RAPPEL REPLIEMENT : la telemetrie est a 100 Hz -> visible de 0 a 50 Hz
#    seulement. Un pic affiche a f peut aussi etre un vrai signal a (100 - f).
#    Pour voir au-dela de 50 Hz : buffer RAM de debug a 1 kHz ou oscilloscope.
#    (Pour un CSV de buffer debug : mettre FS = 1000.0 et MODE = "brut".)
# =============================================================================

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy import signal

# ============================== A MODIFIER ===================================
FICHIER  = "MesureSansIntervention.csv"
COLONNE  = "p1"        # p1..p4 (entrefers), qz/qt/qr (modes), c1..c4, fc1..fc4
MODE     = "telemetrie"  # "telemetrie" (33 colonnes) ou "brut" (1 colonne, sans entete)
FS       = 100.0       # [Hz] 100 = telemetrie ; 1000 = buffer debug decime 1/25
FENETRE  = "vol"       # "vol" (etat >= 6 uniquement) ou "tout"
# =============================================================================

COLS = ['t','cnt','state','qrz','qrt','qrr','qz','qt','qr','vz','vt','vr',
        'ucz','uct','ucr','usz','ust','usr','iz','it','ir',
        'p1','p2','p3','p4','fc1','fc2','fc3','fc4','c1','c2','c3','c4']


def charge():
    if MODE == "brut":
        x = np.loadtxt(FICHIER)
        return x - x.mean(), np.arange(len(x)) / FS
    df = pd.read_csv(FICHIER)
    df.columns = COLS
    # rejet des lignes corrompues (UART) : qz doit etre la moyenne des 4 entrefers
    bad = (np.abs(df['qz'] - df[['p1', 'p2', 'p3', 'p4']].mean(axis=1)) > 0.03).to_numpy()
    df = df[~bad].reset_index(drop=True)
    if FENETRE == "vol":
        st = df['state'].to_numpy()
        if (st >= 6).any():
            i0 = int(np.argmax(st >= 6)) + 100          # saute 1 s apres la bascule
            df = df.iloc[i0:len(df) - 5]
        else:
            print("(pas de phase MIMO trouvee, j'utilise tout le fichier)")
    x = df[COLONNE].to_numpy().astype(float).copy()
    t = (df['cnt'].to_numpy() - df['cnt'].to_numpy()[0]) / FS
    return x - x.mean(), t


def main():
    x, t = charge()
    print(f"{FICHIER} / {COLONNE} : {len(x)} points ({len(x)/FS:.1f} s), "
          f"std = {x.std()*1000:.1f} (um si mm / mA si A)")

    nper = min(1024, len(x))
    f, P = signal.welch(x, fs=FS, nperseg=nper, noverlap=nper // 2, detrend='linear')
    PdB = 10 * np.log10(np.maximum(P, 1e-30))

    # 3 pics dominants entre 0.5 Hz et Nyquist
    band = (f > 0.5) & (f < 0.995 * FS / 2)
    fb, Pb = f[band], P[band].copy()
    pics = []
    for _ in range(3):
        k = int(np.argmax(Pb))
        pics.append((fb[k], Pb[k]))
        Pb[max(0, k - 4):k + 5] = 0
    print("Pics dominants :")
    for fp, pp in pics:
        alias = FS - fp
        print(f"  {fp:6.2f} Hz  (puissance {pp:.3g})"
              + (f"   [ou repliement d'un vrai {alias:.1f} Hz]" if FS <= 200 else ""))

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(11, 7))
    ax1.plot(t, x, lw=0.6)
    ax1.set_xlabel("Temps [s]"); ax1.set_ylabel(COLONNE)
    ax1.set_title(f"{FICHIER} — {COLONNE} ({'vol' if FENETRE == 'vol' else 'tout'})")
    ax1.grid(True, alpha=0.4)

    ax2.plot(f[band], PdB[band])
    for fp, pp in pics:
        ax2.plot(fp, 10 * np.log10(pp), 'ro')
        ax2.annotate(f"{fp:.2f} Hz", (fp, 10 * np.log10(pp)),
                     textcoords="offset points", xytext=(6, 6), color='r')
    ax2.set_xlabel("Frequence [Hz]"); ax2.set_ylabel("PSD [dB]")
    ax2.grid(True, alpha=0.4)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
