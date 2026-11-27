"""Génère le schéma de câblage du nœud AirWatt (montage complet) en SVG.

Usage : python generer_schema.py   -> schema-cablage.svg (même dossier)
Les liaisons sont indiquées par des étiquettes de réseau (+12V, +5V, GPIO25...) :
deux points portant la même étiquette sont reliés électriquement.
"""
from pathlib import Path

W, H = 1600, 1180
C12, C5, C33, CGND, CSIG = "#c62828", "#e65100", "#b8860b", "#212121", "#1565c0"
NET_COLORS = {"+12V": C12, "+5V": C5, "+3V3": C33, "GND": CGND}
out = []


def add(s):
    out.append(s)


def text(x, y, s, size=14, anchor="start", weight="normal", color="#212121", style="normal"):
    s = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    add(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" '
        f'font-style="{style}" fill="{color}">{s}</text>')


def line(x1, y1, x2, y2, color=CGND, w=2, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    add(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{w}"{d}/>')


def dot(x, y, color=CGND):
    add(f'<circle cx="{x}" cy="{y}" r="4" fill="{color}"/>')


def net_color(net):
    return NET_COLORS.get(net, CSIG)


def net_label(x, y, net, side="right"):
    """Étiquette de réseau en forme de drapeau, accrochée au point (x, y)."""
    c = net_color(net)
    w = 9 * len(net) + 18
    if side == "right":
        pts = f"{x},{y} {x+10},{y-11} {x+w},{y-11} {x+w},{y+11} {x+10},{y+11}"
        tx = x + 10 + (w - 10) / 2
    else:
        pts = f"{x},{y} {x-10},{y-11} {x-w},{y-11} {x-w},{y+11} {x-10},{y+11}"
        tx = x - 10 - (w - 10) / 2
    add(f'<polygon points="{pts}" fill="white" stroke="{c}" stroke-width="2"/>')
    text(tx, y + 5, net, 13, "middle", "bold", c)


def section(x, y, w, h, title):
    add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" fill="#fafafa" stroke="#9e9e9e" '
        f'stroke-width="1.5" stroke-dasharray="8 5"/>')
    text(x + 14, y + 26, title, 17, weight="bold", color="#37474f")


def block(x, y, w, h, title, sub=None):
    add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" fill="#e3f2fd" stroke="#0d47a1" stroke-width="2"/>')
    text(x + w / 2, y + 24, title, 16, "middle", "bold", "#0d47a1")
    if sub:
        text(x + w / 2, y + 42, sub, 12, "middle", color="#455a64")


def pin(x, y, side, name, net=None, length=55, nc=False):
    """Broche d'un bloc : nom à l'intérieur, fil vers l'extérieur, étiquette de réseau."""
    c = net_color(net) if net else "#757575"
    x2 = x + length if side == "right" else x - length
    line(x, y, x2, y, c)
    if side == "right":
        text(x - 8, y + 5, name, 13, "end")
    else:
        text(x + 8, y + 5, name, 13, "start")
    if nc:
        text(x2 + (6 if side == "right" else -6), y + 5, "non connecté", 12,
             "start" if side == "right" else "end", color="#757575", style="italic")
    elif net:
        net_label(x2, y, net, side)


def resistor_h(x, y, label, value):
    """Résistance horizontale (norme IEC) entre x et x+70."""
    line(x, y, x + 15, y)
    add(f'<rect x="{x+15}" y="{y-8}" width="40" height="16" fill="white" stroke="{CGND}" stroke-width="2"/>')
    line(x + 55, y, x + 70, y)
    text(x + 35, y - 14, f"{label} {value}", 12, "middle")


def resistor_v(x, y, label, value):
    """Résistance verticale entre y et y+70."""
    line(x, y, x, y + 15)
    add(f'<rect x="{x-8}" y="{y+15}" width="16" height="40" fill="white" stroke="{CGND}" stroke-width="2"/>')
    line(x, y + 55, x, y + 70)
    text(x + 14, y + 40, f"{label} {value}", 12)


def gnd(x, y):
    line(x, y, x, y + 10)
    for i, half in enumerate((14, 9, 4)):
        line(x - half, y + 10 + i * 5, x + half, y + 10 + i * 5)


def rail(x, y, net):
    c = net_color(net)
    line(x - 16, y, x + 16, y, c, 3)
    text(x, y - 8, net, 13, "middle", "bold", c)


add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" '
    f'font-family="Segoe UI, Roboto, Arial, sans-serif">')
