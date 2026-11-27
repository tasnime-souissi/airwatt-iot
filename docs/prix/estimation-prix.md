# Estimation de prix du système AirWatt

Ce document chiffre la solution complète (carte, communication, capteurs, actionneurs,
passerelle) à partir de prix réels du marché tunisien.

Les prix ont été relevés en **novembre 2026** sur les pages produits de deux boutiques :
**Didactico** (didactico.tn, Sfax) et **Electronic Shop** (es-online.tn). Le classeur Excel joint
détaille chaque composant : quantité, prix unitaire, total calculé par formule, boutique, lien et
période du relevé.

## 1. Synthèse

| Configuration | Contenu | Total (TND) |
| --- | --- | --- |
| Prototype réalisé | 1 ESP32 : communication BLE réelle, mesures simulées, passerelle sur ordinateur portable | **30,875** |
| Prototype complet | 1 nœud conforme au schéma de câblage : ESP32, PMS5003, BME280, MOSFET, ventilateur 12 V, alimentation, LED RGB, buzzer, plaque d'essai, boîtier | **221,265** |
| Déploiement dans un atelier de 3 zones | 3 nœuds en boîtier étanche et 1 passerelle ESP32 | **644,193** |

Le déploiement ne comprend ni le variateur de fréquence de l'extracteur, souvent déjà installé,
ni l'installation électrique, à chiffrer avec un électricien.

## 2. Disponibilité des composants

| Composant | Situation au moment du relevé | Choix retenu |
| --- | --- | --- |
| PMS5003 | Non disponible en Tunisie (Didactico, Electronic Shop, seli.tn, Celectronix) | Importation : environ 23 USD (69 TND) plus 30 % de port et de douane, soit **90 TND**. Équivalent local moins précis : DSM501A à 31,350 TND, sans sortie numérique UART |
| IRLZ44N | Non disponible | **IRL540N**, à commande logique, 4,500 TND, en rupture de stock au relevé. L'IRF520 disponible ne convient pas : il exige 10 V sur la grille |
| Ventilateur 80 mm 12 V | Non disponible (80 mm seulement en 24 V ou 230 V) | Ventilateur **60 mm 12 V**, 14,000 TND, sur commande |

Les prix Didactico relevés comprennent une remise temporaire de 5 %. Hors remise, le prototype
complet coûterait environ 6 TND de plus.

## 3. Rentabilité énergétique (ordre de grandeur)

| Paramètre | Valeur retenue |
| --- | --- |
| Puissance d'un extracteur d'atelier | 1,5 kW |
| Durée de fonctionnement | 8 h par jour, 250 jours par an |
| Vitesse moyenne avec AirWatt | 60 % (air propre la plupart du temps) |
| Puissance moyenne (loi cubique) | 1,5 × 0,6³ ≈ 0,32 kW, soit **≈ 78 % d'énergie économisée** |
| Énergie économisée par an et par extracteur | 1,5 × 0,78 × 8 × 250 ≈ **2 300 kWh** |

Au tarif industriel de la STEG, cette économie annuelle se compare directement aux 644 TND du
déploiement. Le calcul définitif se fera avec le tarif en vigueur et la puissance réelle des
extracteurs de l'atelier équipé.

## 4. Sources principales

| Composant | Lien |
| --- | --- |
| ESP32-WROOM-32 | https://didactico.tn/produit/carte-developpement-esp32-wroom-32/ |
| PMS5003 (prix international) | https://www.ebay.com/itm/405136238957 |
| BME280 | https://didactico.tn/produit/capteur-pression-atmospherique-gy-bme280/ |
| IRL540N | https://es-online.tn/produit/mosfet-de-puissance-canal-n |
| Ventilateur 6015 12 V | https://didactico.tn/produit/ventilateur-refroidissement-6015/ |
| Adaptateur 12 V 2 A | https://didactico.tn/produit/adaptateur-12v-2a-arduino/ |
| LM2596 | https://didactico.tn/produit/module-lm2596/ |
| Boîte de jonction étanche CNP-3103 | https://didactico.tn/produit/boite-de-jonction-etanche-cnp-3103/ |

Les liens de tous les autres composants figurent dans le classeur Excel.
