# Git Cheatsheet

## Setup
```bash
git config --global user.name "Alice"
git config --global user.email "alice@x.com"
git config --global init.defaultBranch main
git config --global core.editor "nano"
git config --global pull.rebase true
git config --global rebase.autoStash true
```

## Estado
```bash
git status              # working tree
git status -sb          # short + branch
git diff                # unstaged
git diff --staged       # staged
git diff branch1..branch2
git log --oneline --graph --decorate --all
git log -p file.txt     # patch por commit
git show HEAD~2         # commit n-2
git blame file.txt      # autoria por linha
```

## Stage / commit
```bash
git add file.txt
git add -p              # interativo (chunks)
git add -u              # só já-tracked modificados
git commit -m "msg"
git commit --amend      # edita último commit
git commit --fixup HEAD~2 && git rebase -i --autosquash HEAD~3
```

## Branching
```bash
git branch                          # listar
git branch foo                      # criar
git checkout -b foo                 # criar + trocar
git switch foo                      # trocar (novo)
git switch -c foo                   # criar + trocar (novo)
git branch -d foo                   # deletar (merged)
git branch -D foo                   # forçar
git branch -m old new               # rename
```

## Merge / rebase
```bash
git merge feature                   # merge fast-forward se possível
git merge --no-ff feature           # sempre cria commit de merge
git rebase main                     # replay branch atual em cima de main
git rebase -i HEAD~5                # interactive (squash, fixup, edit)
git rebase --abort                  # cancela
git rebase --continue               # depois de resolver conflitos
git cherry-pick abc1234             # aplica um commit avulso
```

## Remotes
```bash
git remote -v
git remote add origin git@github.com:user/repo.git
git fetch origin
git pull --ff-only                  # não cria merge commit
git push origin main
git push -u origin feature          # set upstream
git push --force-with-lease         # safe force-push
```

## Stash
```bash
git stash                           # salva mudanças
git stash -u                        # inclui untracked
git stash list
git stash show -p stash@{0}
git stash pop                       # aplica + remove
git stash apply stash@{1}           # aplica sem remover
git stash drop stash@{0}
```

## Undo
```bash
git restore file.txt                # descarta mudanças não-staged
git restore --staged file.txt       # un-stage (mantém alterações)
git reset HEAD~1                    # desfaz último commit, mantém arquivos
git reset --hard HEAD~1             # desfaz commit + descarta tudo (perigo!)
git revert abc1234                  # cria commit que reverte
git reflog                          # histórico de HEAD (pra recuperar)
```

## Tags
```bash
git tag                             # listar
git tag v1.0                        # leve
git tag -a v1.0 -m "release"        # anotada
git push origin v1.0
git push origin --tags
```

## Search
```bash
git log --all --grep="bug"          # mensagens
git log -S "function_x"             # commits que adicionam/removem string
git log -G "regex"                  # commits cujo diff casa regex
git log --author="alice"
git log --since="1 week ago"
git bisect start; git bisect bad; git bisect good v1.0
```

## Submodules
```bash
git submodule add URL path
git submodule update --init --recursive
git submodule foreach git pull origin main
```

## Worktrees
```bash
git worktree add ../feature-x feature
git worktree list
git worktree remove ../feature-x
```

## Hooks frequentes
```
.git/hooks/pre-commit          # antes do commit (linter, format)
.git/hooks/commit-msg          # valida mensagem
.git/hooks/pre-push            # antes do push (testes)
```

## .gitignore patterns
```
*.log               # qualquer .log
build/              # diretório
!important.log      # exceção
**/temp             # qualquer temp em qualquer profundidade
```

## Aliases úteis
```bash
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.ci commit
git config --global alias.st status
git config --global alias.lg "log --oneline --graph --decorate --all"
```