add(f'<rect width="{W}" height="{H}" fill="white"/>')
text(30, 44, "AirWatt — Schéma de câblage du nœud capteur (montage complet)", 26, weight="bold", color="#0d47a1")
text(30, 70, "Projet IoT SETP · technologie imposée : BLE · deux points portant la même étiquette "
     "(+12V, +5V, GPIO25…) sont reliés · masse (GND) commune à tous les éléments", 14, color="#455a64")

# ---- Alimentation --------------------------------------------------------------
section(30, 95, 370, 470, "1. Alimentation")
block(60, 140, 150, 90, "Adaptateur", "secteur → 12 V / 2 A")
pin(210, 180, "right", "+", "+12V")
pin(210, 205, "right", "−", "GND")
block(130, 270, 140, 160, "LM2596", "abaisseur DC-DC")
pin(130, 345, "left", "IN+", "+12V", length=25)
pin(130, 395, "left", "IN−", "GND", length=25)
pin(270, 345, "right", "OUT+", "+5V", length=35)
pin(270, 395, "right", "OUT−", "GND", length=35)
text(200, 452, "Sortie réglée à 5,0 V", 12, "middle", style="italic", color="#455a64")
text(200, 468, "(potentiomètre, au multimètre", 12, "middle", style="italic", color="#455a64")
text(200, 484, "avant de brancher l'ESP32)", 12, "middle", style="italic", color="#455a64")
# Condensateur de découplage sur le +5V
rail(320, 470, "+5V")
line(320, 470, 320, 500, C5)
line(304, 500, 336, 500, CGND, 3)
add(f'<path d="M304 510 Q320 500 336 510" fill="none" stroke="{CGND}" stroke-width="3"/>')
line(320, 508, 320, 530)
gnd(320, 530)
text(345, 500, "C1", 12, weight="bold")
text(345, 516, "470 µF / 25 V", 12)

# ---- Capteurs ------------------------------------------------------------------
section(30, 590, 370, 560, "2. Capteurs")
block(60, 630, 150, 245, "PMS5003", "particules (UART)")
for i, (name, net, nc) in enumerate([("1 VCC", "+5V", False), ("2 GND", "GND", False), ("3 SET", None, True),
                                       ("4 RX", "GPIO17", False), ("5 TX", "GPIO16", False),
                                       ("6 RESET", None, True)]):
    pin(210, 700 + i * 30, "right", name, net, nc=nc)
text(135, 895, "Alimentation 5 V, logique 3,3 V", 12, "middle", style="italic", color="#455a64")
text(135, 911, "(compatible ESP32, sans adaptateur)", 12, "middle", style="italic", color="#455a64")
block(60, 935, 150, 180, "BME280", "temp. / humidité (I2C)")
for i, (name, net) in enumerate([("VIN", "+3V3"), ("GND", "GND"), ("SCL", "GPIO22"), ("SDA", "GPIO21")]):
    pin(210, 1000 + i * 30, "right", name, net)
text(135, 1137, "Adresse I2C 0x76 (ou 0x77)", 12, "middle", style="italic", color="#455a64")

# ---- Microcontrôleur -----------------------------------------------------------
section(430, 95, 520, 1055, "3. Microcontrôleur et communication")
block(600, 150, 190, 560, "ESP32 DevKit V1", "ESP32-WROOM-32")
left = [("VIN", "+5V"), ("GND", "GND"), ("3V3", "+3V3"), ("GPIO16 (RX2)", "GPIO16"), ("GPIO17 (TX2)", "GPIO17"),
        ("GPIO21 (SDA)", "GPIO21"), ("GPIO22 (SCL)", "GPIO22")]
