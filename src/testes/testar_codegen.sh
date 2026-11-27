#!/bin/sh
# Testes do gerador de codigo final: o Assembly de cada programa de
# testes/codegen/*.txt deve ser identico ao de testes/codegen/*.expected.
# Complementa os testes de execucao (testar_execucao.sh), que conferem o
# comportamento: aqui se fixa a forma do codigo (selecao de instrucoes, quadro
# de pilha, secao de dados). Se uma mudanca intencional alterar o Assembly,
# confira o diff e regenere o .expected, por exemplo:
#   ./parser < testes/codegen/nome.txt 2>/dev/null \
#     | sed -n '/^=== CODIGO FINAL (ASSEMBLY RISC-V) ===$/,/^======================================$/p' \
#     > testes/codegen/nome.expected
set -eu

tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

fail() {
    echo "[FALHA] $1" >&2
    [ ! -f "$tmp_dir/diff" ] || cat "$tmp_dir/diff" >&2
    [ ! -f "$tmp_dir/err" ] || cat "$tmp_dir/err" >&2
    exit 1
}

for entrada in testes/codegen/*.txt; do
    nome=$(basename "$entrada" .txt)
    ./parser <"$entrada" >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "$nome: terminou com erro"
    [ ! -s "$tmp_dir/err" ] || fail "$nome: programa valido produziu diagnostico"
    sed -n '/^=== CODIGO FINAL (ASSEMBLY RISC-V) ===$/,/^======================================$/p' \
        "$tmp_dir/out" >"$tmp_dir/asm"
    [ -s "$tmp_dir/asm" ] || fail "$nome: nenhum Assembly gerado"
    diff -u "testes/codegen/$nome.expected" "$tmp_dir/asm" >"$tmp_dir/diff" || fail "$nome: Assembly diferente do esperado"
    echo "[OK] codegen: $nome"
done

# Sem main nao ha ponto de entrada: o gerador nao inventa um _start.
./parser <testes/codegen/operadores.txt >"$tmp_dir/out" 2>/dev/null
if grep -Fq '_start:' "$tmp_dir/out"; then fail "programa sem main gerou _start"; fi
echo "[OK] codegen: sem main nao ha _start"

echo "Bateria do gerador de codigo concluida."
