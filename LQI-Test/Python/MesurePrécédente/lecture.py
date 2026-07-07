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

# Noms des 32 colonnes
colonnes_csv = ['Temps(s)']
for i in range(1, 5):
    colonnes_csv += [f'Pos_C{i}(mm)', f'Pos_M{i}(mm)', f'Err_Pos_{i}(mm)', f'Cur_C{i}(A)', f'Cur_M{i}(A)', f'Force_C{i}', f'Xr_{i}', f'Vit_{i}']

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
        colonnes_data = list(zip(*donnees_globales)) 
        
        while True:
            choix = input("\n📊 Quel inducteur voulez-vous tracer ? (1, 2, 3, 4) ou 'q' pour quitter : ").strip()
            
            if choix.lower() == 'q':
                break
                
            if choix in ['1', '2', '3', '4']:
                idx = int(choix)
                offset = (idx - 1) * 8  # Décalage de 8 colonnes par inducteur
                
                # Extraction des données
                pos_c = colonnes_data[offset + 0]
                pos_m = colonnes_data[offset + 1]
                err_pos = colonnes_data[offset + 2]
                cur_c = colonnes_data[offset + 3]
                cur_m = colonnes_data[offset + 4]
                force_c = colonnes_data[offset + 5]
                xr_val = colonnes_data[offset + 6]
                vitesse = colonnes_data[offset + 7]
                
                # Création de 5 sous-graphiques au lieu de 4
                fig, axs = plt.subplots(5, 1, figsize=(12, 12), sharex=True)
                
                # 1. Position
                axs[0].plot(temps, pos_c, label='Consigne', color='red', linestyle='--')
                axs[0].plot(temps, pos_m, label='Mesure', color='blue')
                axs[0].set_ylabel("Position (mm)")
                axs[0].legend(loc="upper right")
                axs[0].grid(True, linestyle='--', alpha=0.7)
                
                # 2. Erreur de Position
                axs[1].plot(temps, err_pos, label='Erreur (ep)', color='magenta')
                axs[1].axhline(0, color='black', linewidth=1) # Ligne à 0 pour bien voir l'écart
                axs[1].set_ylabel("Erreur Pos (mm)")
                axs[1].legend(loc="upper right")
                axs[1].grid(True, linestyle='--', alpha=0.7)
                
                # 3. Courant
                axs[2].plot(temps, cur_c, label='Consigne', color='red', linestyle='--')
                axs[2].plot(temps, cur_m, label='Mesure', color='green')
                axs[2].set_ylabel("Courant (A)")
                axs[2].legend(loc="upper right")
                axs[2].grid(True, linestyle='--', alpha=0.7)
                
                # 4. Force et Intégrale
                axs[3].plot(temps, force_c, label='Force Consigne', color='purple')
                ax3_bis = axs[3].twinx() 
                ax3_bis.plot(temps, xr_val, label='Intégrale Xr', color='orange')
                axs[3].set_ylabel("Force (N)")
                ax3_bis.set_ylabel("Xr", color='orange')
                axs[3].legend(loc="upper left")
                ax3_bis.legend(loc="upper right")
                axs[3].grid(True, linestyle='--', alpha=0.7)
                
                # 5. Vitesse
                axs[4].plot(temps, vitesse, label='Vitesse', color='cyan')
                axs[4].set_ylabel("Vitesse")
                axs[4].set_xlabel("Temps (s)")
                axs[4].legend(loc="upper right")
                axs[4].grid(True, linestyle='--', alpha=0.7)

                plt.suptitle(f"Analyse Systémique - INDUCTEUR {idx}")
                plt.tight_layout()
                plt.show() 
            else:
                print("Choix invalide.")