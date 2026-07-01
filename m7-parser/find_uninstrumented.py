"""
Unterscheidet bei jeder gefundenen Anweisung:
- ist es bereits ein cov_mark()-Aufruf selbst (schon instrumentiert)?
- oder ist es eine "normale" Anweisung, die noch keine Probe hat?

Das ist der Schritt direkt vor der eigentlichen Instrumentierung:
erst wissen, WELCHE Stellen überhaupt noch eine cov_mark()-Probe
brauchen, bevor man anfängt, Text einzufügen.
"""

from tree_sitter import Language, Parser
import tree_sitter_c

C_LANGUAGE = Language(tree_sitter_c.language())
parser = Parser(C_LANGUAGE)

with open("../m2-static-lib/main.c", "rb") as f:
    source_code = f.read()

tree = parser.parse(source_code)

STATEMENT_TYPES = {"expression_statement", "return_statement"}

def is_cov_mark_call(node):
    """
    Prüft, ob ein expression_statement-Knoten bereits ein
    Aufruf von cov_mark(...) ist.
    Baumstruktur dafür: expression_statement -> call_expression -> identifier
    """
    if node.type != "expression_statement":
        return False
    for child in node.children:
        if child.type == "call_expression":
            for sub in child.children:
                if sub.type == "identifier":
                    name = source_code[sub.start_byte:sub.end_byte].decode()
                    return name == "cov_mark"
    return False

def find_statements(node, current_function=None):
    if node.type == "function_definition":
        for child in node.children:
            if child.type == "function_declarator":
                for sub in child.children:
                    if sub.type == "identifier":
                        current_function = source_code[sub.start_byte:sub.end_byte].decode()

    if node.type in STATEMENT_TYPES:
        line = node.start_point[0] + 1
        snippet = source_code[node.start_byte:node.end_byte].decode().strip()

        if is_cov_mark_call(node):
            status = "bereits instrumentiert"
        else:
            status = "BENÖTIGT PROBE"

        print(f"Zeile {line:3d}  [{current_function:16s}]  {status:22s}  {snippet}")

    for child in node.children:
        find_statements(child, current_function)

find_statements(tree.root_node)
