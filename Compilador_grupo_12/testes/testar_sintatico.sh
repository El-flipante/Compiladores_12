#!/bin/bash

echo "Compilando o compilador completo..."
make clean
make

echo ""
echo "Executando os testes sintaticos e de recuperacao de erro..."
./parser < testes/teste_sintatico.txt
