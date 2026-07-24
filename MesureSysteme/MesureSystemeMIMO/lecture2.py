import serial
import time
import csv
import threading
import matplotlib.pyplot as plt

# --- CONFIGURATION ---
PORT = 'COM3'       
BAUDRATE = 921600
FICHIER_CSV = 'mesure_complete_4_inducteurs.csv'

enregistrement_actif = True

def ecouter_clavier():
    global enregistrement_actif
    while enregistrement_actif:
        commande = input()
        if commande.strip().lower() in ['s', 'stop']:
            enregistrement_actif = False
            break

# --- Noms des 33 colonnes (Temps + 32 valeurs de telemetry[]) ---
colonnes_csv = ['Temps(s)', 'frameCnt', 'state']
for k in range(3):                        # 3 axes modaux (MIMO)
    colonnes_csv.append(f'qm_ref_{k}(mm)')
for k in range(3):
    colonnes_csv.append(f'qm_{k}(mm)')
for k in range(3):
    colonnes_csv.append(f'vm_est_{k}(mm/s)')
for k in range(3):
    colonnes_csv.append(f'u_cmd_{k}')
for k in range(3):
    colonnes_csv.append(f'u_sat_{k}')
for k in range(3):
    colonnes_csv.append(f'integ_{k}')     # LQI_EPS[k] * eps_m[k]
for i in range(1, 5):                     # 4 inducteurs physiques
    colonnes_csv.append(f'Pos_{i}(mm)')
for i in range(1, 5):
    colonnes_csv.append(f'fc_{i}')
for i in range(1, 5):
    colonnes_csv.append(f'Cur_{i}(A)')

temps = []
donnees_globales = []

try:
    ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
    print(f"Connecté à {PORT} à {BAUDRATE} bauds.")
    print("--------------------------------------------------")
    print("🟢 ENREGISTREMENT EN COURS (32 variables)...")
    print("👉 Tapez 's' ou 'stop' puis appuyez sur Entrée pour terminer.")
    print("--------------------------------------------------")
    
    thread_clavier = threading.Thread(target=ecouter_clavier, daemon=True)
    thread_clavier.start()

    start_time = time.time()

    with open(FICHIER_CSV, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(colonnes_csv) 

        while enregistrement_actif: 
            if ser.in_waiting > 0:
                ligne = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if ligne:
                    valeurs_str = ligne.split(',')
                    
                    if len(valeurs_str) == 32:  # On attend maintenant 32 valeurs
                        try:
                            t_actuel = time.time() - start_time
                            valeurs = list(map(float, valeurs_str))
                            
                            writer.writerow([round(t_actuel, 4)] + valeurs)
                            
                            temps.append(t_actuel)
                            donnees_globales.append(valeurs)
                        except ValueError:
                            pass

except serial.SerialException as e:
    print(f"\n❌ Erreur de port série : {e}")
except KeyboardInterrupt:
    enregistrement_actif = False 
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
    
    print(f"\n🔴 Enregistrement terminé. {len(temps)} lignes sauvegardées.")
    
    if len(temps) > 0:
        cols = list(zip(*donnees_globales))  # cols[i] correspond à telemetry[i]

        noms_modaux = {
            0: "Z — Pilonnement (hauteur moyenne)",
            1: "T — Tangage (ind. 1+2 vs 3+4)",
            2: "R — Roulis (ind. 1+3 vs 2+4)",
        }

        def tracer_modal(k):
            qm_ref = cols[2 + k]
            qm     = cols[5 + k]
            vm_est = cols[8 + k]
            u_cmd  = cols[11 + k]
            u_sat  = cols[14 + k]
            integ  = cols[17 + k]

            fig, axs = plt.subplots(4, 1, figsize=(12, 12), sharex=True)

            # 1. Position modale : référence vs mesure
            axs[0].plot(temps, qm_ref, label='Référence', color='red', linestyle='--')
            axs[0].plot(temps, qm, label='Mesure', color='blue')
            axs[0].set_ylabel("Position modale (mm)")
            axs[0].legend(loc="upper right")
            axs[0].grid(True, linestyle='--', alpha=0.7)

            # 2. Vitesse modale estimée
            axs[1].plot(temps, vm_est, label='Vitesse estimée', color='cyan')
            axs[1].axhline(0, color='black', linewidth=1, alpha=0.5)
            axs[1].set_ylabel("Vitesse (mm/s)")
            axs[1].legend(loc="upper right")
            axs[1].grid(True, linestyle='--', alpha=0.7)

            # 3. Commande : avant / après saturation
            axs[2].plot(temps, u_cmd, label='u_cmd (avant sat.)', color='orange')
            axs[2].plot(temps, u_sat, label='u_sat (après sat.)', color='green', linestyle='--')
            axs[2].set_ylabel("Commande")
            axs[2].legend(loc="upper right")
            axs[2].grid(True, linestyle='--', alpha=0.7)

            # 4. Terme intégral LQI
            axs[3].plot(temps, integ, label='Terme intégral (LQI)', color='purple')
            axs[3].axhline(0, color='black', linewidth=1, alpha=0.5)
            axs[3].set_ylabel("Intégrale")
            axs[3].set_xlabel("Temps (s)")
            axs[3].legend(loc="upper right")
            axs[3].grid(True, linestyle='--', alpha=0.7)

            plt.suptitle(f"Vue modale — {noms_modaux[k]}")
            plt.tight_layout()
            plt.show()

        def tracer_inducteur(i):
            pos = cols[20 + (i - 1)]
            fc  = cols[24 + (i - 1)]
            cur = cols[28 + (i - 1)]

            fig, axs = plt.subplots(3, 1, figsize=(12, 9), sharex=True)

            # 1. Position physique
            axs[0].plot(temps, pos, label='Position mesurée', color='blue')
            axs[0].set_ylabel("Position (mm)")
            axs[0].legend(loc="upper right")
            axs[0].grid(True, linestyle='--', alpha=0.7)

            # 2. Force consigne
            axs[1].plot(temps, fc, label='Force consigne', color='purple')
            axs[1].set_ylabel("Force (N)")
            axs[1].legend(loc="upper right")
            axs[1].grid(True, linestyle='--', alpha=0.7)

            # 3. Courant
            axs[2].plot(temps, cur, label='Courant', color='green')
            axs[2].set_ylabel("Courant (A)")
            axs[2].set_xlabel("Temps (s)")
            axs[2].legend(loc="upper right")
            axs[2].grid(True, linestyle='--', alpha=0.7)

            plt.suptitle(f"Vue physique - INDUCTEUR {i}")
            plt.tight_layout()
            plt.show()

        while True:
            choix = input(
                "\n📊 Que tracer ? 'm0'/'m1'/'m2' (axes modaux), "
                "'1'-'4' (inducteurs), 'q' pour quitter : "
            ).strip().lower()

            if choix == 'q':
                break
            elif choix in ['m0', 'm1', 'm2']:
                tracer_modal(int(choix[1]))
            elif choix in ['1', '2', '3', '4']:
                tracer_inducteur(int(choix))
            else:
                print("Choix invalide.")