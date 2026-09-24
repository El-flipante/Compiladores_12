# Compilador de C -> Assembly

Este documento descreve as especificações do subconjunto da linguagem C que será aceito pelo nosso compilador, cujo objetivo final é a tradução para código Assembly. O documento também possui os testes realizados até o momento atual. No versionamento atual encontra-se implementado o analizador lexico (a principio completo, porem pode sofrer revisões) e o analisador sintatico completo

---

## Equipe

<div align="center">

<table>
<tr>
<td align="center">
<a href="https://github.com/El-flipante">
<img src="https://github.com/El-flipante.png?size=120" width="120px" alt="Alexandre Henrique"/><br>
<sub><b>Alexandre Henrique</b></sub><br>
</a>
</td>

<td align="center">
<a href="https://github.com/christianrolim">
<img src="https://github.com/christianrolim.png?size=120" width="120px" alt="Davi Sakai"/><br>
<sub><b>Christiam Rolim</b></sub><br>
</a>
</td>

<td align="center">
<a href="https://github.com/igorlym">
<img src="https://github.com/igorlym.png?size=120" width="120px" alt="Igor Lima"/><br>
<sub><b>Igor Lima</b></sub><br>
</a>
</td>

<td align="center">
<a href="https://github.com/joaorolim-code">
<img src="https://github.com/joaorolim-code.png?size=120" width="120px" alt="João Rolim"/><br>
<sub><b>João Rolim</b></sub><br>
</a>
</td>
</tr>

<td align="center">
<a href="https://github.com/LucasPeixotoRodrigues">
<img src="https://github.com/LucasPeixotoRodrigues.png?size=120" width="120px" alt="Lucas Peixoto"/><br>
<sub><b>Lucas Peixoto</b></sub><br>
</a>
</td>

</table>

</div>

---

# Tecnologias Utilizadas

| Tecnologia | Utilização |
|------------|------------|
| C | Linguagem principal do compilador |
| Assembly | Linguagem final do compilador |
| Flex | Geração do Analisador Léxico |
| Bison | Geração do Analisador Sintático  |
| WSL | Bash para Windows |
| Git | Versionamento |
| Markdown | Documentação |

---

# Documentação da Linguagem: Subconjunto C

## 1. Elementos Léxicos (Tokens)

Nossa linguagem reconhece as seguintes categorias de tokens:

### 1.1. Palavras Reservadas (Keywords)
As seguintes palavras são reservadas e não podem ser usadas como identificadores:
* `int`, `char`, `void`, `return`, `if`, `else`, `for`, `while`

### 1.2. Identificadores
Nomes de variáveis e funções. 
* **Regra:** Devem começar com uma letra (a-z, A-Z) ou sublinhado (`_`), seguidos por zero ou mais letras, dígitos (0-9) ou sublinhados.

### 1.3. Literais
* **Inteiros:** Sequência de um ou mais dígitos de `0` a `9`.
* **Caracteres:** Um único caractere delimitado por aspas simples (ex: `'a'`, `'X'`, `'5'`).

### 1.4. Operadores e Símbolos Especiais
| Categoria | Símbolos |
| :--- | :--- |
| Aritméticos | `+`, `-`, `*`, `/` |
| Relacionais | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Atribuição | `=` |
| Delimitadores | `(`, `)`, `{`, `}`, `[`, `]`, `;`, `,` |

### 1.5. Comentários
* **Linha única:** Iniciados por `//` até o fim da linha.
* **Múltiplas linhas:** Delimitados por `/*` e `*/`. O conteúdo interno será ignorado pelo analisador léxico.

---

## 2. Estruturas Sintáticas

### 2.1. Declaração de Variáveis e Vetores
Serão suportadas declarações simples (com ou sem inicialização imediata) e declarações de vetores unidimensionais de tamanho fixo.
* **Tipos básicos:** `int x;`, `int y = 10;`, `char letra = 'a';`
* **Vetores (Arrays):** `int numeros[10];`, `char palavra[20];` *(Nota: a inicialização de vetores no momento da declaração, como `int v[2] = {1, 2}`, não será suportada nesta versão).*

### 2.2. Estruturas de Controle de Fluxo
* **Condicionais:** Estrutura `if` e `if-else`.
* **Laços de Repetição:** Estruturas `for` e `while`.

### 2.3. Funções
O programa deve conter ao menos a função principal.
* **Sintaxe:** `int main() { ... }`

---

## 3. Exemplos de Código

### 3.1. Exemplo Válido 1: Estruturas de Repetição
O código abaixo demonstra um programa válido calculando um fatorial:

```c
// Programa que calcula o fatorial de 5
int main() {
    int n = 5;
    int resultado = 1;
    
    while (n > 0) {
        resultado = resultado * n;
        n = n - 1;
    }
    
    return resultado;
}
```

---

