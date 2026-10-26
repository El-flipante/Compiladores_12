EXEC = parser
BISON_FILE = parser.y
FLEX_FILE  = lexer.l
BISON_C = parser.tab.c
BISON_H = parser.tab.h
FLEX_C  = lex.yy.c

CC = gcc
CFLAGS = -Wall

all: $(EXEC)

$(EXEC): $(BISON_C) $(FLEX_C) symtab.c ast.c
	$(CC) $(CFLAGS) -o $@ $(BISON_C) $(FLEX_C) symtab.c ast.c

$(BISON_C) $(BISON_H): $(BISON_FILE)
	bison -d $(BISON_FILE)

$(FLEX_C): $(FLEX_FILE) $(BISON_H)
	flex $(FLEX_FILE)

clean:
	rm -f $(EXEC) $(BISON_C) $(BISON_H) $(FLEX_C)