# Estimation de prix — AirWatt

Livrable « Price estimate » du cahier des charges : coût détaillé de la solution complète
(carte, communication, capteurs, actionneurs, passerelle), à partir de prix réels du marché.

**Fichier détaillé :** `estimation-prix.xlsx`, avec deux feuilles et une ligne par composant :
quantité, prix unitaire, total calculé par formule, boutique, lien et date du relevé.
Les prix ont été lus le **27/11/2026** sur les pages produits de deux boutiques tunisiennes :
**Didactico** (didactico.tn, Sfax) et **Electronic Shop** (es-online.tn).

## Résumé

| Configuration | Contenu | Total (TND) |
| --- | --- | --- |
| Prototype réellement réalisé | 1 ESP32 (BLE réel, capteurs simulés, passerelle sur PC) | **30,875** |
| Prototype complet (schéma `docs/schema`) | 1 nœud : ESP32, PMS5003, BME280, MOSFET, ventilateur 12 V, alimentation, LED RGB, buzzer, plaque d'essai, boîtier | **221,265** |
| Déploiement dans un atelier de 3 zones | 3 nœuds en boîtier étanche + 1 passerelle ESP32 | **644,193** |

Le déploiement exclut le variateur de fréquence de l'extracteur (souvent déjà présent) et
l'installation électrique, à chiffrer avec un fournisseur.

## Points d'attention (relevé honnête)

| Composant | Situation | Choix retenu |
| --- | --- | --- |
| PMS5003 | **Introuvable en Tunisie** (Didactico, es-online, seli.tn, Celectronix) | Import : ≈ 23 USD (≈ 69 TND) + ≈ 30 % de port et douane = **90 TND, estimation à confirmer**. Option locale : DSM501A à 31,350 TND, moins précis et sans sortie numérique UART |
| IRLZ44N | Introuvable en Tunisie | **IRL540N** (commande logique, 4,500 TND), en rupture au relevé. L'IRF520 en stock ne convient pas : il lui faut 10 V sur la grille |
| Ventilateur 80 mm 12 V | Introuvable (80 mm seulement en 24 V ou 230 V) | Ventilateur **60 mm 12 V** (14,000 TND, sur commande) ; schéma mis à jour |
| Prix Didactico | Remise « White Friday » de 5 % du 27 au 29/11/2026 | Prix affichés conservés ; prix habituels ≈ + 5 % (environ + 6 TND sur le prototype complet) |

## Ordre de grandeur de la rentabilité (hypothèses à valider)

Cet ordre de grandeur sert au pitch. Chaque hypothèse est annoncée comme telle.

| Hypothèse | Valeur |
| --- | --- |
| Puissance d'un extracteur d'atelier | 1,5 kW (hypothèse) |
| Fonctionnement | 8 h/jour, 250 jours/an (hypothèse) |
| Vitesse moyenne avec AirWatt | 60 % (hypothèse ; air propre la plupart du temps) |
| Puissance moyenne (loi cubique) | 1,5 × 0,6³ ≈ 0,32 kW, soit **≈ 78 % d'énergie économisée** |
| Énergie économisée par an et par extracteur | 1,5 × 0,78 × 8 × 250 ≈ **2 300 kWh** |

Au tarif industriel en vigueur (à relever auprès de la STEG), cette économie se compare
directement au coût de 644 TND du déploiement IoT. Le calcul exact reste à faire avec le
tarif réel et la puissance des extracteurs de l'atelier visé.

## Sources principales

| Composant | Lien |
| --- | --- |
| ESP32-WROOM-32 | https://didactico.tn/produit/carte-developpement-esp32-wroom-32/ |
| PMS5003 (prix international) | https://www.ebay.com/itm/405136238957 |
| BME280 | https://didactico.tn/produit/capteur-pression-atmospherique-gy-bme280/ |
| IRL540N | https://es-online.tn/produit/mosfet-de-puissance-canal-n |
| Ventilateur 6015 12 V | https://didactico.tn/produit/ventilateur-refroidissement-6015/ |
| Adaptateur 12 V 2 A | https://didactico.tn/produit/adaptateur-12v-2a-arduino/ |
| LM2596 | https://didactico.tn/produit/module-lm2596/ |
| Boîte étanche CNP-3103 | https://didactico.tn/produit/boite-de-jonction-etanche-cnp-3103/ |

Tous les autres liens figurent dans `estimation-prix.xlsx`. Pour mettre à jour un prix :
modifier `prix_releves.py`, puis lancer `generer_estimation.py` (ou modifier directement la
cellule dans Excel : les totaux se recalculent).
