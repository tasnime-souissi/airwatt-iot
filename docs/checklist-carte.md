# Checklist : le jour où la carte ESP32 arrive

Objectif : valider le jalon noté « BLE configuré et communicant », puis la chaîne réelle
nœud -> BLE -> PC -> ThingsBoard. Durée prévue : environ 1 heure.

## À apporter / préparer

- [ ] La carte ESP32 (modèle classique « DevKit » ou « WROOM-32 » ; pas d'ESP32-S2, sans Bluetooth)
- [ ] Un câble USB **qui transmet les données** (pas un câble de charge seule)
- [ ] L'application gratuite **nRF Connect** installée sur un téléphone Android ou iPhone
- [ ] Le Bluetooth du PC activé

## Étape 1 — Brancher et programmer (15 min)

- [ ] Brancher la carte : vérifier qu'un port COM apparaît (`pio device list` ; sinon, pilote CP210x ou CH340)
- [ ] Téléverser le nœud : `pio run -t upload` dans `node/` (configuration `carte_seule`)
- [ ] Moniteur série : une ligne toutes les 2 s, `[BLE] Annonce lancée sous le nom AirWatt-Node`
- [ ] Appuyer sur **BOOT** : `[SIM] Bouton BOOT : pic de pollution déclenché`, la LED bleue
      de la carte s'allume de plus en plus fort puis clignote (air mauvais)

## Étape 2 — Jalon BLE avec le téléphone (15 min)

- [ ] nRF Connect > SCAN : `AirWatt-Node` apparaît → **capture d'écran**
- [ ] CONNECT : le service `6e7a0001-…` apparaît avec ses 2 caractéristiques → **capture**
- [ ] Activer les notifications sur `6e7a0002-…` : une valeur de 12 octets toutes les 2 s → **capture**
- [ ] Écrire `020F00` sur `6e7a0003-…` (setFan, 15 %) après `010000` (mode manuel) :
      le moniteur série affiche `[CMD] Mode manuel` puis `[CMD] Ventilateur manuel à 15 %`
- [ ] Filmer 30 s : téléphone + moniteur série → **preuve du jalon**

Décodage d'un paquet (petit-boutiste) : octets 0-1 = PM2.5, 2-3 = PM10, 4-5 = température × 10,
6 = humidité, 7 = ventilateur %, 8 = indice (0 vert, 1 orange, 2 rouge), 9 = drapeaux, 10-11 = compteur.

## Étape 3 — Chaîne réelle jusqu'à ThingsBoard (20 min)

- [ ] Déconnecter le téléphone (un seul client BLE à la fois)
- [ ] Arrêter la simulation Wokwi
- [ ] Lancer `gateway_pc/lancer_passerelle.bat`
- [ ] ThingsBoard reçoit les mesures, avec `link` = `ble`
- [ ] Appuyer sur BOOT : le tableau de bord passe au rouge, alarme Critique après 30 s
- [ ] Depuis le tableau de bord : mode manuel, vitesse 70 % → la LED de la carte change d'intensité
- [ ] Éloigner la carte de quelques mètres : `rssi` diminue ; la couper : alarme « Noeud hors ligne »
- [ ] Filmer 1 min → **preuve de la chaîne complète**

## Si quelque chose bloque

| Symptôme | Cause probable |
| --- | --- |
| Pas de port COM | Câble de charge seule, ou pilote USB manquant |
| Échec du téléversement « Failed to connect » | Maintenir BOOT appuyé au début du téléversement |
| nRF Connect ne voit pas la carte | Carte ESP32-S2 (pas de Bluetooth), ou BLE du téléphone désactivé |
| La passerelle PC ne trouve pas le nœud | Le téléphone est encore connecté à la carte |
