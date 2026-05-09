# Regex Cheatsheet (PCRE / JS)

## Anchors
```
^      início de linha (ou string sem multiline)
$      fim de linha
\A     início da string
\z     fim da string
\b     borda de palavra
\B     não-borda
```

## Quantificadores
```
*      0 ou mais
+      1 ou mais
?      0 ou 1
{n}    exatamente n
{n,}   pelo menos n
{n,m}  entre n e m
*?     lazy (não-greedy)
+?     idem
```

## Classes
```
.      qualquer char (exceto \n por padrão)
\d     dígito        \D não-dígito
\w     [A-Za-z0-9_]  \W oposto
\s     whitespace    \S oposto
[abc]  qualquer de a, b ou c
[^abc] nada disso
[a-z]  range
```

## Grupos
```
(abc)         grupo capturador (1)
(?:abc)       não-capturador
(?<name>abc)  capturador nomeado
\1            backreference ao grupo 1
\k<name>      backreference nomeada
```

## Lookaround
```
(?=...)   lookahead positivo
(?!...)   lookahead negativo
(?<=...)  lookbehind positivo
(?<!...)  lookbehind negativo
```

## Flags comuns
```
g    global (encontra todos)
i    case-insensitive
m    multiline ($/^ por linha)
s    dotall (. casa \n)
x    extended (ignora whitespace + comentários)
u    Unicode
```

## Receitas úteis
```regex
^\s*$                                  linha vazia
^[A-Z][a-z]+$                          palavra capitalizada
^\d{2,4}-\d{2}-\d{2}$                  data ISO
^[\w\.-]+@[\w\.-]+\.\w+$               email simples
^https?://[^\s]+$                      URL simples
^\d{3}\.\d{3}\.\d{3}-\d{2}$            CPF formatado
^\d{2}\.\d{3}\.\d{3}/\d{4}-\d{2}$      CNPJ formatado
^\(\d{2}\)\s*9?\d{4}-\d{4}$            telefone BR
\b[0-9a-fA-F]{6,8}\b                   hex color
\b[A-F0-9]{8}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{12}\b   UUID
```

## Substituição
```
$1, $2          grupos capturados
${name}         capturador nomeado (PCRE/Python)
\u, \l          uppercase/lowercase próximo char (sed/Vim)
```