for i, (name, net) in enumerate(left):
    pin(600, 220 + i * 55, "left", name, net)
right = [("GPIO25", "GPIO25"), ("GPIO26", "GPIO26"), ("GPIO27", "GPIO27"), ("GPIO32", "GPIO32"),
         ("GPIO33", "GPIO33")]
for i, (name, net) in enumerate(right):
    pin(790, 220 + i * 55, "right", name, net)
line(615, 600, 775, 600, "#90caf9", 1)
text(695, 625, "Sur la carte :", 13, "middle", "bold", "#37474f")
text(695, 645, "GPIO2 = LED intégrée", 12, "middle", color="#37474f")
text(695, 663, "GPIO0 = bouton BOOT", 12, "middle", color="#37474f")
text(695, 681, "micro-USB = programmation", 12, "middle", color="#37474f")
# Encadré BLE
add('<rect x="470" y="745" width="440" height="190" rx="8" fill="#e8f5e9" stroke="#2e7d32" stroke-width="2"/>')
text(690, 775, "Module de communication : BLE intégré", 16, "middle", "bold", "#2e7d32")
for i, s in enumerate(["Radio Bluetooth Low Energy 2,4 GHz de l'ESP32-WROOM-32,",
                        "antenne gravée sur le circuit : aucun module externe à câbler.",
                        "Service GATT « AirWatt » : mesures (NOTIFY, 12 octets / 2 s)",
                        "et commandes (WRITE, 3 octets) vers la passerelle,",
                        "puis Wi-Fi / MQTT vers ThingsBoard (second saut libre).",
                        "Garder l'antenne loin du métal et du ventilateur."]):
    text(690, 802 + i * 21, s, 13, "middle", color="#1b5e20")
add('<rect x="470" y="960" width="440" height="170" rx="8" fill="#fff8e1" stroke="#f9a825" stroke-width="2"/>')
text(690, 990, "Règles de câblage", 16, "middle", "bold", "#8d6e00")
for i, s in enumerate(["Masse commune : GND de l'alimentation, du LM2596, de l'ESP32,",
                        "des capteurs et du MOSFET reliés ensemble.",
                        "Ne jamais relier le +12 V à une broche de l'ESP32.",
                        "Courant max par broche GPIO : 20 mA (d'où Q1 pour le buzzer).",
                        "Fils I2C et UART courts (< 30 cm)."]):
    text(690, 1017 + i * 21, s, 13, "middle", color="#5d4037")

# ---- Ventilation (actionneur) --------------------------------------------------
section(980, 95, 590, 470, "4. Ventilation (actionneur)")
rail(1250, 150, "+12V")
line(1250, 150, 1250, 200, C12)
dot(1250, 200, C12)
# Ventilateur (moteur)
line(1250, 200, 1250, 235, C12)
add(f'<circle cx="1250" cy="270" r="35" fill="white" stroke="{CGND}" stroke-width="2"/>')
text(1250, 278, "M", 24, "middle", "bold")
text(1300, 262, "Ventilateur 12 V", 13, weight="bold")
text(1300, 280, "60 mm, 0,1 à 0,15 A", 12)
line(1250, 305, 1250, 340)
dot(1250, 340)
# Diode de roue libre en parallèle (cathode vers +12 V)
line(1250, 200, 1150, 200, C12)
line(1150, 200, 1150, 245, C12)
add(f'<polygon points="1136,290 1164,290 1150,262" fill="white" stroke="{CGND}" stroke-width="2"/>')
line(1136, 262, 1164, 262, CGND, 3)
line(1150, 245, 1150, 262)
line(1150, 290, 1150, 340)
line(1150, 340, 1250, 340)
text(1126, 270, "D1", 12, "end", "bold")
text(1126, 286, "1N4007", 12, "end")
text(1126, 302, "roue libre", 12, "end", style="italic", color="#455a64")
# MOSFET (commutation côté masse)
line(1250, 340, 1250, 380)
block(1200, 380, 100, 90, "Q2", "IRL540N")
text(1250, 456, "MOSFET N", 12, "middle", color="#455a64")
text(1260, 378, "D", 12, weight="bold")
line(1250, 470, 1250, 500)
text(1260, 492, "S", 12, weight="bold")
gnd(1250, 500)
# Grille
line(1200, 425, 1130, 425)
text(1188, 418, "G", 12, "end", "bold")
dot(1130, 425)
resistor_h(1060, 425, "R1", "220 Ω")
net_label(1060, 425, "GPIO25", "left")
line(1130, 425, 1130, 450)
resistor_v(1130, 450, "R2", "10 kΩ")
gnd(1130, 520)
text(1350, 400, "PWM 25 kHz sur la grille :", 12, color="#455a64")
text(1350, 418, "vitesse du ventilateur 0 à 100 %", 12, color="#455a64")
text(1350, 436, "R2 bloque Q2 au démarrage", 12, color="#455a64")
text(1350, 454, "(ventilateur arrêté tant que", 12, color="#455a64")
text(1350, 472, "l'ESP32 n'a pas démarré)", 12, color="#455a64")

