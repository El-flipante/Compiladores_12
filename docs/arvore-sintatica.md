# Exemplo de Árvore Sintática Abstrata

Esta página mostra como o compilador representa uma expressão na Árvore Sintática Abstrata (AST). O exemplo completo está em [`exemplos/arvore_precedencia.c`](./exemplos/arvore_precedencia.c):

```c
int main() {
    int x;
    x = 1 + 2 * 3;
    return x;
}
```

## Saída da AST

Para imprimir apenas o trecho da AST, execute o compilador a partir da pasta `src`:

```bash
make
./parser < ../docs/exemplos/arvore_precedencia.c \
  | sed -n '/^=== ARVORE SINTATICA ABSTRATA (AST) ===$/,/^=======================================$/p'
```

A saída correspondente é:

```text
=== ARVORE SINTATICA ABSTRATA (AST) ===
FUNC_DECL(main)
  BODY:
    DECLARATION(x)
    ASSIGN(x)
      OP(262)
        NUM(1)
        OP(264)
          NUM(2)
          NUM(3)
    RETURN
      ID(x)
=======================================
```

## Como ler a árvore

* A indentação mostra a relação entre os nós: os itens mais recuados pertencem ao nó acima deles.
* `FUNC_DECL(main)` é a função; `BODY` contém as instruções do bloco.
* `DECLARATION(x)` representa a declaração da variável, e `ASSIGN(x)` representa sua atribuição.
* `OP(262)` é a soma (`+`) e `OP(264)` é a multiplicação (`*`) nesta versão gerada pelo Bison. Os números são códigos internos dos tokens e podem mudar se a lista de tokens mudar.
* A soma é o nó externo e a multiplicação aparece dentro do seu ramo direito. Isso mostra que `2 * 3` é agrupado antes de ser somado a `1`, conforme a precedência dos operadores.
* `RETURN` contém `ID(x)`, que representa o uso da variável no retorno.

O compilador imprime a AST junto com outras informações. O comando acima usa `sed` para exibir somente o bloco delimitado pelos cabeçalhos da árvore.
