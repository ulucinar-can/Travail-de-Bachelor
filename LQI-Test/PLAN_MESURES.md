# Plan de mesures — campagne « raideur sans tremblement »

Objectif de la journée : récupérer de la phase entre 10 et 30 Hz (boucle de courant + notch),
le mesurer par FRF en vol, et finir avec les données qui permettent de calculer le jeu de
pôles le plus raide admissible.

**Règles générales**
- UN seul changement à la fois entre deux essais.
- Pour chaque essai : noter dans un fichier texte → nom du CSV, Kpi, notch, pôles,
  observations (tremblement ? après combien de temps ? bruit ?), LED CC de l'alim.
- Oscillo branché sur le bus 48 V toute la journée (creux pendant les corrections ?).
- Couper le vol si une oscillation dépasse ~±0,5 mm visible.
- Vols FRF : 40 à 60 s de maintien calme, sans toucher la maquette.

---

## Bloc A — Boucle de courant, maquette POSÉE (~1 h, aucun risque)

### A1 — Échelon de courant, Kpi actuel
- [ ] Échelon de consigne 1 → 2 A sur l'inducteur 1 (ta méthode habituelle debugger/oscillo).
- [ ] Noter : constante de temps τ_ibf, dépassement, **erreur statique à 2–3 s** (intégrateur actif).
- [ ] Si erreur statique ≠ 0 : mesurer le courant RÉEL (pince/multimètre en série) →
      départager « le courant est faux » vs « la mesure est fausse » (offset capteur).

### A2 — Paliers de Kpi : 3,5 puis 5 (puis 8–10 si tout est propre)
À chaque palier, refaire l'échelon et noter :
- [ ] τ_ibf et dépassement
- [ ] bus 48 V à l'oscillo : creux ? LED CC ?
- [ ] bruit audible / échauffement carte de puissance
- **Critère d'arrêt** : dépassement > ~30 %, creux de bus visibles, sifflement ou carte chaude.

### A3 — Dérive thermique de l'erreur statique (5 min)
- [ ] Courant maintenu à 3 A sur les 4 voies, maquette posée.
- [ ] Noter l'erreur consigne−mesure des 4 voies à t = 0 et t = 5 min.
      Si elle dérive → offset thermique de la mesure (la calibration au boot ne suffit pas).

**Livrables bloc A** : Kpi retenu, les 4 τ_ibf mesurés (pour mise à jour de TAU_IBF),
verdict sur l'origine de l'erreur statique.

---

## Bloc B — Vols FRF (l'après-midi, ~15 min par vol)

⚠️ Le `.c` actuel est en pôles rapides : le vol B0 va probablement re-trembler lentement.
C'est VOULU (c'est la référence FRF). Le laisser osciller tant que ça reste < ±0,5 mm.

### B0 — Référence (rien à modifier)
- Config telle quelle : pôles [-80,-85,-90,-15], ton Kpi/N=7 actuel, notch large (_6).
- [ ] Vol 60 s → `vol_B0_ref.csv`

### B1 — Effet Kpi seul
- Modif : appliquer le Kpi retenu au bloc A. **Ne PAS régénérer les gains de position.**
- [ ] Vol 60 s → `vol_B1_kpi.csv`

### B2 — Notch étroit
- Modif : dans FunctionHeader.c (lignes ~25-26), passer aux coefficients 57–95 Hz :
  `float a[Na] = {A1_B1_5, A2_5};`
  `float b[Nb] = {B0_B2_5, A1_B1_5, B0_B2_5};`
- Garder Kpi de B1, mêmes gains de position.
- [ ] Vol 60 s → `vol_B2_notch.csv`
- [ ] Observations spécifiques : bourdonnement ~75 Hz à l'oreille / au toucher ?
      (dans le CSV, le 75 Hz réel apparaîtrait replié à 25 Hz — je le surveillerai.)

### B3 — (Optionnel, seulement si B2 sans aucun signe de 75 Hz) Sans notch du tout
- Modif 1 — bypass du notch, dans FunctionHeader.c :
  `float a[Na] = {0.0f, 0.0f};`
  `float b[Nb] = {1.0f, 0.0f, 0.0f};`
- Modif 2 — pôles doux par sécurité, dans Sustentation_Main.c remplacer les 4 lignes :
  `static const float LQI_Q[3]       = {2.31301821e+04f, 1.08647335e+03f, 1.08647335e+03f};`
  `static const float LQI_QD[3]      = {9.50282811e+02f, 4.46367845e+01f, 4.46367845e+01f};`
  `static const float LQI_EPS[3]     = {1.58667629e+05f, 7.45295255e+03f, 7.45295255e+03f};`
  `static const float LQI_EPS_INV[3] = {6.30248280e-06f, 1.34175012e-04f, 1.34175012e-04f};`
- [ ] Vol 60 s → `vol_B3_sansnotch.csv`
- [ ] ÉCOUTER : le moindre bourdonnement/sifflement → couper, remettre le notch _5.

### B4 — Pôles intermédiaires sur la meilleure config du jour
- Base : Kpi retenu + le notch retenu (B2 ou B3).
- Modif : dans lqi-inertie.py →
  - `TAU_IBF` = les 4 valeurs mesurées au bloc A,
  - `NOTCH_LAG` = `1.5e-3` si notch _5 retenu, `0.0` si sans notch,
  - `POLES_LQI = [-65.0, -70.0, -75.0, -13.0]`
  - Lancer le script, coller dans le .c : les 4 lignes LQI_* **ET** les 7 lignes OBS_*
    (les observateurs changent avec TAU_ACT).
- [ ] Vol 60 s → `vol_B4_inter.csv`

---

## Bloc C — Bonus si le temps le permet
- [ ] C1 : enregistrer la rotation Z (lacet) pendant un vol (noter l'heure de départ pour
      resynchroniser avec le log).
- [ ] C2 : oscillo sur le bus 48 V pendant un vol (pas seulement au sol).

---

## À m'envoyer à la fin
1. Les CSV nommés (`vol_B0_ref.csv` … `vol_B4_inter.csv`)
2. Le fichier de notes (config + observations par vol)
3. Les τ_ibf et le Kpi retenus au bloc A
4. Tes impressions à l'œil : quel vol semblait le plus stable / le plus raide ?

Je rends ensuite : la phase récupérée par chaque levier (FRF), le verdict notch,
et le jeu de pôles le plus raide admissible (critère |C·G| ≤ 0,5 à tous les points
critiques mesurés) pour le vol de raideur final.