# ---- Signalisation locale ------------------------------------------------------
section(980, 590, 590, 560, "5. Signalisation locale")
# LED RGB à cathode commune
text(1050, 640, "LED RGB à cathode commune", 14, weight="bold")
for i, (net, col, lab, val) in enumerate([("GPIO26", "#d32f2f", "R", "220 Ω"), ("GPIO27", "#388e3c", "G", "100 Ω"),
                                           ("GPIO32", "#1976d2", "B", "100 Ω")]):
    y = 680 + i * 55
    net_label(1080, y, net, "left")
    resistor_h(1080, y, f"R{3+i}", val)
    line(1150, y, 1190, y)
    add(f'<polygon points="1190,{y-12} 1190,{y+12} 1214,{y}" fill="{col}" fill-opacity="0.25" '
        f'stroke="{col}" stroke-width="2"/>')
    line(1214, y - 12, 1214, y + 12, col, 3)
    text(1202, y - 16, lab, 12, "middle", "bold", col)
    line(1214, y, 1260, y)
line(1260, 680, 1260, 810)
dot(1260, 735)
gnd(1260, 810)
text(1280, 740, "Vert = bon, orange = moyen (R+G),", 12, color="#455a64")
text(1280, 758, "rouge = mauvais, bleu = capteur en défaut", 12, color="#455a64")
# Buzzer piloté par transistor
text(1050, 880, "Buzzer actif 5 V (via transistor)", 14, weight="bold")
rail(1330, 900, "+5V")
line(1330, 900, 1330, 925, C5)
add('<rect x="1300" y="925" width="60" height="40" rx="6" fill="white" stroke="#212121" stroke-width="2"/>')
text(1330, 950, "BZ1", 13, "middle", "bold")
text(1370, 940, "+ en haut", 12, color="#455a64")
line(1330, 965, 1330, 995)
block(1280, 995, 100, 80, "Q1", "2N2222")
text(1340, 993, "C", 12, weight="bold")
line(1330, 1075, 1330, 1100)
text(1340, 1092, "E", 12, weight="bold")
gnd(1330, 1100)
line(1280, 1035, 1200, 1035)
text(1272, 1028, "B", 12, "end", "bold")
resistor_h(1130, 1035, "R6", "1 kΩ")
line(1200, 1035, 1200, 1035)
net_label(1130, 1035, "GPIO33", "left")

# ---- Légende -------------------------------------------------------------------
y0 = 1168
for i, (lab, c) in enumerate([("+12 V", C12), ("+5 V", C5), ("+3,3 V", C33), ("Masse GND", CGND),
                              ("Signal GPIO", CSIG)]):
    x = 30 + i * 170
    line(x, y0 - 5, x + 30, y0 - 5, c, 4)
    text(x + 38, y0, lab, 13)
text(1570, y0, "AirWatt · v1.0 · 27/11/2026", 13, "end", color="#757575")
add("</svg>")

Path(__file__).with_name("schema-cablage.svg").write_text("\n".join(out), encoding="utf-8")
print("schema-cablage.svg généré")
