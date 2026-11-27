/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "parser.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "symtab.h"
#include "ast.h"
#include "ir.h"
#include "codegen.h"

int yylex(void);
void yyerror(const char *s);

int current_scope = 0;
int syntax_error_count = 0;
int semantic_error_count = 0;
#define SEM_ERR(...) (semantic_error_count++, fprintf(stderr, __VA_ARGS__))
ASTNode *ast_root = NULL;
DataType current_function_return_type = TYPE_UNKNOWN;
int current_function_signature_valid = 1;

int always_returns(ASTNode *node) {
    if (!node) return 0;
    switch (node->type) {
        case AST_RETURN:
            return 1;
        case AST_SEQ:
            return always_returns(node->seq.stmt1) || always_returns(node->seq.stmt2);
        case AST_BLOCK:
            return always_returns(node->block.body);
        case AST_IF:
            return node->if_stmt.else_branch != NULL &&
                   always_returns(node->if_stmt.if_branch) &&
                   always_returns(node->if_stmt.else_branch);
        default:
            return 0;
    }
}

int validate_array_index(Symbol *array, ASTNode *index, const char *name);
int validate_array_initializer(ASTNode *items, DataType element_type,
                               int array_size, const char *name);

ASTNode *make_comparison(int op, ASTNode *left, ASTNode *right) {
    ASTNode *node = ast_new_binop(op, left, right);
    if (!left || !right) {
        SEM_ERR("[ERRO SEMANTICO] Operandos invalidos na comparacao.\n");
        node->expr_type = TYPE_UNKNOWN;
    } else if (left->expr_type != right->expr_type ||
               left->expr_type == TYPE_VOID ||
               left->expr_type == TYPE_UNKNOWN) {
        SEM_ERR("[ERRO SEMANTICO] Tipos incompativeis na comparacao.\n");
        node->expr_type = TYPE_UNKNOWN;
    } else {
        node->expr_type = TYPE_BOOL;
    }
    return node;
}

ASTNode *make_for_assignment(const char *id, ASTNode *expr) {
    Symbol *symbol = lookup_symbol(id, current_scope);
    if (!symbol) {
        SEM_ERR("[ERRO SEMANTICO] Variavel '%s' nao declarada no for.\n", id);
        return NULL;
    }
    if (symbol->is_array) {
        SEM_ERR("[ERRO SEMANTICO] Vetor '%s' precisa de um indice no for.\n", id);
        return NULL;
    }
    if (!expr || symbol->type != expr->expr_type) {
        SEM_ERR("[ERRO SEMANTICO] Tipo incompativel na atribuicao de '%s' no for.\n", id);
        return NULL;
    }
    return ast_new_assign(id, expr);
}

// ----- VALIDACAO DE FUNCOES -----
DataType p_types[MAX_PARAMS];
int p_count = 0;
int p_overflow = 0;
void extract_params(ASTNode *node) {
    if (!node) return;
    if (node->type == AST_PARAM_LIST) {
        extract_params(node->param_list.next);
        extract_params(node->param_list.param);
    } else if (node->type == AST_PARAM) {
        if (p_count < MAX_PARAMS) p_types[p_count++] = node->expr_type;
        else p_overflow = 1;
    }
}
int register_function(const char *name, DataType ret_type, ASTNode *param_list_node,
                      int is_definition) {
    p_count = 0;
    p_overflow = 0;
    extract_params(param_list_node);
    if (p_overflow) {
        current_function_signature_valid = 0;
        SEM_ERR("[ERRO SEMANTICO] Funcao '%s' excede o limite de %d parametros.\n", name, MAX_PARAMS);
        return 0;
    }
    Symbol *existing = lookup_function(name);
    if (existing) {
        int same_signature = existing->return_type == ret_type &&
                             existing->param_count == p_count;
        for (int i = 0; same_signature && i < p_count; i++)
            same_signature = existing->param_types[i] == p_types[i];
        if (!same_signature) {
            SEM_ERR("[ERRO SEMANTICO] Assinatura de '%s' difere da declaracao anterior.\n", name);
            return 0;
        }
        if (is_definition && existing->is_defined) {
            SEM_ERR("[ERRO SEMANTICO] Funcao '%s' definida mais de uma vez.\n", name);
            return 0;
        }
        if (is_definition) existing->is_defined = 1;
        return 1;
    }
    Symbol *s = insert_function(name, ret_type, p_count, p_types);
    if (!s) {
        SEM_ERR("[ERRO SEMANTICO] Falha ao registrar funcao '%s' (ja existe ou muitos parametros).\n", name);
        return 0;
    }
    s->is_defined = is_definition;
    return 1;
}

DataType a_types[MAX_PARAMS];
int a_count = 0;
int a_overflow = 0;
int a_invalid = 0;
void extract_args(ASTNode *node) {
    if (!node) return;
    if (node->type == AST_ARGS_LIST) {
        extract_args(node->args_list.next);
        if (a_count < MAX_PARAMS) {
            ASTNode *arg = node->args_list.expr;
            a_types[a_count++] = arg ? arg->expr_type : TYPE_UNKNOWN;
            if (!arg || arg->expr_type == TYPE_UNKNOWN) a_invalid = 1;
        } else {
            a_overflow = 1;
        }
    } else {
        if (a_count < MAX_PARAMS) {
            a_types[a_count++] = node->expr_type;
            if (node->expr_type == TYPE_UNKNOWN) a_invalid = 1;
        } else {
            a_overflow = 1;
        }
    }
}
int validate_function_call(const char *name, ASTNode *args_list_node) {
    Symbol *shadow = lookup_symbol(name, current_scope);
    if (shadow) {
        SEM_ERR("[ERRO SEMANTICO] '%s' e uma variavel, nao uma funcao.\n", name);
        return 0;
    }
    Symbol *fn = lookup_function(name);
    if (!fn) {
        SEM_ERR("[ERRO SEMANTICO] Funcao '%s' nao declarada!\n", name);
        return 0;
    }
    a_count = 0;
    a_overflow = 0;
    a_invalid = 0;
    extract_args(args_list_node);
    if (a_overflow) {
        SEM_ERR("[ERRO SEMANTICO] Chamada a '%s' excede o limite de %d argumentos.\n", name, MAX_PARAMS);
        return 0;
    }
    if (a_count != fn->param_count) {
        SEM_ERR("[ERRO SEMANTICO] Funcao '%s' espera %d argumentos, mas %d foram passados.\n", name, fn->param_count, a_count);
        return 0;
    }
    if (a_invalid) {
        SEM_ERR("[ERRO SEMANTICO] Chamada a '%s' contem argumento invalido.\n", name);
        return 0;
    }
    for (int i = 0; i < a_count; i++) {
        if (a_types[i] != fn->param_types[i] && a_types[i] != TYPE_UNKNOWN && fn->param_types[i] != TYPE_UNKNOWN) {
            SEM_ERR("[ERRO SEMANTICO] Tipo incompativel no argumento %d da chamada a '%s'.\n", i+1, name);
            return 0;
        }
    }
    return 1;
}

