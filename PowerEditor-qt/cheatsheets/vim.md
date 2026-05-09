# Vim Cheatsheet

## Modos
```
ESC      normal
i / I    insert / insert início linha
a / A    append / append fim linha
o / O    nova linha abaixo / acima
v / V    visual char / visual line
Ctrl+v   visual block
:        command
R        replace
```

## Movimento
```
h j k l       ← ↓ ↑ →
w / W         próxima palavra (W ignora pontuação)
b / B         palavra anterior
e / E         fim da palavra
0 / ^         início da linha / 1ª não-branco
$             fim da linha
gg            início do arquivo
G             fim do arquivo
:42           linha 42
H / M / L     topo / meio / fundo da tela
%             match { } [ ] ( )
*             próxima ocorrência da palavra sob cursor
#             anterior
fX            próximo char X (FX para trás)
tX            até antes do char X
```

## Edit
```
x             delete char
dd            delete linha
yy            yank linha (copiar)
p / P         paste depois / antes
u             undo
Ctrl+r        redo
.             repete último comando
~             toggle case
J             junta linha de baixo
>>            indenta linha
<<            desindenta
```

## Operadores + movimento
```
d{motion}     delete (dw=word, d$=fim, di"=inside ", da(=around ()
c{motion}     change (idem + entra em insert)
y{motion}     yank
g~/gu/gU      toggle/lower/upper case
=             auto-indent
```

## Text objects
```
iw / aw       inner / around word
is / as       inner / around sentence
ip / ap       inner / around paragraph
i" i' i`      inside aspas
i( i[ i{ i<   inside delimitadores
it / at       inside / around tag (HTML)
```

## Search & replace
```
/pattern        busca à frente
?pattern        busca para trás
n / N           próximo / anterior
:s/old/new/     substitui na linha
:s/old/new/g    todas na linha
:%s/old/new/g   todas no arquivo
:%s/old/new/gc  com confirmação
:%s/\<word\>/x/g  palavra inteira
```

## Janelas / buffers / tabs
```
:e file         abre arquivo
:bn / :bp       próximo / anterior buffer
:b name         buffer por nome
:bd             fecha buffer
:sp / :vsp      split horizontal / vertical
Ctrl+w h/j/k/l  navega entre splits
Ctrl+w =        equaliza
:tabnew / :tabe próxima tab
gt / gT         próxima/anterior tab
```

## Marcas / registros
```
ma              marca local 'a'
'a              pula para marca a
"ay             yank pra registro a
"ap             paste registro a
"+y / "+p       clipboard do sistema
:reg            mostra todos
```

## Macros
```
qa              grava macro em 'a'
q               para gravação
@a              executa macro a
@@              repete último @
10@a            10 vezes
```

## Selection
```
gv              re-seleciona última visual
o               troca extremo do visual
==              indenta seleção (em visual)
:'<,'>          range = visual atual
```

## Comandos úteis
```
:w              save
:wa             save all
:q / :qa        quit / quit all
:wq / :x        save + quit
:!cmd           shell command
:r !cmd         lê output de cmd
:set number     mostra linhas
:set rnu        relative line numbers
:set ic / sm    case-insensitive search / smart
:noh            limpa highlight
```

## Plugins essenciais (vim-plug)
```vim
call plug#begin()
Plug 'tpope/vim-surround'        " ys, cs, ds
Plug 'tpope/vim-fugitive'        " :Git
Plug 'tpope/vim-commentary'      " gcc
Plug 'junegunn/fzf.vim'
Plug 'preservim/nerdtree'
Plug 'neoclide/coc.nvim'
call plug#end()
```
