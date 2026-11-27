# ThingsBoard — configuration AirWatt

État au 27 novembre 2026 : **configuré** sur ThingsBoard Cloud (`thingsboard.cloud`,
version 4.4). Responsable : Membre 3.

Ce document décrit ce qui existe, et comment le refaire sur un autre compte si besoin.

## Ce qui est en place

| Élément | Nom | Remarque |
| --- | --- | --- |
| Profil d'appareil | `AirWatt Node` | Transport MQTT par défaut, porte les règles d'alarme |
| Appareil | `AirWatt-Atelier-01` | Son jeton est dans `wokwi/secrets.h` (privé) |
| Tableau de bord | `AirWatt — Atelier de vulcanisation` | 13 widgets, temps réel sur la dernière heure |

Le jeton de l'appareil ne doit jamais être publié (GitHub public, projet Wokwi public).

## Règles d'alarme (profil `AirWatt Node` > onglet « Alarm rules »)

Depuis la version 4.3, ThingsBoard gère les alarmes avec des « arguments » (les clés
surveillées) et des conditions, et non plus dans les paramètres du profil.

| Alarme | Argument | Déclenchement | Effacement |
| --- | --- | --- | --- |
| Pollution PM2.5 | `pm25` | Avertissement si > 15 ; Critique si > 35 pendant 30 s | `pm25` ≤ 15 |
| Capteur PMS5003 en defaut | `pms_ok` | Majeure si `pms_ok` = faux | `pms_ok` = vrai |
| Noeud hors ligne | `pm25` | Majeure si aucune donnée depuis 60 s | nouvelle mesure reçue |

15 µg/m³ : repère 24 h de l'OMS (2021) ; 35 µg/m³ : repère du prototype.

## Tableau de bord

| Zone | Widget | Clés ou méthodes RPC |
| --- | --- | --- |
| État de l'air | Carte de valeur | `aqi` affiché « Bon », « Moyen », « Mauvais », en vert, orange ou rouge |
| Particules | 2 grandes cartes de valeur colorées | `pm25` : vert < 15, orange 15–35, rouge > 35 ; `pm10` : vert < 45, orange 45–100, rouge > 100 (les jauges « justgage » restaient bloquées sur leur première valeur, remplacées le 27/11) |
| Ambiance, ventilateur | 3 cartes de valeur | `temp`, `hum`, `fan` |
| Tendance | Graphique temporel (1 h) | `pm25`, `pm10`, `fan` |
| Énergie | 2 cartes de valeur | `saved_pct` (%), `saved_wh` (Wh) |
| Mode | Interrupteur | état lu dans la télémétrie `mode` ; commande RPC `setMode` (activé = automatique) |
| Vitesse manuelle | Bouton rotatif 0–100 | `getFan` / `setFan` (agit en mode manuel) |
| Seuil rouge PM2.5 | Bouton rotatif 16–150 | `getThreshold` / `setThreshold` |
| Alertes | Tableau d'alarmes | alarmes de l'appareil |

Quand aucun appareil n'est connecté, les commandes affichent « Failed to parse the payload » :
c'est normal, elles attendent une réponse de l'ESP32. Le message disparaît dès que la
simulation Wokwi (ou la passerelle) tourne.

**Sauvegarde :** `docs/thingsboard/tableau-de-bord-airwatt.json` (exporté le 27/11/2026).
Pour le réimporter (autre compte, problème le jour J) : Dashboards > + > Import dashboard,
choisir ce fichier, puis vérifier que l'alias « Atelier » pointe vers l'appareil
`AirWatt-Atelier-01` du nouveau compte (l'identifiant d'appareil change d'un compte à l'autre).
Le fichier ne contient aucun jeton.

## Données de test

Le 27 novembre 2026, 41 mesures de test ont été envoyées à la main (un pic de pollution
sur 20 minutes) pour vérifier le tableau de bord. Elles restent dans l'historique ; on peut
les supprimer depuis l'appareil, onglet « Dernière télémétrie », si besoin.

## Scénario de vérification avec Wokwi

| Action | Résultat attendu |
| --- | --- |
| Curseur PM2.5 de la puce PMS5003 à 10 | Carte « Bon » en vert, ventilateur à 20 %, énergie économisée élevée |
| Curseur PM2.5 à 25 | « Moyen » en orange, alarme Avertissement, ventilateur vers 50 % |
| Curseur PM2.5 à 60 | « Mauvais » en rouge, buzzer, alarme Critique après 30 s, ventilateur à 100 % |
| Curseur PM2.5 à 10 | Alarmes effacées, ventilateur au ralenti |
| Interrupteur sur arrêt (manuel), bouton vitesse à 70 | LED bleue à 70 %, `mode` = manuel |
| Arrêter la simulation 60 s | Alarme « Noeud hors ligne » |
| Curseur « Panne » de la puce à 1 ou 2 | Alarme « Capteur PMS5003 en defaut », ventilateur à 100 % |
| Curseur « Scénario auto » à 1 | Pic de pollution rejoué toutes les 2 minutes, sans manipulation |
