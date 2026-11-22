# ocsh : mini-shell en C

Projet semestriel **Systèmes d'Exploitation** (UIR / ESIN, Pr. Ilias Tougui) — **Projet 3 : `ocsh`**.

`ocsh` est un interpréteur de commandes écrit en C qui appelle directement les appels système du noyau
(`fork`, `execvp`, `wait`, `pipe`, `dup2`, signaux) et reproduit le cœur de Bash avec des commandes renommées
et l'opérateur de pipe `->`.

**Groupe :** Ismail, Omar, Abderrahmane, Ayman.

## Prérequis

- Linux (Ubuntu/Debian recommandé), `gcc`, `make`
- Optionnel mais recommandé (historique avec les flèches + complétion par Tab) :

```bash
sudo apt install build-essential libreadline-dev
```

Sans `libreadline-dev`, la compilation fonctionne quand même (lecture avec `getline`, sans édition de ligne).

## Compilation et exécution

```bash
cd code
make              # produit ./ocsh
./ocsh            # shell interactif, prompt "ocsh> "
./ocsh script.ocsh   # mode script
make test         # tests du parser + tests de fumée
make clean
```

Options : `make READLINE=0` force la compilation sans readline. `./ocsh --help` affiche l'aide.
On peut aussi alimenter le shell par l'entrée standard : `printf 'echo salut\n' | ./ocsh`.

## Commandes

| Bash | ocsh | Type |
|---|---|---|
| `cd` | `go` | builtin |
| `exit` | `quit` | builtin |
| `pwd` | `path` | builtin |
| `echo` | `say` | builtin |
| `help` | `info` | builtin |
| `history` | `past` | builtin |
| *(spécifique)* | `osinfo` | builtin : affiche « Systèmes d'Exploitation -- UIR/ESIN » |
| `ls` / `cat` / `cp` | `list` / `show` / `copy` | alias |
| `mv` / `rm` | `move` / `del` | alias |
| `mkdir` / `touch` | `newdir` / `newfile` | alias |
| pipe `\|` | pipe `->` | opérateur |

Opérateurs : `->` (pipe), `>` `>>` `<` (redirections), `&` (arrière-plan), `$VAR` `${VAR}` `$?` `$!`,
`$(...)` et `` `...` `` (substitution), `#` (commentaire).

## Exemples

```
ocsh> list
ocsh> go ..
ocsh> path
ocsh> say hello > f.txt
ocsh> show f.txt
ocsh> show f.txt -> wc -l
ocsh> sleep 10 &
ocsh> osinfo
```

## Architecture

Chaque ligne suit le même chemin dans `execute_line()` (`main.c`) :

```
ligne lue (main.c)
  -> try_assignment      X=5                      (Ayman)
  -> command_subst       $(...) et `...`          (Abderrahmane)
  -> expand_vars         $VAR ${VAR} $? $!        (Ayman)
  -> parse_line          texte -> Pipeline        (Ismail)
  -> execute_pipeline    fork / pipe / dup2 / wait (Abderrahmane)
        -> run_builtin_redir  si builtin seul   (Omar)
        -> child_exec         redirs + alias + execvp (Ismail / Omar)
```

La structure `Pipeline` (voir `code/shell.h`) est le contrat entre les modules :
un `Pipeline` contient jusqu'à 16 `Command` (séparées par `->`), chacune avec `argv`, `infile`, `outfile`, `append`.

| Fichier | Rôle | Responsable |
|---|---|---|
| `shell.h` | contrat commun (structures + prototypes) | Ismail |
| `main.c` | boucle REPL, `execute_line` | Ismail |
| `parser.c` | tokenizer + découpage par `->`, redirections, `&` | Ismail |
| `exec.c` | `child_exec` (execvp), `exec_simple`, codes de retour | Ismail |
| `rc.c` | chargement de `~/.ocshrc` | Ismail |
| `builtins.c`, `alias.c`, `redirect.c`, `history.c` | builtins, alias, redirections (dup2), historique | Omar |
| `pipeline.c`, `signals.c`, `subst.c`, `jobs.c` | pipes, `&`, SIGINT/SIGCHLD, `$(...)` | Abderrahmane |
| `expand.c`, `vars.c`, `script.c`, `tests/` | variables, `export`, scripts if/for/while | Ayman |
| `stubs.c` | versions provisoires (*weak*) des fonctions pas encore écrites | Ismail |

`stubs.c` : chaque fonction y est déclarée `weak`. Dès qu'un vrai fichier définit la fonction,
l'éditeur de liens l'utilise automatiquement, sans rien modifier.

## Fichier `.ocshrc`

Au démarrage interactif, `ocsh` exécute chaque ligne de `~/.ocshrc` (lignes vides et `#` ignorées), par exemple :

```
# ~/.ocshrc
say Bienvenue dans ocsh
```

## Tests

- `code/tests/test_parser.c` : tests unitaires du parser (opérateurs, guillemets, erreurs de syntaxe).
- `code/tests/smoke_ismail.sh` : tests de fumée du shell (code retour 127, erreurs de syntaxe, `.ocshrc`, mode script).

## État d'avancement

| Jalon | Contenu | État |
|---|---|---|
| 1 | REPL, découpage par `->`, fork/execvp/wait | fait |
| 5 (partie) | `.ocshrc` | fait |
| 2 | builtins, alias, redirections | à intégrer (Omar) |
| 3 | pipes, `&`, signaux | à intégrer (Abderrahmane) |
| 4 | variables et expansion | à intégrer (Ayman) |
| 5 (partie) | `$(...)`, historique, complétion | à intégrer |
| 6 | mode script if/for/while | à intégrer (Ayman) |

*(Ce tableau est mis à jour par le groupe au fil de l'intégration.)*