#line 258 "parser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_NUM = 3,                        /* NUM  */
  YYSYMBOL_CHAR_LITERAL = 4,               /* CHAR_LITERAL  */
  YYSYMBOL_ID = 5,                         /* ID  */
  YYSYMBOL_STRING_LITERAL = 6,             /* STRING_LITERAL  */
  YYSYMBOL_PLUS = 7,                       /* PLUS  */
  YYSYMBOL_MINUS = 8,                      /* MINUS  */
  YYSYMBOL_TIMES = 9,                      /* TIMES  */
  YYSYMBOL_DIVIDE = 10,                    /* DIVIDE  */
  YYSYMBOL_MOD = 11,                       /* MOD  */
  YYSYMBOL_LT = 12,                        /* LT  */
  YYSYMBOL_GT = 13,                        /* GT  */
  YYSYMBOL_LE = 14,                        /* LE  */
  YYSYMBOL_GE = 15,                        /* GE  */
  YYSYMBOL_EQ = 16,                        /* EQ  */
  YYSYMBOL_NE = 17,                        /* NE  */
  YYSYMBOL_LPAREN = 18,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 19,                    /* RPAREN  */
  YYSYMBOL_LBRACK = 20,                    /* LBRACK  */
  YYSYMBOL_RBRACK = 21,                    /* RBRACK  */
  YYSYMBOL_LBRACE = 22,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 23,                    /* RBRACE  */
  YYSYMBOL_SEMI = 24,                      /* SEMI  */
  YYSYMBOL_ASSIGN = 25,                    /* ASSIGN  */
  YYSYMBOL_COMMA = 26,                     /* COMMA  */
  YYSYMBOL_IF = 27,                        /* IF  */
  YYSYMBOL_ELSE = 28,                      /* ELSE  */
  YYSYMBOL_FOR = 29,                       /* FOR  */
  YYSYMBOL_WHILE = 30,                     /* WHILE  */
  YYSYMBOL_RETURN = 31,                    /* RETURN  */
  YYSYMBOL_INT = 32,                       /* INT  */
  YYSYMBOL_CHAR = 33,                      /* CHAR  */
  YYSYMBOL_BOOL = 34,                      /* BOOL  */
  YYSYMBOL_FLOAT = 35,                     /* FLOAT  */
  YYSYMBOL_VOID = 36,                      /* VOID  */
  YYSYMBOL_LOWER_THAN_ELSE = 37,           /* LOWER_THAN_ELSE  */
  YYSYMBOL_UMINUS = 38,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 39,                  /* $accept  */
  YYSYMBOL_program = 40,                   /* program  */
  YYSYMBOL_stmt = 41,                      /* stmt  */
  YYSYMBOL_expr_stmt = 42,                 /* expr_stmt  */
  YYSYMBOL_decl_stmt = 43,                 /* decl_stmt  */
  YYSYMBOL_assign_stmt = 44,               /* assign_stmt  */
  YYSYMBOL_type = 45,                      /* type  */
  YYSYMBOL_expr = 46,                      /* expr  */
  YYSYMBOL_func_decl = 47,                 /* func_decl  */
  YYSYMBOL_48_1 = 48,                      /* $@1  */
  YYSYMBOL_func_header = 49,               /* func_header  */
  YYSYMBOL_50_2 = 50,                      /* $@2  */
  YYSYMBOL_param_list = 51,                /* param_list  */
  YYSYMBOL_param = 52,                     /* param  */
  YYSYMBOL_args_list = 53,                 /* args_list  */
  YYSYMBOL_block_stmt = 54,                /* block_stmt  */
  YYSYMBOL_55_3 = 55,                      /* $@3  */
  YYSYMBOL_if_stmt = 56,                   /* if_stmt  */
  YYSYMBOL_while_stmt = 57,                /* while_stmt  */
  YYSYMBOL_for_stmt = 58,                  /* for_stmt  */
  YYSYMBOL_59_4 = 59,                      /* $@4  */
  YYSYMBOL_for_init = 60,                  /* for_init  */
  YYSYMBOL_for_condition = 61,             /* for_condition  */
  YYSYMBOL_for_update = 62                 /* for_update  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   454

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  39
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  24
/* YYNRULES -- Number of rules.  */
#define YYNRULES  73
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  146

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   293


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   231,   231,   232,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   260,   269,   277,   281,   298,   316,   341,
     359,   382,   383,   384,   385,   386,   390,   400,   410,   420,
     430,   440,   455,   456,   457,   458,   459,   460,   461,   462,
     463,   464,   480,   495,   505,   516,   516,   541,   541,   549,
     550,   551,   555,   573,   574,   575,   579,   579,   587,   593,
     602,   611,   611,   622,   623,   624,   625,   626,   648,   649,
     653,   654,   655,   659
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "NUM", "CHAR_LITERAL",
  "ID", "STRING_LITERAL", "PLUS", "MINUS", "TIMES", "DIVIDE", "MOD", "LT",
  "GT", "LE", "GE", "EQ", "NE", "LPAREN", "RPAREN", "LBRACK", "RBRACK",
  "LBRACE", "RBRACE", "SEMI", "ASSIGN", "COMMA", "IF", "ELSE", "FOR",
  "WHILE", "RETURN", "INT", "CHAR", "BOOL", "FLOAT", "VOID",
  "LOWER_THAN_ELSE", "UMINUS", "$accept", "program", "stmt", "expr_stmt",
  "decl_stmt", "assign_stmt", "type", "expr", "func_decl", "$@1",
  "func_header", "$@2", "param_list", "param", "args_list", "block_stmt",
  "$@3", "if_stmt", "while_stmt", "for_stmt", "$@4", "for_init",
  "for_condition", "for_update", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-85)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -85,    70,   -85,   -22,   -85,   -85,    39,    12,    12,   -85,
      13,    18,    19,     5,   -85,   -85,   -85,   -85,   -85,   -85,
     -85,   -85,   -85,    46,   234,   -85,    -3,   -85,   -85,   -85,
     -85,   -85,    12,    12,    12,    67,   -85,   387,   -85,    12,
     -85,    12,   -85,   252,     4,    12,    12,    12,    12,    12,
      12,    12,    12,    12,    12,    12,   -85,   -85,   -85,   426,
     -14,   342,   270,    12,   -85,   112,   400,   204,   413,   -85,
     -85,    52,   -85,    84,    84,   -85,   -85,   -85,   437,   437,
     437,   437,   437,   437,   -85,   -85,    12,    66,   -85,   357,
     -85,   184,   -85,   -85,   -85,    72,   288,    12,   184,    48,
      77,   148,   426,    12,   -85,    79,    29,   -85,   426,    85,
     -85,   103,    -8,   -85,    44,   -85,   306,   184,    12,    58,
     -85,   -85,    48,   -85,    88,   -85,   -85,   324,    47,   426,
      95,   -85,    12,   -85,    12,    12,   184,    63,   372,   426,
     -85,    94,    96,   -85,    12,   426
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     1,     0,    39,    40,    41,     0,     0,    56,
       0,     0,     0,     0,    21,    22,    24,    23,    25,     3,
       4,     5,     6,     0,     0,     7,     0,     8,     9,    10,
      11,    14,    53,     0,     0,    41,    27,     0,     2,     0,
      61,     0,    13,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    15,    45,    44,    54,
       0,     0,     0,     0,    38,     0,     0,     0,     0,    12,
      47,     0,    16,    26,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,     2,    43,     0,    42,    19,     0,
      57,     0,    63,    64,    65,     0,     0,    68,     0,    49,
       0,     0,    55,     0,    42,    58,     0,    66,    69,     0,
      60,     0,     0,    50,     0,    46,     0,     0,     0,    70,
      52,    48,     0,    17,     0,    20,    59,     0,    41,    71,
       0,    51,    53,    67,     0,     0,     0,     0,     0,    72,
      62,     0,    42,    18,     0,    73
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -85,   -34,   -84,   -85,    55,    56,   -64,    -7,   -85,   -85,
     -85,   -85,   -85,     2,   -13,   -85,   -85,   -85,   -85,   -85,
     -85,   -85,   -85,   -85
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,    19,    20,    21,    22,    23,    24,    25,    84,
      26,    99,   112,   113,    60,    27,    38,    28,    29,    30,
      67,    97,   109,   130
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      36,    37,    31,    95,    65,    85,    43,   105,     4,     5,
      35,   121,    86,     7,   110,     4,     5,    35,   122,    57,
       7,    58,    70,     8,    71,    59,    61,    62,    72,    42,
       8,    39,    66,   126,    68,   111,    40,    41,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    71,
     101,    44,   140,    72,   118,   100,    89,    32,   111,    33,
      96,     4,     5,   128,    34,    32,     7,   134,   123,   124,
       2,     3,   135,     4,     5,     6,     8,   106,     7,   102,
      14,    15,    16,    17,    18,    32,   141,    63,     8,    86,
     108,   103,     9,    47,    48,    49,   116,    10,   114,    11,
      12,    13,    14,    15,    16,    17,    18,   117,   120,   119,
     132,   127,   129,     3,   136,     4,     5,     6,   143,   137,
       7,   144,    93,    94,   131,    59,     0,   138,   139,     0,
       8,     0,     0,     0,     9,    90,     0,   145,     0,    10,
       0,    11,    12,    13,    14,    15,    16,    17,    18,     3,
       0,     4,     5,     6,     0,     0,     7,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     0,     0,     0,
       9,   115,     0,     0,     0,    10,     0,    11,    12,    13,
      14,    15,    16,    17,    18,     3,     0,     4,     5,     6,
       0,     0,     7,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     0,     0,     0,     9,     4,     5,     6,
       0,    10,     7,    11,    12,    13,    14,    15,    16,    17,
      18,     0,     8,     0,     0,     0,     0,     0,    92,     0,
       0,     0,     0,     0,     0,     0,    14,    15,    16,    17,
      18,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,     0,     0,     0,     0,     0,     0,    56,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       0,     0,     0,     0,     0,     0,    69,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     0,     0,
       0,     0,     0,     0,    88,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,     0,     0,     0,     0,
       0,     0,   107,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,     0,     0,     0,     0,     0,
     125,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,     0,     0,     0,     0,     0,     0,   133,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       0,     0,     0,    87,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     0,     0,     0,   104,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       0,     0,     0,   142,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     0,    64,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     0,    91,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,     0,    98,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    45,    46,    47,    48,    49,    -1,
      -1,    -1,    -1,    -1,    -1
};

