"""
Erster Test von tree-sitter: liest eine C-Datei ein und zeigt
ihre Baumstruktur (AST) an.

Ziel: verstehen, wie tree-sitter Quellcode strukturiert erkennt -
Grundlage für die spätere automatische Instrumentierung (M7).
"""

from tree_sitter import Language, Parser
import tree_sitter_c

# Die C-Grammatik laden und einen Parser dafür einrichten
C_LANGUAGE = Language(tree_sitter_c.language())
parser = Parser(C_LANGUAGE)

# Unsere main.c aus dem M2-Ordner einlesen
with open("../m2-static-lib/main.c", "rb") as f:
    source_code = f.read()

# Parsen: erzeugt den Syntaxbaum
tree = parser.parse(source_code)

def print_tree(node, indent=0):
    """Gibt den Baum rekursiv aus, mit Einrückung pro Ebene."""
    text_snippet = source_code[node.start_byte:node.end_byte][:40]
    print("  " * indent + f"{node.type}  ({text_snippet!r})")
    for child in node.children:
        print_tree(child, indent + 1)

print_tree(tree.root_node)
