"""Prix relevés le 27/11/2026 sur les pages produits (prix affichés, en TND).

Didactico (didactico.tn, Sfax) : prix affichés avec la remise « White Friday » de 5 %
valable du 27 au 29/11/2026 ; prix habituel ≈ prix affiché / 0,95.
Electronic Shop (es-online.tn) : deuxième source.

clé -> (désignation, référence, prix unitaire TND, boutique, lien, remarque)
"""

PRIX = {
    "esp32": ("Carte ESP32 DevKit (ESP32-WROOM-32, BLE intégré)", "ESP32-WROOM-32 CH340, 38 broches",
              30.875, "Didactico", "https://didactico.tn/produit/carte-developpement-esp32-wroom-32/",
              "En stock. Autre source : es-online 32 à 34 TND"),
    "pms5003": ("Capteur de particules laser PM1 / PM2.5 / PM10", "Plantower PMS5003 + câble",
                90.000, "Import (vendeurs en ligne internationaux)", "https://www.ebay.com/itm/405136238957",
                "Introuvable en Tunisie. ≈ 23 USD ≈ 69 TND + ≈ 30 % de port et douane (estimation à confirmer). "
                "Option locale moins précise : DSM501A, 31,350 TND chez Didactico"),
    "bme280": ("Capteur température / humidité / pression (I2C)", "GY-BME280 3,3 V",
               19.000, "Didactico", "https://didactico.tn/produit/capteur-pression-atmospherique-gy-bme280/",
               "En stock. Vérifier l'identifiant 0x60 à réception (pas un BMP280)"),
    "mosfet": ("MOSFET canal N à commande logique", "IRL540N",
               4.500, "Electronic Shop", "https://es-online.tn/produit/mosfet-de-puissance-canal-n",
               "Rupture de stock au relevé. IRLZ44N introuvable ; ne pas prendre l'IRF520 (grille 10 V)"),
    "diode": ("Diode de roue libre", "1N4007",
              0.285, "Didactico", "https://didactico.tn/produit/diode-schottky-1n4007/", "En stock"),
    "ventilateur": ("Ventilateur 12 V (modèle réduit d'extracteur)", "6015, 60 × 60 × 15 mm, 12 V DC",
                    14.000, "Didactico", "https://didactico.tn/produit/ventilateur-refroidissement-6015/",
                    "Sur commande. Pas de 80 mm en 12 V trouvé. Alternative en stock : 40 mm 0,1 A, 8 TND (es-online)"),
    "adaptateur12v": ("Adaptateur secteur 12 V / 2 A", "AC/DC 12 V 2 A, fiche jack",
                      13.300, "Didactico", "https://didactico.tn/produit/adaptateur-12v-2a-arduino/",
                      "En stock. Autre source : es-online 15 TND"),
    "lm2596": ("Abaisseur de tension DC-DC 12 V → 5 V", "Module LM2596 3 A",
               22.800, "Didactico", "https://didactico.tn/produit/module-lm2596/",
               "En stock. Version LM2596S à 5 TND sur commande"),
    "led_rgb": ("LED RGB à cathode commune", "LED RGB 4 broches",
                0.665, "Didactico", "https://didactico.tn/produit/led-rgb-cathode-commune-4pin/", "En stock"),
    "buzzer": ("Buzzer actif 5 V", "Buzzer actif 5 V",
               1.425, "Didactico", "https://didactico.tn/produit/capteur-buzzer-actif-5v/", "En stock"),
    "transistor": ("Transistor NPN (commande du buzzer)", "2N2222 TO-92",
                   0.285, "Didactico", "https://didactico.tn/produit/transistor-npn-2n2222/", "En stock"),
    "resistances": ("Résistances 1/4 W (220 Ω, 100 Ω × 2, 1 kΩ, 10 kΩ, 220 Ω)", "6 pièces",
                    1.200, "Didactico", "https://didactico.tn/produit/resistance-en-ohm-220/",
                    "6 × 0,200 TND au plus (220 Ω à 0,095 TND)"),
    "condensateur": ("Condensateur électrolytique", "470 µF 25 V",
                     0.380, "Didactico", "https://didactico.tn/produit/condensateur-chimique-470uf-25v/", "En stock"),
    "plaque_fils": ("Plaque d'essai 830 points + 65 fils Dupont", "Kit de prototypage",
                    14.000, "Electronic Shop", "https://es-online.tn/produit/breadboard-830-points-65-fils-dupont",
                    "En stock (prototype uniquement)"),
    "boitier": ("Boîtier plastique", "21 × 13 × 11 cm",
                8.550, "Didactico", "https://didactico.tn/produit/boitier-plastique-mm-21x13x11/", "En stock"),
    "boitier_ip": ("Boîte de jonction étanche (atelier)", "CNP-3103",
                   13.201, "Didactico", "https://didactico.tn/produit/boite-de-jonction-etanche-cnp-3103/",
                   "Sur commande. Protège de la poussière de l'atelier"),
}