# Como Executar o Projeto

## Pré-requisitos

Antes de iniciar, certifique-se de possuir instalado: 

Bison, Flex, GCC, WSL(caso use Windows)/Linux 



---

## 1. Clonar o Repositório

```bash
git clone https://github.com/El-flipante/Compiladores_12

cd Compiladores_12
```

---

## 2. Compilar o analizador léxico

```bash
flex lexer.l
```

---

## 3. Compilar o analizador sintático

```bash
bison -d parser.y
```
## 4. Compilar tudo

```bash
gcc -o compilador parser.tab.c lexer.yy.c -lfl
```
---
## 5. Executar o compilador
``` bash
./compilador
```
## Estrutura do Projeto

```text
.Compiladores_12
├── Professor
├── README.md
├── parser.y
├── lexer.l
```
# Teste
Foram implementados testes para verificar o funcionamento do analisador léxico e sintático, contemplando operações aritméticas, números negativos, uso de parênteses, precedência de operadores, divisão por zero e entradas inválidas.

Os testes foram divididos em **casos válidos** e **casos inválidos**, permitindo verificar tanto o reconhecimento correto das expressões quanto o tratamento de erros.

### 1. Testes de expressões aritméticas válidas

Os testes a seguir verificam operações básicas de soma, subtração, multiplicação e divisão.

```text
5 + 3;
5 - 3;
10 * 3;
10 * 10;
10 / 2;
0 / 5;
```

### 2. Testes com números negativos

Verificam o reconhecimento do operador unário de negação e sua utilização em operações aritméticas.

```text
10 * -5;
-6 * 3;
10 / -2;
-15 / 3;
-15 + 1*4;
-15 - 1*4;
```

### 3. Testes com números negativos entre parênteses

Verificam o tratamento de valores negativos quando utilizados dentro de expressões agrupadas.

```text
10 * (-5);
10 / (-2);
(-6) * 3;
(-15) / 3;
(-15) - 1*4;
(-15) + 1*4;
```

### 4. Testes de precedência e associação de operadores

Verificam se o analisador respeita corretamente a precedência dos operadores aritméticos e o agrupamento definido pelos parênteses.

```text
4 - 3*15;
-15 - 1*4;
(-15) - 1*4;
(-15 - 4)*3;
((-15) - 4)*3;
-(15 - 5)*3;
-15 + 1*4;
(-15) + 1*4;
10 / (5-3);
10 / (1*5);
-(14*33);
```

### 5. Testes com operador de módulo

Verificam o reconhecimento e o funcionamento do operador `%`.

```text
3 % 14;
14 % 3;
```

### 6. Testes de operadores unários

Verificam a utilização isolada dos operadores `+` e `-` como operadores unários.

```text
+ 33;
- 45;
- 33;
```

### 7. Teste de divisão por zero

O teste abaixo verifica o comportamento do compilador diante de uma operação de divisão por zero.

```text
5 / 0;
```
a saida de erro específica desse teste será implementada junto com o analisador semântico
### 8. Testes de entradas inválidas

Os casos abaixo foram utilizados para verificar o tratamento de caracteres, operadores e estruturas que não pertencem à linguagem definida.

#### 8.1 Caracteres inválidos

```text
@;
.
.
22 ¨ 44;
700 @ 14;
```

#### 8.2 Identificadores ou palavras não reconhecidas

```text
x;
y - 14;
14 * z;
teste;
123erro;
554aiaiai;
```

#### 8.3 Operadores ou símbolos não suportados

```text
15 = 60;
15 = 60/2;
% 14;
* 15;
/ 22;
11 - 15.
```

#### 8.4 Expressões incompletas ou estruturalmente inválidas

```text
1
2
3

+ 10;

1

(114*3;
114*3);

44 55 73;

2 + - 7;
```

Esses casos permitem verificar se o analisador é capaz de detectar expressões que não obedecem à gramática definida.

### 9. Resumo dos testes

| Categoria                        | Objetivo                                         |
| -------------------------------- | ------------------------------------------------ |
| Operações básicas                | Verificar `+`, `-`, `*` e `/`                    |
| Números negativos                | Verificar operadores unários e valores negativos |
| Parênteses                       | Verificar agrupamento de expressões              |
| Precedência                      | Verificar a ordem de avaliação dos operadores    |
| Módulo                           | Verificar o operador `%`                         |
| Divisão por zero                 | Verificar tratamento de operação inválida        |
| Caracteres inválidos             | Verificar erros léxicos                          |
| Identificadores não reconhecidos | Verificar tokens não previstos                   |
| Operadores inválidos             | Verificar símbolos não pertencentes à linguagem  |
| Expressões inválidas             | Verificar erros sintáticos                       |
# Licença

Este projeto foi desenvolvido para a disciplina **Compiladores 1** da Universidade de Brasília (UnB).
