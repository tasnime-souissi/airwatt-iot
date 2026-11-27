"""Génère l'estimation de prix AirWatt (estimation-prix.xlsx).

Les prix sont saisis une seule fois dans PRIX ci-dessous (relevés en ligne, avec lien et
date). Les totaux sont des formules Excel : modifier un prix dans le fichier Excel met
automatiquement les totaux à jour.

Usage : python generer_estimation.py
"""
from pathlib import Path

from openpyxl import Workbook
from openpyxl.styles import Alignment, Border, Font, PatternFill, Side
from openpyxl.utils import get_column_letter

DATE_RELEVE = "27/11/2026"

# clé -> (désignation, référence conseillée, prix unitaire TND, boutique, lien, remarque)
PRIX: dict[str, tuple] = {}  # rempli à partir des relevés (voir prix_releves.py)

try:
    from prix_releves import PRIX as PRIX_RELEVES
    PRIX.update(PRIX_RELEVES)
except ImportError:
    pass

# Nomenclature du prototype (montage complet du nœud, passerelle = PC)
PROTOTYPE = [
    ("U1", "esp32", 1), ("U2", "pms5003", 1), ("U3", "bme280", 1), ("Q2", "mosfet", 1), ("D1", "diode", 1),
    ("M1", "ventilateur", 1), ("PS1", "adaptateur12v", 1), ("U4", "lm2596", 1), ("LED1", "led_rgb", 1),
    ("BZ1", "buzzer", 1), ("Q1", "transistor", 1), ("R1-R6", "resistances", 1), ("C1", "condensateur", 1),
    ("", "plaque_fils", 1), ("", "boitier", 1),
]

# Déploiement dans un atelier de 3 zones (mélange, vulcanisation, meulage)
DEPLOIEMENT = [
    ("Nœuds (× 3)", "esp32", 3), ("", "pms5003", 3), ("", "bme280", 3), ("", "boitier_ip", 3),
    ("", "lm2596", 3), ("", "adaptateur12v", 3), ("", "condensateur", 3),
    ("Passerelle", "esp32", 1), ("", "boitier", 1), ("", "adaptateur12v", 1), ("", "lm2596", 1),
]

BLEU, GRIS = "0D47A1", "F2F4F3"
fin = Side(style="thin", color="B0BEC5")
bord = Border(left=fin, right=fin, top=fin, bottom=fin)


def feuille(wb, titre, lignes, intro):
    ws = wb.create_sheet(titre)
    ws["A1"] = f"AirWatt : estimation de prix ({titre.lower()})"
    ws["A1"].font = Font(size=14, bold=True, color=BLEU)
    ws["A2"] = intro
    ws["A2"].font = Font(italic=True, color="455A64")
    entetes = ["Repère", "Désignation", "Référence", "Qté", "Prix unitaire (TND)", "Total (TND)",
               "Boutique", "Lien", "Relevé le", "Remarque"]
    for c, h in enumerate(entetes, 1):
        cell = ws.cell(row=4, column=c, value=h)
        cell.font = Font(bold=True, color="FFFFFF")
        cell.fill = PatternFill("solid", fgColor=BLEU)
        cell.alignment = Alignment(horizontal="center", vertical="center", wrap_text=True)
        cell.border = bord
    r = 5
    for repere, cle, qte in lignes:
        des, ref, pu, boutique, lien, rem = PRIX.get(cle, (cle, "", None, "", "", "Prix à relever"))
        valeurs = [repere, des, ref, qte, pu, f"=D{r}*E{r}", boutique, lien, DATE_RELEVE if pu else "", rem]
        for c, v in enumerate(valeurs, 1):
            cell = ws.cell(row=r, column=c, value=v)
            cell.border = bord
            cell.alignment = Alignment(vertical="center", wrap_text=c in (2, 3, 10))
            if r % 2 == 0:
                cell.fill = PatternFill("solid", fgColor=GRIS)
        ws.cell(row=r, column=5).number_format = "0.000"
        ws.cell(row=r, column=6).number_format = "0.000"
        if lien:
            ws.cell(row=r, column=8).hyperlink = lien
            ws.cell(row=r, column=8).font = Font(color="1565C0", underline="single")
        r += 1
    ws.cell(row=r, column=5, value="TOTAL TTC").font = Font(bold=True)
    tot = ws.cell(row=r, column=6, value=f"=SUM(F5:F{r-1})")
    tot.font = Font(bold=True, color=BLEU, size=12)
    tot.number_format = "0.000"
    for c, w in enumerate([10, 34, 26, 6, 14, 13, 18, 48, 11, 40], 1):
        ws.column_dimensions[get_column_letter(c)].width = w
    ws.freeze_panes = "A5"
    return ws, r


wb = Workbook()
wb.remove(wb.active)
wb.properties.creator = "Équipe AirWatt (3 SETP, ENSTAB)"
wb.properties.lastModifiedBy = "Équipe AirWatt (3 SETP, ENSTAB)"
wb.properties.title = "Estimation de prix du système AirWatt"
feuille(wb, "Prototype", PROTOTYPE,
        "Montage complet d'un nœud, conforme au schéma de câblage. La passerelle est un ordinateur "
        "portable déjà disponible (0 TND). Prix relevés le 27/11/2026.")
feuille(wb, "Déploiement atelier", DEPLOIEMENT,
        "3 nœuds (zones de mélange, de vulcanisation et de meulage) et 1 passerelle ESP32. "
        "Variateur de l'extracteur et installation électrique non compris.")
sortie = Path(__file__).with_name("estimation-prix.xlsx")
wb.save(sortie)
print(f"{sortie.name} généré ({len(PRIX)} prix renseignés)")
