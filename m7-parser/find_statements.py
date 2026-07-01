"""
Findet automatisch alle Anweisungen (Statements) in einer C-Datei,
die als Coverage-Instrumentierungspunkt in Frage kommen.

Das ist der konkrete nächste Schritt Richtung automatischer
Instrumentierung (T7): statt wie in M2 von Hand main.c anzuschauen
und Zeilennummern zu schätzen, findet der Parser sie zuverlässig
selbst und liefert exakte Zeilennummern.
"""

from tree_sitter import Language, Parser
import tree_sitter_c

C_LANGUAGE = Language(tree_sitter_c.language())
parser = Parser(C_LANGUAGE)

with open("../m2-static-lib/main.c", "rb") as f:
    source_code = f.read()

tree = parser.parse(source_code)

# Welche Knoten-Typen gelten für uns als "Anweisung",
# an deren Anfang später ein cov_mark() eingefügt werden könnte.
STATEMENT_TYPES = {"expression_statement", "return_statement"}

def find_statements(node, current_function=None):
    """
    Geht rekursiv durch den Baum. Merkt sich, in welcher Funktion
    wir gerade sind (current_function), damit wir das gleich mit
    ausgeben können - genau wie in unserer probe_map.csv von M2.
    """
    if node.type == "function_definition":
        # Der Funktionsname steckt im "declarator"-Kind-Knoten
        for child in node.children:
            if child.type == "function_declarator":
                for sub in child.children:
                    if sub.type == "identifier":
                        current_function = source_code[sub.start_byte:sub.end_byte].decode()

    if node.type in STATEMENT_TYPES:
        # start_point ist ein (Zeile, Spalte)-Tupel, 0-indiziert -
        # daher +1, damit es mit normaler Editor-Zeilenzählung übereinstimmt
        line = node.start_point[0] + 1
        snippet = source_code[node.start_byte:node.end_byte].decode().strip()
        print(f"Zeile {line:3d}  [{current_function}]  {snippet}")

    for child in node.children:
        find_statements(child, current_function)

find_statements(tree.root_node)
