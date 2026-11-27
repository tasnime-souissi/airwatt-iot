# Plateforme ThingsBoard

La supervision repose sur ThingsBoard Cloud (`thingsboard.cloud`).

## Configuration

| Élément | Nom | Rôle |
| --- | --- | --- |
| Profil d'appareil | `AirWatt Node` | Transport MQTT ; porte les règles d'alarme |
| Appareil | `AirWatt-Atelier-01` | Identifié par un jeton d'accès privé |
| Tableau de bord | `AirWatt — Atelier de vulcanisation` | 13 widgets, temps réel sur la dernière heure |

## Règles d'alarme

| Alarme | Donnée surveillée | Déclenchement | Effacement |
| --- | --- | --- | --- |
| Pollution PM2.5 | `pm25` | Avertissement si > 15 ; critique si > 35 pendant 30 s | `pm25` ≤ 15 |
| Capteur PMS5003 en défaut | `pms_ok` | Majeure si `pms_ok` est faux | `pms_ok` redevient vrai |
| Nœud hors ligne | `pm25` | Majeure si aucune donnée depuis 60 s | Nouvelle mesure reçue |

Le délai de 30 s sur l'alarme critique évite les fausses alertes lors d'un pic très bref.

## Tableau de bord

| Zone | Widget | Données ou méthodes RPC |
| --- | --- | --- |
| Qualité de l'air | Carte de valeur | `aqi`, affiché « Bon », « Moyen » ou « Mauvais » en vert, orange ou rouge |
| Particules | Deux cartes de valeur colorées | `pm25` et `pm10`, colorées selon les seuils de l'indice |
| Ambiance et ventilation | Trois cartes de valeur | `temp`, `hum`, `fan` |
| Tendance | Graphique temporel sur une heure | `pm25`, `pm10`, `fan` |
| Énergie | Deux cartes de valeur | `saved_pct` (%), `saved_wh` (Wh) |
| Mode | Interrupteur | État lu dans la donnée `mode` ; commande `setMode` (activé = automatique) |
| Vitesse manuelle | Bouton rotatif 0 à 100 % | `getFan` / `setFan`, actif en mode manuel |
| Seuil rouge PM2.5 | Bouton rotatif 16 à 150 µg/m³ | `getThreshold` / `setThreshold` |
| Alarmes | Tableau d'alarmes | Alarmes de l'appareil |

## Réimporter le tableau de bord

Le tableau de bord est sauvegardé dans
[`thingsboard/tableau-de-bord-airwatt.json`](thingsboard/tableau-de-bord-airwatt.json) ; le
fichier ne contient aucun jeton.

1. Dans ThingsBoard : **Dashboards**, puis **+** et **Import dashboard**, et choisir ce fichier.
2. Vérifier que l'alias « Atelier » pointe vers l'appareil `AirWatt-Atelier-01` du compte.

## Vérification avec le jumeau numérique

| Action sur la puce PMS5003 simulée | Résultat attendu |
| --- | --- |
| PM2.5 à 10 µg/m³ | Indice « Bon » en vert, ventilateur à 20 % |
| PM2.5 à 25 µg/m³ | Indice « Moyen » en orange, alarme d'avertissement, ventilateur vers 50 % |
| PM2.5 à 60 µg/m³ | Indice « Mauvais » en rouge, buzzer, alarme critique après 30 s, ventilateur à 100 % |
| Retour à 10 µg/m³ | Alarmes effacées, ventilateur au ralenti |
| Panne à 1 ou 2 | Alarme « Capteur PMS5003 en défaut », ventilateur à 100 % |
| Simulation arrêtée 60 s | Alarme « Nœud hors ligne » |
