#!/bin/bash

echo "Compilando o ambiente de teste léxico..."
bison -d parser.y
flex lexer.l
gcc -Wall -I. lex.yy.c testes/main_lexico.c -o testes/teste_lexico

echo "Executando o script de teste..."
./testes/teste_lexico testes/teste_lexico.txt
