#!/bin/sh
set -eu

tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

fail() {
    echo "[FALHA] $1" >&2
    if [ -f "$tmp_dir/out" ]; then cat "$tmp_dir/out" >&2; fi
    if [ -f "$tmp_dir/err" ]; then cat "$tmp_dir/err" >&2; fi
    exit 1
}

has_text() {
    grep -Fq "$1" "$2" || fail "nao encontrou '$1'"
}

./parser < testes/complementares/positivo_semantica.txt >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "programa positivo terminou com erro"
[ ! -s "$tmp_dir/err" ] || fail "programa positivo produziu diagnostico"
for item in 'FUNC_PROTO(soma)' 'FUNC_DECL(soma)' 'FUNC_CALL(soma)' 'ARRAY_DECLARATION(valores, 4)' 'ARRAY_ACCESS(valores)' 'ARRAY_ASSIGN(valores)' 'FOR' 'WHILE' 'UNOP('; do
    has_text "$item" "$tmp_dir/out"
done
echo "[OK] prototipos, funcoes, tipos, escopos, arrays, operadores e lacos"

./parser < testes/complementares/erros_semanticos.txt >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "casos semanticos terminaram com erro de processo"
for item in \
    "tem mais elementos que o tamanho" \
    "Tipo incompativel no elemento" \
    "Assinatura de 'assinatura' difere" \
    "Indice 3 fora dos limites" \
    "indice de 'dados' deve ser uma expressao inteira" \
    "'dados_com_indice_invalido' nao e um vetor" \
    "operador modulo (%) exige operandos inteiros" \
    "Divisao por zero no operador modulo" \
    "Tipo incompativel no argumento 1" \
    "Funcao 'ausente' nao declarada" \
    "'numero' e uma variavel, nao uma funcao" \
    "Funcao nao void deve retornar um valor" \
    "Funcao void nao pode retornar um valor" \
    "pode terminar sem retornar um valor"; do
    has_text "$item" "$tmp_dir/err"
done
echo "[OK] validacoes semanticas negativas"

if ./parser < testes/complementares/erro_lexico.txt >"$tmp_dir/out" 2>"$tmp_dir/err"; then
    fail "identificador iniciado por numero foi aceito"
fi
has_text "Identificador nao pode comecar com numero" "$tmp_dir/err"
echo "[OK] diagnostico lexico"

./parser < testes/complementares/erro_sintatico.txt >"$tmp_dir/out" 2>"$tmp_dir/err" || fail "recuperacao sintatica terminou com erro"
has_text "erro sintatico" "$tmp_dir/err"
has_text "Recuperado de erro sintatico" "$tmp_dir/err"
echo "[OK] recuperacao sintatica apos ponto e virgula"

echo "Bateria complementar concluida."
