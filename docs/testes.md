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