EXEC = parser
BISON_FILE = parser.y
FLEX_FILE  = lexer.l
BISON_C = parser.tab.c
BISON_H = parser.tab.h
FLEX_C  = lex.yy.c

CC = gcc
CFLAGS = -Wall -I.

all: $(EXEC)

$(EXEC): $(BISON_C) $(FLEX_C) symtab.c ast.c
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) symtab.c ast.c

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

test: test-symtab test-ast

clean:
	rm -f $(EXEC) $(BISON_C) $(BISON_H) $(FLEX_C) testes/test_symtab testes/test_ast
