#include <stdio.h>
#include "symtab.h"
#include "ast.h"
#include "parser.tab.h"

YYSTYPE yylval;

extern int yylex();
extern char* yytext;
extern FILE* yyin;

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            printf("Erro ao abrir ficheiro de teste.\n");
            return 1;
        }
    }

    int token;
    printf("A iniciar validacao lexica...\n");

    while ((token = yylex())) {
        printf("Token: %d | Lexema: %s\n", token, yytext);
    }

    printf("\nAnalise lexica concluida.\n");
    return 0;
}
