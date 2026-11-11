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

bison -d parser.y || fail "bison falhou"
flex lexer.l || fail "flex falhou"
gcc -Wall -I. lex.yy.c testes/test_lexer.c -o "$tmp_dir/test_lexer" || fail "compilacao do harness falhou"

"$tmp_dir/test_lexer" testes/lexer_valid.in >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "entrada valida rejeitada"
diff -u testes/lexer_valid.expected "$tmp_dir/out" || fail "sequencia de tokens difere do esperado"
[ ! -s "$tmp_dir/err" ] || fail "entrada valida produziu diagnostico"
echo "[OK] palavras reservadas, identificadores, literais, operadores, pontuacao e comentarios"

check_invalid() {
    file=$1
    expected=$2
    if "$tmp_dir/test_lexer" "$file" >"$tmp_dir/out" 2>"$tmp_dir/err"; then
        fail "entrada invalida aceita: $file"
    fi
    grep -Fq "$expected" "$tmp_dir/err" || fail "diagnostico esperado nao encontrado: $expected"
}

printf '123abc\n' >"$tmp_dir/bad_identifier.in"
check_invalid "$tmp_dir/bad_identifier.in" "Identificador nao pode comecar com numero"
printf '@\n' >"$tmp_dir/bad_character.in"
check_invalid "$tmp_dir/bad_character.in" "Caractere invalido"
printf "'\\q'\n" >"$tmp_dir/bad_escape.in"
check_invalid "$tmp_dir/bad_escape.in" "Escape invalido em literal de caractere"
echo "[OK] identificador numerico, caractere desconhecido e escape invalido rejeitados"
