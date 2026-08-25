"""
Host-Anwendung (CLI): verbindet sich über UART mit dem Target, löst per
Host-Kommando ('D') den Coverage-Dump aus, empfängt die rohen Probe-IDs
und bildet sie über probe_map_auto.csv auf Datei/Zeile/Funktion zurück ab.
Entspricht der in Proposal Abschnitt 8.1 (funktionale Anforderung 5)
geforderten Host-Anwendung.

Funktioniert fuer beide Zielplattformen ueber Kommandozeilen-Parameter:
  AVR (Mega2560):    python3 uart_host_report.py --port /dev/ttyUSB0 --baud 9600
  STM32 (Nucleo):    python3 uart_host_report.py --port /dev/ttyACM0 --baud 115200
"""
import serial
import csv
import sys
import time
import argparse

parser = argparse.ArgumentParser(description="Coverage-Report ueber UART abrufen")
parser.add_argument("--port", default="/dev/ttyUSB0", help="Serieller Port (AVR: /dev/ttyUSB0, STM32: /dev/ttyACM0)")
parser.add_argument("--baud", type=int, default=9600, help="Baudrate (AVR: 9600, STM32: 115200)")
args = parser.parse_args()

SERIAL_PORT = args.port
BAUD_RATE = args.baud
PROBE_MAP_PATH = "../m7-parser/probe_map_auto.csv"


def load_probe_map(path):
    probes = {}
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            probes[int(row["id"])] = (row["file"], row["line"], row["function"])
    return probes


def request_dump(port, baud):
    ser = serial.Serial(port, baud, timeout=2)
    # Board resettet sich beim Öffnen der seriellen Verbindung (CH340-Verhalten
    # bei AVR, ST-LINK-Reconnect bei STM32) - kurze Pause nötig, bis die
    # Zielanwendung wieder läuft.
    time.sleep(2)

    ser.write(b"D")  # Host-Kommando auslösen

    covered_ids = []
    collecting = False
    start = time.time()

    while time.time() - start < 5:  # Timeout-Schutz, falls Board nicht antwortet
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue
        if "coverage dump start" in line:
            collecting = True
            continue
        if "coverage dump end" in line:
            break
        if collecting and line.isdigit():
            covered_ids.append(int(line))

    ser.close()
    return covered_ids


def print_report(covered_ids, probe_map):
    total = len(probe_map)
    covered = len(covered_ids)
    print(f"Coverage Report ({covered}/{total} probes, {covered/total*100:.1f}%)\n")

    for probe_id in sorted(probe_map.keys()):
        file, line, func = probe_map[probe_id]
        status = "COVERED    " if probe_id in covered_ids else "NOT COVERED"
        print(f"  [{status}] id={probe_id}  {file}:{line}  ({func})")


if __name__ == "__main__":
    probe_map = load_probe_map(PROBE_MAP_PATH)
    print(f"Verbinde mit {SERIAL_PORT} @ {BAUD_RATE} Baud, sende Dump-Kommando...")
    covered_ids = request_dump(SERIAL_PORT, BAUD_RATE)
    print_report(covered_ids, probe_map)
