#!/bin/sh
set -eu

tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

fail() {
    echo "[FALHA] $1" >&2
    [ ! -f "$tmp_dir/out" ] || cat "$tmp_dir/out" >&2
    [ ! -f "$tmp_dir/err" ] || cat "$tmp_dir/err" >&2
    exit 1
}

./parser < testes/ast_basica.txt >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "entrada AST valida terminou com erro"
[ ! -s "$tmp_dir/err" ] || fail "entrada AST valida produziu diagnostico"

# Compara a impressao da arvore, sem depender dos codigos numericos dos tokens
# usados para representar operadores.
sed -n '/=== ARVORE SINTATICA ABSTRATA (AST) ===/,/^=======================================/p' \
    "$tmp_dir/out" >"$tmp_dir/ast"
grep -Fq 'DECLARATION(x)' "$tmp_dir/ast" || fail "declaracao nao aparece na AST"
grep -Fq 'ASSIGN(x)' "$tmp_dir/ast" || fail "atribuicao nao aparece na AST"
grep -Fq 'NUM(1)' "$tmp_dir/ast" || fail "primeiro operando ausente na AST"
grep -Fq 'NUM(2)' "$tmp_dir/ast" || fail "segundo operando ausente na AST"
grep -Fq 'NUM(3)' "$tmp_dir/ast" || fail "terceiro operando ausente na AST"
[ "$(grep -c 'OP(' "$tmp_dir/ast")" -eq 2 ] || fail "AST deveria conter dois nos de operador"

# A precedencia deve agrupar 2 * 3 sob o operador da soma.
awk '
    /^ASSIGN\(x\)$/ { in_assign = 1; next }
    in_assign && /^  OP\(/ { sum = NR; next }
    in_assign && /^    NUM\(1\)$/ { left = NR; next }
    in_assign && /^    OP\(/ { product = NR; next }
    in_assign && /^      NUM\(2\)$/ { two = NR; next }
    in_assign && /^      NUM\(3\)$/ { three = NR; next }
    END { exit !(sum && left && product && two && three && sum < left && left < product && product < two && two < three) }
' "$tmp_dir/ast" || fail "precedencia de multiplicacao e soma incorreta na AST"

echo "[OK] nos da AST e precedencia de operadores"
