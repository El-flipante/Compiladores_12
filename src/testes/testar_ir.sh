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

# Extrai somente o bloco de codigo intermediario da saida do compilador.
extrai_ir() {
    sed -n '/=== CODIGO INTERMEDIARIO (TRES ENDERECOS) ===/,/^=============================================/p' "$1"
}

# Programas validos: o IR gerado deve ser identico ao esperado (testes/ir/*.expected)
for entrada in testes/ir/*.txt; do
    nome=$(basename "$entrada" .txt)
    [ "$nome" != "erro_semantico" ] || continue
    ./parser < "$entrada" >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "$nome: terminou com erro"
    [ ! -s "$tmp_dir/err" ] || fail "$nome: programa valido produziu diagnostico"
    extrai_ir "$tmp_dir/out" >"$tmp_dir/ir"
    [ -s "$tmp_dir/ir" ] || fail "$nome: nenhum codigo intermediario gerado"
    diff -u "testes/ir/$nome.expected" "$tmp_dir/ir" >"$tmp_dir/diff" || { cat "$tmp_dir/diff" >&2; fail "$nome: IR diferente do esperado"; }
    echo "[OK] ir: $nome"
done

# Entrada com erro semantico: nao deve gerar codigo intermediario
./parser < testes/ir/erro_semantico.txt >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "erro semantico: terminou com erro de processo"
extrai_ir "$tmp_dir/out" >"$tmp_dir/ir"
[ ! -s "$tmp_dir/ir" ] || fail "erro semantico: gerou codigo intermediario"
grep -Fq "Codigo intermediario nao gerado" "$tmp_dir/err" || fail "erro semantico: faltou o aviso de que o IR nao foi gerado"
echo "[OK] ir: nao gera codigo quando ha erros"

# Comando fora de funcao nao faz parte da linguagem: e ignorado e sinalizado no IR
./parser < testes/ast_basica.txt >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "comando global: terminou com erro"
grep -Fq "global x : int" "$tmp_dir/out" || fail "comando global: declaracao global ausente"
grep -Fq "; AVISO: comando fora de funcao" "$tmp_dir/out" || fail "comando global: aviso ausente no IR"
echo "[OK] ir: comando fora de funcao e ignorado com aviso"

echo "Bateria de codigo intermediario concluida."
