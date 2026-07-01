"""
Automatische Instrumentierung: liest raw_main.c (unberührten Zielcode),
fügt vor jeder gefundenen Anweisung einen cov_mark(id)-Aufruf ein,
und erzeugt gleichzeitig die passende probe_map.csv.

Löst die manuelle Instrumentierung aus M2 vollständig ab.
"""

from tree_sitter import Language, Parser
import tree_sitter_c
import csv

C_LANGUAGE = Language(tree_sitter_c.language())
parser = Parser(C_LANGUAGE)

with open("raw_main.c", "rb") as f:
    source_code = f.read()

tree = parser.parse(source_code)

STATEMENT_TYPES = {"expression_statement", "return_statement"}

# Sammelt (start_byte, Zeile, Funktion, Snippet) für jede gefundene Anweisung
found_statements = []

def collect_statements(node, current_function=None):
    if node.type == "function_definition":
        for child in node.children:
            if child.type == "function_declarator":
                for sub in child.children:
                    if sub.type == "identifier":
                        current_function = source_code[sub.start_byte:sub.end_byte].decode()

    if node.type in STATEMENT_TYPES:
        line = node.start_point[0] + 1
        snippet = source_code[node.start_byte:node.end_byte].decode().strip()
        found_statements.append((node.start_byte, line, current_function, snippet))

    for child in node.children:
        collect_statements(child, current_function)

collect_statements(tree.root_node)

# IDs vergeben in der Reihenfolge, wie die Anweisungen im Quellcode vorkommen
# (also von vorne nach hinten - nur das EINFÜGEN passiert gleich rückwärts)
found_statements.sort(key=lambda s: s[0])

# probe_map.csv automatisch erzeugen
with open("probe_map_auto.csv", "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["id", "file", "line", "function"])
    for probe_id, (start_byte, line, func, snippet) in enumerate(found_statements):
        writer.writerow([probe_id, "raw_main.c", line, func])

# Instrumentierten Quellcode erzeugen: von HINTEN nach VORNE einfügen,
# damit frühere Byte-Positionen nicht verschoben werden
instrumented = bytearray(source_code)
for probe_id, (start_byte, line, func, snippet) in reversed(list(enumerate(found_statements))):
    insertion = f"cov_mark({probe_id}); ".encode()
    instrumented[start_byte:start_byte] = insertion

# Header-Include am Dateianfang ergänzen
final_code = b'#include "coverage.h"\n\n' + bytes(instrumented)

with open("instrumented_main.c", "wb") as f:
    f.write(final_code)

print(f"{len(found_statements)} Probes eingefügt.")
print("Erzeugt: instrumented_main.c, probe_map_auto.csv")
