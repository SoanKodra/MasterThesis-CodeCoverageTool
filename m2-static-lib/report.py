"""
Host-Anwendung (Prototyp) für das Coverage-Werkzeug.

Führt zwei Datenquellen zusammen:
- covered_ids.txt: rohe, vom "Target" gemeldete Probe-IDs
  (simuliert hier den UART-Dump; auf echter Hardware kämen diese
  IDs über die serielle Schnittstelle statt aus einer Datei)
- probe_map.csv: Zuordnung ID -> Datei/Zeile/Funktion
  (wird später automatisch vom Parser aus M7 erzeugt;
  hier vorerst manuell angelegt)

Erzeugt daraus einen lesbaren Coverage-Report mit Prozentangabe.
Siehe Proposal, Abschnitt 10 / Abbildung 1 für das zugrunde liegende
Datenfluss-Konzept (Target kennt nur IDs, Host kennt die Zuordnung).
"""

import csv

# Schritt 1: Map laden (id -> file, line, function)
# csv.DictReader liest jede Zeile automatisch als Dictionary,
# mit den Spaltenüberschriften aus der ersten Zeile als Schlüssel.
probe_map = {}
with open("probe_map.csv") as f:
    reader = csv.DictReader(f)
    for row in reader:
        probe_map[int(row["id"])] = row

# Schritt 2: abgedeckte IDs vom "Target" laden
# Jede Zeile in covered_ids.txt ist eine einzelne, abgedeckte Probe-ID.
covered_ids = set()
with open("covered_ids.txt") as f:
    for line in f:
        line = line.strip()
        if line:
            covered_ids.add(int(line))

# Schritt 3: Report erzeugen
# Für jede bekannte Probe (aus der Map) wird geprüft, ob ihre ID
# in den abgedeckten IDs vom Target vorkommt.
print("Coverage Report")
print("=" * 50)
for probe_id, info in sorted(probe_map.items()):
    status = "COVERED" if probe_id in covered_ids else "NOT COVERED"
    print(f"{info['file']}:{info['line']} ({info['function']}) -> {status}")

total = len(probe_map)
covered = len(covered_ids)
print("=" * 50)
print(f"Coverage: {covered}/{total} ({100*covered/total:.1f}%)")
