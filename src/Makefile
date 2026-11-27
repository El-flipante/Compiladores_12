EXEC = parser
BISON_FILE = parser.y
FLEX_FILE  = lexer.l
BISON_C = parser.tab.c
BISON_H = parser.tab.h
FLEX_C  = lex.yy.c

CC = gcc
CFLAGS = -Wall -I.

all: $(EXEC)

$(EXEC): $(BISON_C) $(FLEX_C) symtab.c ast.c ir.c ir.h codegen.c codegen.h
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) symtab.c ast.c ir.c codegen.c

$(BISON_C) $(BISON_H): $(BISON_FILE)
	bison -d $(BISON_FILE)

$(FLEX_C): $(FLEX_FILE) $(BISON_H)
	flex $(FLEX_FILE)

test-symtab: symtab.c testes/test_symtab.c
	$(CC) $(CFLAGS) symtab.c testes/test_symtab.c -o testes/test_symtab
	./testes/test_symtab

test-ast: ast.c testes/test_ast.c
	$(CC) $(CFLAGS) ast.c testes/test_ast.c -o testes/test_ast
	./testes/test_ast

test-complementares: $(EXEC)
	sh testes/testar_complementares.sh

test-ir: $(EXEC)
	sh testes/testar_ir.sh

test-lexer:
	sh testes/testar_lexico.sh

test-ast-output: $(EXEC)
	sh testes/testar_ast.sh

test-codegen: $(EXEC)
	sh testes/testar_codegen.sh

test-exec: $(EXEC) testes/rv32sim.c
	sh testes/testar_execucao.sh

test: test-symtab test-lexer test-ast-output test-complementares test-ir test-codegen test-exec

clean:
	rm -f $(EXEC) $(BISON_C) $(BISON_H) $(FLEX_C) testes/test_symtab testes/test_ast

.PHONY: all test test-symtab test-ast test-lexer test-ast-output test-complementares test-ir test-codegen test-exec clean
