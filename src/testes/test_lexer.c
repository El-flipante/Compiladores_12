#include <stdio.h>
#include <stdlib.h>
#include "parser.tab.h"

extern int yylex(void);
extern char *yytext;
extern FILE *yyin;
YYSTYPE yylval;

static const char *token_name(int token) {
    switch (token) {
        case IF: return "IF"; case ELSE: return "ELSE";
        case FOR: return "FOR"; case WHILE: return "WHILE";
        case RETURN: return "RETURN"; case INT: return "INT";
        case CHAR: return "CHAR"; case BOOL: return "BOOL";
        case FLOAT: return "FLOAT"; case VOID: return "VOID";
        case CHAR_LITERAL: return "CHAR_LITERAL";
        case STRING_LITERAL: return "STRING_LITERAL";
        case NUM: return "NUM"; case ID: return "ID";
        case PLUS: return "PLUS"; case MINUS: return "MINUS";
        case TIMES: return "TIMES"; case DIVIDE: return "DIVIDE";
        case MOD: return "MOD"; case LE: return "LE"; case GE: return "GE";
        case EQ: return "EQ"; case NE: return "NE"; case LT: return "LT";
        case GT: return "GT"; case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN"; case SEMI: return "SEMI";
        case LBRACK: return "LBRACK"; case RBRACK: return "RBRACK";
        case LBRACE: return "LBRACE"; case RBRACE: return "RBRACE";
        case ASSIGN: return "ASSIGN"; case COMMA: return "COMMA";
        default: return "UNKNOWN";
    }
}

int main(int argc, char **argv) {
    if (argc != 2 || !(yyin = fopen(argv[1], "r"))) {
        fprintf(stderr, "uso: test_lexer arquivo\n");
        return 2;
    }
    int token;
    while ((token = yylex()) != 0)
        printf("%s\t%s\n", token_name(token), yytext);
    fclose(yyin);
    return 0;
}