static const yytype_int16 yycheck[] =
{
       7,     8,    24,    67,    38,    19,    13,    91,     3,     4,
       5,    19,    26,     8,    98,     3,     4,     5,    26,    22,
       8,    24,    18,    18,    20,    32,    33,    34,    24,    24,
      18,    18,    39,   117,    41,    99,    18,    18,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    20,
      84,     5,   136,    24,    25,     3,    63,    18,   122,    20,
      67,     3,     4,     5,    25,    18,     8,    20,    24,    25,
       0,     1,    25,     3,     4,     5,    18,     5,     8,    86,
      32,    33,    34,    35,    36,    18,    23,    20,    18,    26,
      97,    25,    22,     9,    10,    11,   103,    27,    21,    29,
      30,    31,    32,    33,    34,    35,    36,    28,     5,    24,
      22,   118,   119,     1,    19,     3,     4,     5,    24,   132,
       8,    25,    67,    67,   122,   132,    -1,   134,   135,    -1,
      18,    -1,    -1,    -1,    22,    23,    -1,   144,    -1,    27,
      -1,    29,    30,    31,    32,    33,    34,    35,    36,     1,
      -1,     3,     4,     5,    -1,    -1,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    18,    -1,    -1,    -1,
      22,    23,    -1,    -1,    -1,    27,    -1,    29,    30,    31,
      32,    33,    34,    35,    36,     1,    -1,     3,     4,     5,
      -1,    -1,     8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    18,    -1,    -1,    -1,    22,     3,     4,     5,
      -1,    27,     8,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    18,    -1,    -1,    -1,    -1,    -1,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    32,    33,    34,    35,
      36,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    -1,    -1,    -1,    -1,    -1,    24,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    -1,    -1,    -1,    -1,    -1,    24,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    -1,
      -1,    -1,    -1,    -1,    24,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    -1,    -1,    -1,    -1,
      -1,    -1,    24,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    -1,    -1,    -1,    -1,    -1,
      24,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    -1,    -1,    -1,    -1,    -1,    24,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    -1,    -1,    21,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    -1,    -1,    -1,    21,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    -1,    -1,    21,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    -1,    19,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    -1,    19,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    -1,    19,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    40,     0,     1,     3,     4,     5,     8,    18,    22,
      27,    29,    30,    31,    32,    33,    34,    35,    36,    41,
      42,    43,    44,    45,    46,    47,    49,    54,    56,    57,
      58,    24,    18,    20,    25,     5,    46,    46,    55,    18,
      18,    18,    24,    46,     5,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    24,    22,    24,    46,
      53,    46,    46,    20,    19,    40,    46,    59,    46,    24,
      18,    20,    24,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    46,    48,    19,    26,    21,    24,    46,
      23,    19,    24,    43,    44,    45,    46,    60,    19,    50,
       3,    40,    46,    25,    21,    41,     5,    24,    46,    61,
      41,    45,    51,    52,    21,    23,    46,    28,    25,    24,
       5,    19,    26,    24,    25,    24,    41,    46,     5,    46,
      62,    52,    22,    24,    20,    25,    19,    53,    46,    46,
      41,    23,    21,    24,    25,    46
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    39,    40,    40,    41,    41,    41,    41,    41,    41,
      41,    41,    41,    41,    41,    42,    43,    43,    43,    44,
      44,    45,    45,    45,    45,    45,    46,    46,    46,    46,
      46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
      46,    46,    46,    46,    47,    48,    47,    50,    49,    51,
      51,    51,    52,    53,    53,    53,    55,    54,    56,    56,
      57,    59,    58,    60,    60,    60,    60,    60,    61,    61,
      62,    62,    62,    62
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     2,     2,     2,     3,     6,    10,     4,
       7,     1,     1,     1,     1,     1,     3,     2,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     1,
       1,     1,     4,     4,     2,     0,     5,     0,     6,     0,
       1,     3,     2,     0,     1,     3,     0,     4,     5,     7,
       5,     0,     9,     1,     1,     1,     2,     5,     0,     1,
       0,     1,     3,     6
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: %empty  */
#line 231 "parser.y"
              { (yyval.node) = NULL; ast_root = NULL; }
#line 1462 "parser.tab.c"
    break;

  case 3: /* program: program stmt  */
#line 232 "parser.y"
                   {
        (yyval.node) = ast_new_seq((yyvsp[-1].node), (yyvsp[0].node));
        ast_root = (yyval.node);
    }
#line 1471 "parser.tab.c"
    break;

  case 4: /* stmt: expr_stmt  */
#line 239 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 1477 "parser.tab.c"
    break;

  case 5: /* stmt: decl_stmt  */
#line 240 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 1483 "parser.tab.c"
    break;

  case 6: /* stmt: assign_stmt  */
#line 241 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 1489 "parser.tab.c"
    break;

  case 7: /* stmt: func_decl  */
#line 242 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 1495 "parser.tab.c"
    break;

  case 8: /* stmt: block_stmt  */
#line 243 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 1501 "parser.tab.c"
    break;

  case 9: /* stmt: if_stmt  */
#line 244 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 1507 "parser.tab.c"
    break;

  case 10: /* stmt: while_stmt  */
#line 245 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 1513 "parser.tab.c"
    break;

  case 11: /* stmt: for_stmt  */
#line 246 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 1519 "parser.tab.c"
    break;

  case 12: /* stmt: RETURN expr SEMI  */
#line 247 "parser.y"
                       {
        if (current_function_return_type == TYPE_UNKNOWN) {
            SEM_ERR("[ERRO SEMANTICO] 'return' usado fora de uma funcao.\n");
        } else if (current_function_return_type == TYPE_VOID) {
            SEM_ERR("[ERRO SEMANTICO] Funcao void nao pode retornar um valor.\n");
        } else if ((current_function_return_type == TYPE_INT ||
                    current_function_return_type == TYPE_CHAR) &&
                   (yyvsp[-1].node) && (yyvsp[-1].node)->expr_type != TYPE_UNKNOWN &&
                   (yyvsp[-1].node)->expr_type != current_function_return_type) {
            SEM_ERR("[ERRO SEMANTICO] Tipo do valor retornado difere do tipo da funcao.\n");
        }
        (yyval.node) = ast_new_return((yyvsp[-1].node));
    }
#line 1537 "parser.tab.c"
    break;

  case 13: /* stmt: RETURN SEMI  */
#line 260 "parser.y"
                  {
        if (current_function_return_type == TYPE_UNKNOWN) {
            SEM_ERR("[ERRO SEMANTICO] 'return' usado fora de uma funcao.\n");
        } else if (current_function_return_type == TYPE_INT ||
                   current_function_return_type == TYPE_CHAR) {
            SEM_ERR("[ERRO SEMANTICO] Funcao nao void deve retornar um valor.\n");
        }
        (yyval.node) = ast_new_return(NULL);
    }
#line 1551 "parser.tab.c"
    break;

  case 14: /* stmt: error SEMI  */
#line 269 "parser.y"
                 {
        yyerrok;
        fprintf(stderr, "[AVISO] Recuperado de erro sintatico antes do ponto e virgula.\n");
        (yyval.node) = NULL;
    }
#line 1561 "parser.tab.c"
    break;

  case 15: /* expr_stmt: expr SEMI  */
#line 277 "parser.y"
                { (yyval.node) = (yyvsp[-1].node); }
#line 1567 "parser.tab.c"
    break;

  case 16: /* decl_stmt: type ID SEMI  */
#line 281 "parser.y"
                   {
        Symbol *s = NULL;
        if ((yyvsp[-2].data_type) == TYPE_VOID) {
            SEM_ERR("[ERRO SEMANTICO] Variavel '%s' nao pode ter tipo void.\n", (yyvsp[-1].id));
        } else {
            s = insert_symbol((yyvsp[-1].id), (yyvsp[-2].data_type), 0, 0, current_scope);
        }
        if ((yyvsp[-2].data_type) == TYPE_VOID) {
            (yyval.node) = NULL;
        } else if (!s) {
            SEM_ERR("[ERRO SEMANTICO] Variavel '%s' ja declarada no escopo %d.\n", (yyvsp[-1].id), current_scope);
            (yyval.node) = NULL;
        } else {
            (yyval.node) = ast_new_decl((yyvsp[-1].id), (yyvsp[-2].data_type));
        }
        free((yyvsp[-1].id));
    }
#line 1589 "parser.tab.c"
    break;

  case 17: /* decl_stmt: type ID LBRACK NUM RBRACK SEMI  */
#line 298 "parser.y"
                                     {
        if ((yyvsp[-5].data_type) == TYPE_VOID) {
            SEM_ERR("[ERRO SEMANTICO] Vetor '%s' nao pode ter tipo void.\n", (yyvsp[-4].id));
            (yyval.node) = NULL;
        } else if ((yyvsp[-2].intval) <= 0) {
            SEM_ERR("[ERRO SEMANTICO] O tamanho do vetor '%s' deve ser positivo.\n", (yyvsp[-4].id));
            (yyval.node) = NULL;
        } else {
            Symbol *s = insert_symbol((yyvsp[-4].id), (yyvsp[-5].data_type), 1, (yyvsp[-2].intval), current_scope);
            if (!s) {
                SEM_ERR("[ERRO SEMANTICO] Vetor '%s' ja declarado no escopo %d.\n", (yyvsp[-4].id), current_scope);
                (yyval.node) = NULL;
            } else {
                (yyval.node) = ast_new_array_decl((yyvsp[-4].id), (yyvsp[-5].data_type), (yyvsp[-2].intval));
            }
        }
        free((yyvsp[-4].id));
    }
#line 1612 "parser.tab.c"
    break;

  case 18: /* decl_stmt: type ID LBRACK NUM RBRACK ASSIGN LBRACE args_list RBRACE SEMI  */
#line 316 "parser.y"
                                                                    { /* <--- NOVA REGRA AQUI */
        if ((yyvsp[-9].data_type) == TYPE_VOID) {
            SEM_ERR("[ERRO SEMANTICO] Vetor '%s' nao pode ter tipo void.\n", (yyvsp[-8].id));
            (yyval.node) = NULL;
        } else if ((yyvsp[-6].intval) <= 0) {
            SEM_ERR("[ERRO SEMANTICO] O tamanho do vetor '%s' deve ser positivo.\n", (yyvsp[-8].id));
            (yyval.node) = NULL;
        } else if (!validate_array_initializer((yyvsp[-2].node), (yyvsp[-9].data_type), (yyvsp[-6].intval), (yyvsp[-8].id))) {
            (yyval.node) = NULL;
        } else {
            Symbol *s = insert_symbol((yyvsp[-8].id), (yyvsp[-9].data_type), 1, (yyvsp[-6].intval), current_scope);
            if (!s) {
                SEM_ERR("[ERRO SEMANTICO] Vetor '%s' ja declarado no escopo %d.\n", (yyvsp[-8].id), current_scope);
                (yyval.node) = NULL;
            } else {
                // Reaproveitamos 'args_list' para a lista de inicialização separada por vírgulas
                (yyval.node) = ast_new_array_decl_init((yyvsp[-8].id), (yyvsp[-9].data_type), (yyvsp[-6].intval), (yyvsp[-2].node)); 
            }
        }
        free((yyvsp[-8].id));
    }
#line 1638 "parser.tab.c"
    break;

  case 19: /* assign_stmt: ID ASSIGN expr SEMI  */
#line 341 "parser.y"
                          {
        Symbol *s = lookup_symbol((yyvsp[-3].id), current_scope);
        if (!s) {
            SEM_ERR("[ERRO SEMANTICO] Atribuicao: Variavel '%s' nao foi declarada!\n", (yyvsp[-3].id));
            (yyval.node) = NULL;
        } else if (!(yyvsp[-1].node)) {
            (yyval.node) = NULL;
        } else if (s->is_array) {
            SEM_ERR("[ERRO SEMANTICO] Vetor '%s' deve ser acessado com indice.\n", (yyvsp[-3].id));
            (yyval.node) = NULL;
        } else if (s->type != (yyvsp[-1].node)->expr_type) {
            SEM_ERR("[ERRO SEMANTICO] Atribuicao: Tipos incompativeis para a variavel '%s'.\n", (yyvsp[-3].id));
            (yyval.node) = NULL;
        } else {
            (yyval.node) = ast_new_assign((yyvsp[-3].id), (yyvsp[-1].node));
        }
        free((yyvsp[-3].id));
    }
#line 1661 "parser.tab.c"
    break;

  case 20: /* assign_stmt: ID LBRACK expr RBRACK ASSIGN expr SEMI  */
#line 359 "parser.y"
                                             {
        Symbol *s = lookup_symbol((yyvsp[-6].id), current_scope);
        int valid = s != NULL && s->is_array && (yyvsp[-4].node) != NULL && (yyvsp[-1].node) != NULL;
        if (!s) {
            SEM_ERR("[ERRO SEMANTICO] Vetor '%s' nao foi declarado.\n", (yyvsp[-6].id));
        } else if (!s->is_array) {
            SEM_ERR("[ERRO SEMANTICO] '%s' nao e um vetor.\n", (yyvsp[-6].id));
        }
        if (s && s->is_array && !validate_array_index(s, (yyvsp[-4].node), (yyvsp[-6].id)))
            valid = 0;
        else if (!(yyvsp[-4].node))
            valid = 0;
        if (s && s->is_array && (yyvsp[-1].node) && s->type != (yyvsp[-1].node)->expr_type) {
            SEM_ERR("[ERRO SEMANTICO] Tipo incompativel ao atribuir em '%s'.\n", (yyvsp[-6].id));
            valid = 0;
        }

        (yyval.node) = valid ? ast_new_array_assign((yyvsp[-6].id), (yyvsp[-4].node), (yyvsp[-1].node)) : NULL;
        free((yyvsp[-6].id));
    }
#line 1686 "parser.tab.c"
    break;

  case 21: /* type: INT  */
#line 382 "parser.y"
            { (yyval.data_type) = TYPE_INT; }
#line 1692 "parser.tab.c"
    break;

  case 22: /* type: CHAR  */
#line 383 "parser.y"
            { (yyval.data_type) = TYPE_CHAR; }
#line 1698 "parser.tab.c"
    break;

  case 23: /* type: FLOAT  */
#line 384 "parser.y"
            { (yyval.data_type) = TYPE_FLOAT; }
#line 1704 "parser.tab.c"
    break;

  case 24: /* type: BOOL  */
#line 385 "parser.y"
            { (yyval.data_type) = TYPE_BOOL; }
#line 1710 "parser.tab.c"
    break;

  case 25: /* type: VOID  */
#line 386 "parser.y"
            { (yyval.data_type) = TYPE_VOID; }
#line 1716 "parser.tab.c"
    break;

  case 26: /* expr: expr PLUS expr  */
#line 390 "parser.y"
                          {
        if (!(yyvsp[-2].node) || !(yyvsp[0].node) || (yyvsp[-2].node)->expr_type != (yyvsp[0].node)->expr_type) {
            SEM_ERR("[ERRO SEMANTICO] Incompatibilidade de tipos na soma!\n");
            (yyval.node) = ast_new_binop(PLUS, (yyvsp[-2].node), (yyvsp[0].node));
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        } else {
            (yyval.node) = ast_new_binop(PLUS, (yyvsp[-2].node), (yyvsp[0].node));
            (yyval.node)->expr_type = (yyvsp[-2].node)->expr_type;
        }
      }
#line 1731 "parser.tab.c"
    break;

  case 27: /* expr: MINUS expr  */
#line 400 "parser.y"
                              {
        if (!(yyvsp[0].node) || ((yyvsp[0].node)->expr_type != TYPE_INT && (yyvsp[0].node)->expr_type != TYPE_FLOAT)) {
            SEM_ERR("[ERRO SEMANTICO] Operador unario '-' exige operando numerico.\n");
            (yyval.node) = ast_new_unop(MINUS, (yyvsp[0].node));
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        } else {
            (yyval.node) = ast_new_unop(MINUS, (yyvsp[0].node));
            (yyval.node)->expr_type = (yyvsp[0].node)->expr_type;
        }
      }
#line 1746 "parser.tab.c"
    break;

  case 28: /* expr: expr MINUS expr  */
#line 410 "parser.y"
                          {
        if (!(yyvsp[-2].node) || !(yyvsp[0].node) || (yyvsp[-2].node)->expr_type != (yyvsp[0].node)->expr_type) {
            SEM_ERR("[ERRO SEMANTICO] Incompatibilidade de tipos na subtraÃ§Ã£o!\n");
            (yyval.node) = ast_new_binop(MINUS, (yyvsp[-2].node), (yyvsp[0].node));
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        } else {
            (yyval.node) = ast_new_binop(MINUS, (yyvsp[-2].node), (yyvsp[0].node));
            (yyval.node)->expr_type = (yyvsp[-2].node)->expr_type;
        }
      }
#line 1761 "parser.tab.c"
    break;

  case 29: /* expr: expr TIMES expr  */
#line 420 "parser.y"
                          {
        if (!(yyvsp[-2].node) || !(yyvsp[0].node) || (yyvsp[-2].node)->expr_type != (yyvsp[0].node)->expr_type) {
            SEM_ERR("[ERRO SEMANTICO] Incopatibilidade de tipos na multiplicaÃ§Ã£o!\n");
            (yyval.node) = ast_new_binop(TIMES, (yyvsp[-2].node), (yyvsp[0].node));
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        } else {
            (yyval.node) = ast_new_binop(TIMES, (yyvsp[-2].node), (yyvsp[0].node));
            (yyval.node)->expr_type = (yyvsp[-2].node)->expr_type;
        }
      }
#line 1776 "parser.tab.c"
    break;

  case 30: /* expr: expr DIVIDE expr  */
#line 430 "parser.y"
                          {
        if (!(yyvsp[-2].node) || !(yyvsp[0].node) || (yyvsp[-2].node)->expr_type != (yyvsp[0].node)->expr_type) {
            SEM_ERR("[ERRO SEMANTICO] Incopatibilidade de tipos na divisÃ£o!\n");
            (yyval.node) = ast_new_binop(DIVIDE, (yyvsp[-2].node), (yyvsp[0].node));
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        } else {
            (yyval.node) = ast_new_binop(DIVIDE, (yyvsp[-2].node), (yyvsp[0].node));
            (yyval.node)->expr_type = (yyvsp[-2].node)->expr_type;
        }
      }
#line 1791 "parser.tab.c"
    break;

  case 31: /* expr: expr MOD expr  */
#line 440 "parser.y"
                    {
        (yyval.node) = ast_new_binop(MOD, (yyvsp[-2].node), (yyvsp[0].node));
        if (!(yyvsp[-2].node) || !(yyvsp[0].node)) {
            SEM_ERR("[ERRO SEMANTICO] Operandos invalidos para o modulo.\n");
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        } else if ((yyvsp[-2].node)->expr_type != TYPE_INT || (yyvsp[0].node)->expr_type != TYPE_INT) {
            SEM_ERR("[ERRO SEMANTICO] O operador modulo (%%) exige operandos inteiros.\n");
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        } else if ((yyvsp[0].node)->type == AST_NUM && (yyvsp[0].node)->intval == 0) {
            SEM_ERR("[ERRO SEMANTICO] Divisao por zero no operador modulo (%%).\n");
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        } else {
            (yyval.node)->expr_type = TYPE_INT;
        }
      }
#line 1811 "parser.tab.c"
    break;

  case 32: /* expr: expr LT expr  */
#line 455 "parser.y"
                   { (yyval.node) = make_comparison(LT, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1817 "parser.tab.c"
    break;

  case 33: /* expr: expr GT expr  */
#line 456 "parser.y"
                   { (yyval.node) = make_comparison(GT, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1823 "parser.tab.c"
    break;

  case 34: /* expr: expr LE expr  */
#line 457 "parser.y"
                   { (yyval.node) = make_comparison(LE, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1829 "parser.tab.c"
    break;

  case 35: /* expr: expr GE expr  */
#line 458 "parser.y"
                   { (yyval.node) = make_comparison(GE, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1835 "parser.tab.c"
    break;

  case 36: /* expr: expr EQ expr  */
#line 459 "parser.y"
                   { (yyval.node) = make_comparison(EQ, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1841 "parser.tab.c"
    break;

  case 37: /* expr: expr NE expr  */
#line 460 "parser.y"
                   { (yyval.node) = make_comparison(NE, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1847 "parser.tab.c"
    break;

  case 38: /* expr: LPAREN expr RPAREN  */
#line 461 "parser.y"
                          { (yyval.node) = (yyvsp[-1].node); }
#line 1853 "parser.tab.c"
    break;

  case 39: /* expr: NUM  */
#line 462 "parser.y"
                          { (yyval.node) = ast_new_num((yyvsp[0].intval)); }
#line 1859 "parser.tab.c"
    break;

  case 40: /* expr: CHAR_LITERAL  */
#line 463 "parser.y"
                          { (yyval.node) = ast_new_char((yyvsp[0].intval)); }
#line 1865 "parser.tab.c"
    break;

  case 41: /* expr: ID  */
#line 464 "parser.y"
                          {
        Symbol *s = lookup_symbol((yyvsp[0].id), current_scope);
        if (!s) {
            if (lookup_function((yyvsp[0].id)))
                SEM_ERR("[ERRO SEMANTICO] Funcao '%s' usada sem chamada.\n", (yyvsp[0].id));
            else
                SEM_ERR("[ERRO SEMANTICO] Variavel '%s' nao declarada!\n", (yyvsp[0].id));
            (yyval.node) = ast_new_id((yyvsp[0].id), TYPE_UNKNOWN);
        } else if (s->is_array) {
            SEM_ERR("[ERRO SEMANTICO] Vetor '%s' deve ser acessado com indice.\n", (yyvsp[0].id));
            (yyval.node) = ast_new_id((yyvsp[0].id), TYPE_UNKNOWN);
        } else {
            (yyval.node) = ast_new_id((yyvsp[0].id), s->type);
        }
        free((yyvsp[0].id));
      }
#line 1886 "parser.tab.c"
    break;

  case 42: /* expr: ID LBRACK expr RBRACK  */
#line 480 "parser.y"
                            {
        Symbol *s = lookup_symbol((yyvsp[-3].id), current_scope);
        if (!s) {
            SEM_ERR("[ERRO SEMANTICO] Vetor '%s' nao foi declarado.\n", (yyvsp[-3].id));
            (yyval.node) = NULL;
        } else if (!s->is_array) {
            SEM_ERR("[ERRO SEMANTICO] '%s' nao e um vetor.\n", (yyvsp[-3].id));
            (yyval.node) = NULL;
        } else if (!validate_array_index(s, (yyvsp[-1].node), (yyvsp[-3].id))) {
            (yyval.node) = NULL;
        } else {
            (yyval.node) = ast_new_array_access((yyvsp[-3].id), (yyvsp[-1].node), s->type);
        }
        free((yyvsp[-3].id));
      }
#line 1906 "parser.tab.c"
    break;

  case 43: /* expr: ID LPAREN args_list RPAREN  */
#line 495 "parser.y"
                                 {
        int valid_call = validate_function_call((yyvsp[-3].id), (yyvsp[-1].node));
        (yyval.node) = ast_new_func_call((yyvsp[-3].id), (yyvsp[-1].node));
        if (!valid_call && (yyval.node))
            (yyval.node)->expr_type = TYPE_UNKNOWN;
        free((yyvsp[-3].id));
      }
#line 1918 "parser.tab.c"
    break;

  case 44: /* func_decl: func_header SEMI  */
#line 505 "parser.y"
                       {
          ASTNode *function = (yyvsp[-1].node);
          if (current_function_signature_valid)
              current_function_signature_valid =
                  register_function(function->func_decl.id,
                                   function->func_decl.return_type,
                                   function->func_decl.params, 0);
          close_scope(current_scope);
          current_scope--;
          (yyval.node) = function;
      }
#line 1934 "parser.tab.c"
    break;

  case 45: /* $@1: %empty  */
#line 516 "parser.y"
                         {
          ASTNode *function = (yyvsp[-1].node);
          current_function_return_type = function->func_decl.return_type;
          if (current_function_signature_valid)
              current_function_signature_valid =
                  register_function(function->func_decl.id,
                                   function->func_decl.return_type,
                                   function->func_decl.params, 1);
      }
#line 1948 "parser.tab.c"
    break;

  case 46: /* func_decl: func_header LBRACE $@1 program RBRACE  */
#line 524 "parser.y"
                       {
          ASTNode *function = (yyvsp[-4].node);
          DataType return_type = function->func_decl.return_type;
          close_scope(current_scope);
          current_scope--;
          if ((return_type == TYPE_INT || return_type == TYPE_CHAR) &&
              !always_returns((yyvsp[-1].node))) {
              SEM_ERR("[ERRO SEMANTICO] Funcao '%s' pode terminar sem retornar um valor.\n",
                      function->func_decl.id);
          }
          function->func_decl.body = ast_new_block((yyvsp[-1].node));
          current_function_return_type = TYPE_UNKNOWN;
          (yyval.node) = function;
      }
#line 1967 "parser.tab.c"
    break;

  case 47: /* $@2: %empty  */
#line 541 "parser.y"
                     { current_scope++; current_function_signature_valid = 1; }
#line 1973 "parser.tab.c"
    break;

  case 48: /* func_header: type ID LPAREN $@2 param_list RPAREN  */
#line 542 "parser.y"
                        {
          (yyval.node) = ast_new_func_decl((yyvsp[-4].id), (yyvsp[-5].data_type), (yyvsp[-1].node), NULL);
          free((yyvsp[-4].id));
      }
#line 1982 "parser.tab.c"
    break;

  case 49: /* param_list: %empty  */
#line 549 "parser.y"
                             { (yyval.node) = NULL; }
#line 1988 "parser.tab.c"
    break;

  case 50: /* param_list: param  */
#line 550 "parser.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 1994 "parser.tab.c"
    break;

  case 51: /* param_list: param_list COMMA param  */
#line 551 "parser.y"
                             { (yyval.node) = ast_new_param_list((yyvsp[-2].node), (yyvsp[0].node)); }
#line 2000 "parser.tab.c"
    break;

  case 52: /* param: type ID  */
#line 555 "parser.y"
              {
          Symbol *s = NULL;
          if ((yyvsp[-1].data_type) == TYPE_VOID) {
              SEM_ERR("[ERRO SEMANTICO] Parametro '%s' nao pode ter tipo void.\n", (yyvsp[0].id));
              current_function_signature_valid = 0;
          } else {
              s = insert_symbol((yyvsp[0].id), (yyvsp[-1].data_type), 0, 0, current_scope);
          }
          if ((yyvsp[-1].data_type) != TYPE_VOID && !s) {
              SEM_ERR("[ERRO SEMANTICO] Parametro '%s' duplicado no escopo %d.\n", (yyvsp[0].id), current_scope);
              current_function_signature_valid = 0;
          }
          (yyval.node) = ast_new_param((yyvsp[0].id), (yyvsp[-1].data_type));
          free((yyvsp[0].id));
      }
#line 2020 "parser.tab.c"
    break;

  case 53: /* args_list: %empty  */
#line 573 "parser.y"
                             { (yyval.node) = NULL; }
#line 2026 "parser.tab.c"
    break;

  case 54: /* args_list: expr  */
#line 574 "parser.y"
                             { (yyval.node) = ast_new_args_list(NULL, (yyvsp[0].node)); }
#line 2032 "parser.tab.c"
    break;

  case 55: /* args_list: args_list COMMA expr  */
#line 575 "parser.y"
                             { (yyval.node) = ast_new_args_list((yyvsp[-2].node), (yyvsp[0].node)); }
#line 2038 "parser.tab.c"
    break;

  case 56: /* $@3: %empty  */
#line 579 "parser.y"
             { current_scope++; }
#line 2044 "parser.tab.c"
    break;

  case 57: /* block_stmt: LBRACE $@3 program RBRACE  */
#line 579 "parser.y"
                                                 {
          close_scope(current_scope);
          current_scope--;
          (yyval.node) = ast_new_block((yyvsp[-1].node));
      }
#line 2054 "parser.tab.c"
    break;

  case 58: /* if_stmt: IF LPAREN expr RPAREN stmt  */
#line 587 "parser.y"
                                                       {
        if (!(yyvsp[-2].node) || ((yyvsp[-2].node)->expr_type != TYPE_BOOL && (yyvsp[-2].node)->expr_type != TYPE_INT)) {
           SEM_ERR("[ERRO SEMANTICO] A condiÃ§Ã£o do if deve ser booleana ou inteira\n");
        }
        (yyval.node) = ast_new_if((yyvsp[-2].node), (yyvsp[0].node));
      }
#line 2065 "parser.tab.c"
    break;

  case 59: /* if_stmt: IF LPAREN expr RPAREN stmt ELSE stmt  */
#line 593 "parser.y"
                                           {
        if (!(yyvsp[-4].node) || ((yyvsp[-4].node)->expr_type != TYPE_BOOL && (yyvsp[-4].node)->expr_type != TYPE_INT)) {
           SEM_ERR("[ERRO SEMANTICO] A condiÃ§Ã£o do if deve ser booleana ou inteira\n");
        }
        (yyval.node) = ast_new_if_else((yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
      }
#line 2076 "parser.tab.c"
    break;

  case 60: /* while_stmt: WHILE LPAREN expr RPAREN stmt  */
#line 602 "parser.y"
                                    {
        if (!(yyvsp[-2].node) || ((yyvsp[-2].node)->expr_type != TYPE_BOOL && (yyvsp[-2].node)->expr_type != TYPE_INT)) {
           SEM_ERR("[ERRO SEMANTICO] A condiÃ§Ã£o do while deve ser booleana ou inteira\n");
        }
        (yyval.node) = ast_new_while((yyvsp[-2].node), (yyvsp[0].node));
      }
#line 2087 "parser.tab.c"
    break;

  case 61: /* $@4: %empty  */
#line 611 "parser.y"
                 { current_scope++; }
#line 2093 "parser.tab.c"
    break;

  case 62: /* for_stmt: FOR LPAREN $@4 for_init for_condition SEMI for_update RPAREN stmt  */
#line 611 "parser.y"
                                                                                         {
        close_scope(current_scope);
        current_scope--;
        if ((yyvsp[-4].node) && (yyvsp[-4].node)->expr_type != TYPE_BOOL && (yyvsp[-4].node)->expr_type != TYPE_INT) {
            SEM_ERR("[ERRO SEMANTICO] Condicao do for deve ser booleana ou inteira.\n");
        }
        (yyval.node) = ast_new_for((yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
      }
#line 2106 "parser.tab.c"
    break;

  case 63: /* for_init: SEMI  */
#line 622 "parser.y"
           { (yyval.node) = NULL; }
#line 2112 "parser.tab.c"
    break;

  case 64: /* for_init: decl_stmt  */
#line 623 "parser.y"
                { (yyval.node) = (yyvsp[0].node); }
#line 2118 "parser.tab.c"
    break;

  case 65: /* for_init: assign_stmt  */
#line 624 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 2124 "parser.tab.c"
    break;

  case 66: /* for_init: expr SEMI  */
#line 625 "parser.y"
                { (yyval.node) = (yyvsp[-1].node); }
#line 2130 "parser.tab.c"
    break;

  case 67: /* for_init: type ID ASSIGN expr SEMI  */
#line 626 "parser.y"
                               {
        Symbol *s = NULL;
        if ((yyvsp[-4].data_type) == TYPE_VOID) {
            SEM_ERR("[ERRO SEMANTICO] Variavel do for nao pode ter tipo void.\n");
        } else {
            s = insert_symbol((yyvsp[-3].id), (yyvsp[-4].data_type), 0, 0, current_scope);
        }
        if (!s || !(yyvsp[-1].node)) {
            if ((yyvsp[-4].data_type) != TYPE_VOID)
                SEM_ERR("[ERRO SEMANTICO] Declaracao inicial invalida para '%s' no for.\n", (yyvsp[-3].id));
            (yyval.node) = NULL;
        } else if (s->type != (yyvsp[-1].node)->expr_type) {
            SEM_ERR("[ERRO SEMANTICO] Tipo incompativel na inicializacao de '%s' no for.\n", (yyvsp[-3].id));
            (yyval.node) = NULL;
        } else {
            (yyval.node) = ast_new_seq(ast_new_decl((yyvsp[-3].id), (yyvsp[-4].data_type)), ast_new_assign((yyvsp[-3].id), (yyvsp[-1].node)));
        }
        free((yyvsp[-3].id));
    }
#line 2154 "parser.tab.c"
    break;

  case 68: /* for_condition: %empty  */
#line 648 "parser.y"
             { (yyval.node) = NULL; }
#line 2160 "parser.tab.c"
    break;

  case 69: /* for_condition: expr  */
#line 649 "parser.y"
           { (yyval.node) = (yyvsp[0].node); }
#line 2166 "parser.tab.c"
    break;

  case 70: /* for_update: %empty  */
#line 653 "parser.y"
             { (yyval.node) = NULL; }
#line 2172 "parser.tab.c"
    break;

  case 71: /* for_update: expr  */
#line 654 "parser.y"
           { (yyval.node) = (yyvsp[0].node); }
#line 2178 "parser.tab.c"
    break;

  case 72: /* for_update: ID ASSIGN expr  */
#line 655 "parser.y"
                     {
        (yyval.node) = make_for_assignment((yyvsp[-2].id), (yyvsp[0].node));
        free((yyvsp[-2].id));
    }
#line 2187 "parser.tab.c"
    break;

  case 73: /* for_update: ID LBRACK expr RBRACK ASSIGN expr  */
#line 659 "parser.y"
                                        {
        Symbol *s = lookup_symbol((yyvsp[-5].id), current_scope);
        int valid = s && s->is_array && (yyvsp[0].node) && s->type == (yyvsp[0].node)->expr_type;
        if (!s || !s->is_array)
            SEM_ERR("[ERRO SEMANTICO] Alvo de vetor invalido no incremento do for.\n");
        if (s && s->is_array && !validate_array_index(s, (yyvsp[-3].node), (yyvsp[-5].id)))
            valid = 0;
        if (s && s->is_array && (yyvsp[0].node) && s->type != (yyvsp[0].node)->expr_type) {
            SEM_ERR("[ERRO SEMANTICO] Tipo incompativel no incremento do for.\n");
            valid = 0;
        }
        (yyval.node) = valid ? ast_new_array_assign((yyvsp[-5].id), (yyvsp[-3].node), (yyvsp[0].node)) : NULL;
        free((yyvsp[-5].id));
    }
#line 2206 "parser.tab.c"
    break;


#line 2210 "parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 674 "parser.y"


int main() {
    symtab_init();
    printf("Iniciando parser com Tabela de Simbolos e AST...\n");
    yyparse();

    printf("\n=== ARVORE SINTATICA ABSTRATA (AST) ===\n");
    ast_print(ast_root, 0);
    printf("=======================================\n");

    print_symtab();

    // Geracao de codigo intermediario: so ocorre se a entrada nao tiver erros.
    int total_errors = syntax_error_count + semantic_error_count;
    if (total_errors > 0) {
        fprintf(stderr, "[AVISO] Codigo intermediario nao gerado: %d erro(s) encontrado(s).\n", total_errors);
    } else {
        IRProgram *ir = ir_generate(ast_root);
        if (ir->errors == 0) {
            printf("\n=== CODIGO INTERMEDIARIO (TRES ENDERECOS) ===\n");
            ir_print(ir, stdout);
            printf("=============================================\n");

            // Geracao de codigo final: traduz o codigo intermediario para Assembly.
            AsmProgram *code = codegen_generate(ir);
            if (code->errors == 0) {
                printf("\n=== CODIGO FINAL (ASSEMBLY RISC-V) ===\n");
                codegen_print(code, stdout);
                printf("======================================\n");
            } else {
                fprintf(stderr, "[AVISO] Codigo final nao gerado: %d erro(s) na geracao.\n", code->errors);
            }
            codegen_free(code);
        } else {
            fprintf(stderr, "[AVISO] Codigo intermediario nao gerado: %d erro(s) na geracao.\n", ir->errors);
        }
        ir_free(ir);
    }
    return 0;
}

void yyerror(const char *s) {
    syntax_error_count++;
    fprintf(stderr, "erro sintatico: %s\n", s);
}

int evaluate_constant_int(ASTNode *node, int *value) {
    if (!node || !value) return 0;
    if (node->type == AST_NUM) {
        *value = node->intval;
        return 1;
    }
    if (node->type == AST_UNOP && node->unop.op == MINUS) {
        int operand;
        if (!evaluate_constant_int(node->unop.expr, &operand) || operand == INT_MIN)
            return 0;
        *value = -operand;
        return 1;
    }
    if (node->type != AST_BINOP) return 0;

    int left, right;
    if (!evaluate_constant_int(node->binop.left, &left) ||
        !evaluate_constant_int(node->binop.right, &right))
        return 0;

    long long result;
    switch (node->binop.op) {
        case PLUS:  result = (long long) left + right; break;
        case MINUS: result = (long long) left - right; break;
        case TIMES: result = (long long) left * right; break;
        case DIVIDE:
            if (right == 0 || (left == INT_MIN && right == -1)) return 0;
            result = left / right;
            break;
        case MOD:
            if (right == 0 || (left == INT_MIN && right == -1)) return 0;
            result = left % right;
            break;
        default:
            return 0;
    }
    if (result < INT_MIN || result > INT_MAX) return 0;
    *value = (int) result;
    return 1;
}

int validate_array_index(Symbol *array, ASTNode *index, const char *name) {
    if (!array || !array->is_array || !index || index->expr_type != TYPE_INT) {
        SEM_ERR("[ERRO SEMANTICO] O indice de '%s' deve ser uma expressao inteira.\n", name);
        return 0;
    }

    int constant_index;
    if (evaluate_constant_int(index, &constant_index) &&
        (constant_index < 0 || constant_index >= array->array_size)) {
        SEM_ERR("[ERRO SEMANTICO] Indice %d fora dos limites do vetor '%s'.\n", constant_index, name);
        return 0;
    }
    return 1;
}

static int validate_array_initializer_items(ASTNode *items, DataType element_type,
                                            int array_size, const char *name,
                                            int *count) {
    if (!items) return 1;
    int valid = 1;
    if (items->type == AST_ARGS_LIST) {
        valid &= validate_array_initializer_items(items->args_list.next, element_type,
                                                  array_size, name, count);
        ASTNode *item = items->args_list.expr;
        (*count)++;
        if (*count > array_size) {
            SEM_ERR("[ERRO SEMANTICO] Inicializador do vetor '%s' tem mais elementos que o tamanho %d.\n",
                    name, array_size);
            valid = 0;
        }
        if (!item || item->expr_type != element_type) {
            SEM_ERR("[ERRO SEMANTICO] Tipo incompativel no elemento %d do vetor '%s'.\n",
                    *count, name);
            valid = 0;
        }
        return valid;
    }

    (*count)++;
    if (*count > array_size) {
        SEM_ERR("[ERRO SEMANTICO] Inicializador do vetor '%s' tem mais elementos que o tamanho %d.\n",
                name, array_size);
        valid = 0;
    }
    if (items->expr_type != element_type) {
        SEM_ERR("[ERRO SEMANTICO] Tipo incompativel no elemento %d do vetor '%s'.\n",
                *count, name);
        valid = 0;
    }
    return valid;
}

int validate_array_initializer(ASTNode *items, DataType element_type,
                               int array_size, const char *name) {
    int count = 0;
    return validate_array_initializer_items(items, element_type, array_size, name, &count);
}
