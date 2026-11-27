#!/bin/sh
# Testes de execucao: compila cada programa de testes/exec/*.txt com o proprio
# compilador, executa o Assembly RISC-V gerado no simulador testes/rv32sim.c e
# compara o valor devolvido por main() com o do arquivo .expected.
#
# Os valores esperados foram calculados com o gcc do sistema a partir do mesmo
# fonte (veja valor_referencia.sh), entao sao independentes do compilador.
# Para criar um caso novo:
#   1. escreva testes/exec/nome.txt (com main devolvendo int);
#   2. sh testes/valor_referencia.sh testes/exec/nome.txt > testes/exec/nome.expected
set -eu

tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

fail() {
    echo "[FALHA] $1" >&2
    [ ! -f "$tmp_dir/err" ] || cat "$tmp_dir/err" >&2
    exit 1
}

gcc -Wall -O1 -o "$tmp_dir/rv32sim" testes/rv32sim.c || fail "compilacao do simulador falhou"

total=0
for src in testes/exec/*.txt; do
    name=$(basename "$src" .txt)
    expected=$(tr -d '\r' <"testes/exec/$name.expected")

    ./parser <"$src" >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "$name: o compilador terminou com erro"
    [ ! -s "$tmp_dir/err" ] || fail "$name: o compilador produziu diagnosticos"

    sed -n '/^=== CODIGO FINAL (ASSEMBLY RISC-V) ===$/,/^======================================$/p' \
        "$tmp_dir/out" | sed '1d;$d' >"$tmp_dir/prog.s"
    [ -s "$tmp_dir/prog.s" ] || fail "$name: nenhum Assembly foi gerado"

    "$tmp_dir/rv32sim" "$tmp_dir/prog.s" >"$tmp_dir/result" 2>"$tmp_dir/err" \
        || fail "$name: erro ao executar o Assembly (veja a mensagem do simulador)"
    actual=$(cat "$tmp_dir/result")
    [ "$actual" = "$expected" ] || fail "$name: main devolveu $actual, esperado $expected"

    echo "[OK] exec: $name = $actual"
    total=$((total + 1))
done
echo "Bateria de execucao concluida: $total programas."
