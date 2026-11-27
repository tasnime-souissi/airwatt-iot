# État de conformité avec le cahier des charges (Mme Ikbel)

Bilan au 27 novembre 2026, exigence par exigence, d'après `IoT_Semester_Project.pdf`.
Légende : ✅ fait et vérifié · ⚠️ partiel · ❌ manquant.

## Le point le plus important : le BLE

Le PDF dit : « *The drawn technology is the mandatory link that must be demonstrated* ».
Le BLE est donc **le** lien qui doit être montré, configuré et fonctionnel.

| Où est le BLE ? | État |
| --- | --- |
| Code du nœud capteur : serveur GATT, paquet de 12 octets, commandes (`node/`) | ✅ **testé sur ESP32 réelle le 27/11/2026** (ESP32-D0WD-V3) : annonce « AirWatt-Node », notifications toutes les 2 s, commandes reçues |
| Chaîne réelle nœud → BLE → PC → ThingsBoard | ✅ validée le 27/11/2026 : 0 paquet perdu, pic de pollution, mode manuel et vitesse pilotés depuis le tableau de bord |
| Code de la passerelle ESP32 : client BLE, décodage, commandes (`gateway/`) | ⚠️ écrit et compilé, **jamais testé**, et **sans envoi MQTT vers ThingsBoard** |
| Passerelle sur PC (`gateway_pc/`), pour une équipe avec une seule ESP32 | ✅ testée avec la carte le 27/11/2026 |
| Preuves du jalon avec nRF Connect (captures + vidéo) | ⚠️ à faire |
| Procédure de test du jalon BLE (`docs/checklist-carte.md`) | ✅ prête |
| Simulation Wokwi | ❌ **pas de BLE** : Wokwi ne simule pas le Bluetooth, la simulation envoie directement en Wi-Fi |
| Vidéo de démonstration | ❌ **pas de BLE** visible |

**Conséquence :** aujourd'hui, rien dans ce qui est montré ne prouve la technologie tirée.
C'est le premier jalon noté (« technologie configurée et communicante », chapitre 3) et une
partie du critère « Exactitude technique » (25 %). Ce n'est faisable que sur une vraie carte
ESP32 (le BLE est intégré à la puce, aucun module supplémentaire).

## Livrables (section 4 du PDF)

| Livrable | Exigé | État | Ce qui manque |
| --- | --- | --- | --- |
| Fiche sujet | Cas d'usage, technologie tirée, capteurs et actionneurs | ⚠️ | Le formulaire rendu ne contenait que les noms : vérifier avec Mme Ikbel que le cas d'usage (AirWatt), le BLE et les capteurs sont bien enregistrés |
| Prototype fonctionnel | Mesure et actionnement **via la technologie tirée** | ⚠️ | Mesure + actionnement OK en simulation, mais **sans BLE** |
| Schéma de câblage | Brochage du MCU, capteurs, actionneurs, **alimentation**, module de communication | ✅ | `docs/schema/` : schéma complet (SVG + PNG 3200 px), brochage, alimentation, choix des composants, bilan de consommation, contrôles avant mise sous tension |
| Plateforme et tableau de bord | Valeur/jauge, courbe, commande, flux en direct | ✅ | ThingsBoard fonctionne en direct (vérifié). Reste : messages d'erreur des widgets de commande |
| Estimation de prix | Nomenclature chiffrée réaliste de toute la solution | ✅ | `docs/prix/` : Excel avec formules, prix lus le 27/11/2026 chez Didactico et Electronic Shop avec liens ; prototype complet 221,265 TND, déploiement 3 zones 644,193 TND. Reste à confirmer : prix d'import du PMS5003 (estimation) |
| Pitch et démo | Présentation narrative + démo live (matériel ou émulateur) | ⚠️ | Vidéo `demo_iot3_rapide.mp4` prête. Manquent : les diapositives du pitch du Demo Day, et l'accord de Mme Ikbel pour une démo enregistrée |

## Jalons (section 5)

| Jalon | État |
| --- | --- |
| Technologie configurée et communicante avec le MCU (chapitre 3) | ✅ BLE validé sur carte le 27/11/2026 ; captures nRF Connect à joindre |
| Demo Day | ⚠️ simulation et vidéo prêtes, BLE absent |

## Critères d'évaluation (section 7)

| Critère | Poids | État | Commentaire |
| --- | --- | --- | --- |
| Exactitude technique | 25 % | ⚠️ | Logique capteurs/actionneurs solide et testée ; **BLE non démontré** |
| Plateforme et tableau de bord | 20 % | ✅ | Flux de bout en bout validé en simulation, alarmes, commandes |
| Pertinence au thème SETP | 10 % | ✅ | Angle énergie : ventilation à la demande, énergie économisée affichée |
| Storytelling et pitch | 20 % | ⚠️ | Vidéo prête ; pitch du Demo Day à préparer |
| Schéma et prix | 15 % | ✅ | Schéma complet et estimation sourcée produits le 27/11/2026 |
| Travail d'équipe | 10 % | ⚠️ | Rôles définis, mais chaque membre doit pouvoir présenter et expliquer sa partie |

## Ce qui reste, par ordre de priorité

1. **BLE sur une vraie ESP32** (jalon noté) : téléverser `node/`, vérifier avec l'application
   nRF Connect sur un téléphone. Une seule carte suffit pour ce jalon.
2. **Chaîne réelle complète** : ajouter l'envoi MQTT à la passerelle (`gateway/`), tester
   nœud → BLE → passerelle → ThingsBoard avec deux ESP32.
3. **Schéma de câblage** du vrai montage (PMS5003, BME280, MOSFET, ventilateur 12 V,
   LM2596, LED RGB, buzzer, alimentation).
4. **Estimation de prix** sourcée (boutiques tunisiennes, liens et date).
5. **Lecture réelle des capteurs** dans le nœud (PMS5003 + BME280, `SIMULATED_SENSORS=0`).
6. **Pitch du Demo Day** (diapositives + répartition de la parole entre les 4 membres).
7. **Questions à poser à Mme Ikbel** : démo enregistrée acceptée ? Comment démontrer le BLE
   si l'équipe passe par l'émulateur ? Fiche sujet complète ?
