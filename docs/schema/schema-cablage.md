# Schéma de câblage du nœud capteur AirWatt

Ce document décrit le montage complet du nœud capteur : brochage du microcontrôleur, raccordement
des capteurs et des actionneurs, alimentation et module de communication BLE.

![Schéma de câblage du nœud capteur AirWatt](schema-cablage.png)

## 1. Brochage de l'ESP32 DevKit V1

| Broche ESP32 | Relié à | Type | Remarque |
| --- | --- | --- | --- |
| VIN | +5V (sortie LM2596) | Alimentation | Entrée du régulateur 3,3 V de la carte |
| GND | Masse commune | Alimentation | |
| 3V3 | VIN du BME280 | Alimentation | Sortie du régulateur de la carte |
| GPIO16 (RX2) | TX du PMS5003 (broche 5) | UART 9600 bauds | Trames de 32 octets |
| GPIO17 (TX2) | RX du PMS5003 (broche 4) | UART | Commandes du capteur (facultatif) |
| GPIO21 (SDA) | SDA du BME280 | I2C | Adresse 0x76 ou 0x77 |
| GPIO22 (SCL) | SCL du BME280 | I2C | |
| GPIO25 | Grille de Q2 via R1 220 Ω | PWM 25 kHz | Vitesse du ventilateur |
| GPIO26 | LED RGB rouge via R3 220 Ω | Sortie | |
| GPIO27 | LED RGB verte via R4 100 Ω | Sortie | |
| GPIO32 | LED RGB bleue via R5 100 Ω | Sortie | |
| GPIO33 | Base de Q1 via R6 1 kΩ | Sortie | Commande du buzzer |
| GPIO2 | LED intégrée à la carte | PWM | Indicateur de vitesse en configuration « carte seule » |
| GPIO0 | Bouton BOOT de la carte | Entrée | Déclenchement d'un pic de pollution de test |

**Module de communication.** La radio BLE est intégrée au module ESP32-WROOM-32, avec une antenne
gravée sur le circuit. Aucun module externe n'est câblé ; l'antenne doit rester éloignée des
pièces métalliques et du ventilateur.

## 2. Alimentation

| Élément | Rôle |
| --- | --- |
| Adaptateur secteur 12 V / 2 A | Alimente le ventilateur (+12V) et le régulateur LM2596 |
| Régulateur LM2596 | Abaisse le 12 V à 5,0 V pour l'ESP32 (VIN) et le PMS5003 ; tension réglée au multimètre avant le premier branchement de l'ESP32 |
| Régulateur 3,3 V de la carte ESP32 | Alimente le BME280 et la LED RGB |
| Condensateur C1, 470 µF / 25 V | Placé sur le +5V, il absorbe les appels de courant du ventilateur interne du PMS5003 et des émissions radio |

Toutes les masses sont reliées. Le +12 V n'est raccordé à aucune broche de l'ESP32.

## 3. Choix des composants

| Repère | Composant | Justification |
| --- | --- | --- |
| Q2 | MOSFET IRL540N, à commande logique | Seuil de grille de 1 à 2 V : commute pleinement avec les 3,3 V de l'ESP32. L'IRF520, courant en module, exige 10 V sur la grille. L'IRLZ44N, équivalent, n'est pas disponible en Tunisie |
| R1 | 220 Ω | Limite le courant de charge de la grille vu par la broche GPIO25 |
| R2 | 10 kΩ | Maintient Q2 bloqué au démarrage : le ventilateur reste arrêté tant que l'ESP32 ne le pilote pas |
| D1 | 1N4007 | Diode de roue libre : absorbe la surtension du moteur à chaque coupure du PWM |
| Q1, R6 | 2N2222 et 1 kΩ | Le buzzer consomme environ 30 mA, au-delà des 20 mA conseillés par broche ; courant de base ≈ (3,3 − 0,7) / 1000 ≈ 2,6 mA |
| R3 | 220 Ω (rouge) | (3,3 − 2,0) / 220 ≈ 6 mA |
| R4, R5 | 100 Ω (vert, bleu) | Tension de seuil voisine de 3 V : 100 Ω donnent environ 3 mA, contre 1 mA seulement avec 220 Ω |

## 4. Bilan de consommation (valeurs maximales)

| Élément | Rail | Courant maximal | Source |
| --- | --- | --- | --- |
| ESP32, radio BLE active | +5V | ≈ 130 mA | Fiche technique ESP32 |
| PMS5003 | +5V | ≤ 100 mA | Fiche technique Plantower |
| Buzzer, pendant une alerte | +5V | ≈ 30 mA | Buzzer actif 5 V |
| BME280 et LED RGB | +3V3 (via l'ESP32) | ≈ 15 mA | Compris dans le courant de la carte |
| **Total sur le rail +5V** | | **≈ 260 mA, soit 1,3 W** | |
| Ventilateur 60 mm | +12V | 100 à 150 mA, soit 1,8 W au plus | Caractéristiques du ventilateur |
| LM2596 (rendement ≈ 85 %) | +12V | 1,3 W / 0,85 / 12 V ≈ 130 mA | |
| **Total sur l'adaptateur 12 V** | | **≈ 280 mA, soit 3,4 W** | Marge d'un facteur 7 avec un adaptateur de 2 A |

Le nœud consomme donc moins de 4 W, ventilateur compris. Sur un extracteur d'atelier de
plusieurs centaines de watts, une réduction de vitesse de moitié divise la puissance absorbée par
huit : la consommation de l'électronique de pilotage est négligeable devant l'énergie économisée.

## 5. Contrôles avant la première mise sous tension

1. Régler la sortie du LM2596 à 5,0 V à vide, au multimètre, avant de brancher l'ESP32.
2. Vérifier qu'aucun fil ne relie le +12 V à une broche de l'ESP32.
3. Vérifier l'orientation de D1 (bague côté +12 V) et de C1 (borne négative à la masse).
4. Contrôler le brochage du connecteur du PMS5003 sur la fiche du câble fourni, l'ordre des fils
   variant d'un fournisseur à l'autre.
5. Au démarrage, vérifier dans le moniteur série que le BME280 est reconnu (identifiant 0x60).
