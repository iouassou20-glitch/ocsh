/* stubs.c : implementations PROVISOIRES des fonctions des autres membres.
 *
 * But : permettre a TOUT LE MONDE de compiler et tester ocsh des le jour 2,
 * sans attendre les modules des autres.
 *
 * Toutes les fonctions sont declarees "weak" : des qu'un fichier definit la
 * vraie version (ex. builtins.c definit is_builtin), l'editeur de liens
 * choisit automatiquement la vraie et ignore le stub. On n'a donc RIEN a
 * supprimer ni a modifier ici. A la fin du projet, stubs.c peut etre
 * supprime une fois que toutes les fonctions sont implementees.
 */
#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WEAK __attribute__((weak))

static void todo(const char *what)
{
    static char seen[512];
    if (strstr(seen, what)) return;                 /* un seul avertissement */
    if (strlen(seen) + strlen(what) + 2 < sizeof seen) {
        strcat(seen, what);
        strcat(seen, "|");
    }
    fprintf(stderr, "[stub] %s : pas encore implemente\n", what);
}

WEAK pid_t last_bg_pid = 0;

/* --- Omar --- */
WEAK int is_builtin(const char *name) { return strcmp(name, "quit") == 0; }

WEAK int run_builtin(Command *c)
{
    if (strcmp(c->argv[0], "quit") == 0)
        exit(c->argc > 1 ? atoi(c->argv[1]) : last_status);
    return 0;
}

WEAK int run_builtin_redir(Command *c) { return run_builtin(c); }

WEAK void apply_redirections(Command *c)
{
    if (c->infile || c->outfile) todo("redirections < > >> (Omar)");
}

WEAK void translate_alias(Command *c) { (void)c; }
WEAK void history_add(const char *line) { (void)line; }
WEAK void init_readline(void) { }

/* --- Abderrahmane --- */
WEAK int execute_pipeline(Pipeline *p)
{
    if (p->ncmds > 1) {
        todo("pipes -> (Abderrahmane)");
        return 1;
    }
    if (p->background) todo("arriere-plan & (Abderrahmane)");
    if (is_builtin(p->cmds[0].argv[0]))
        last_status = run_builtin_redir(&p->cmds[0]);
    else
        last_status = exec_simple(&p->cmds[0]);     /* jalon 1 d'Ismail */
    return last_status;
}

WEAK void setup_signals(void) { }
WEAK char *command_subst(const char *line) { return strdup(line); }

/* --- Ayman --- */
WEAK char *expand_vars(const char *line) { return strdup(line); }
WEAK int try_assignment(const char *line) { (void)line; return 0; }

WEAK int builtin_export(char **argv)
{
    (void)argv;
    todo("export (Ayman)");
    return 1;
}

WEAK void var_set(const char *k, const char *v) { (void)k; (void)v; }
WEAK const char *var_get(const char *k) { return getenv(k); }

/* Stub de run_script : execute le fichier ligne par ligne, SANS if/for/while. */
WEAK int run_script(const char *path)
{
    FILE *f = fopen(path, "r");
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;

    if (!f) {
        perror(path);
        return 127;
    }
    while ((n = getline(&line, &cap, f)) >= 0) {
        if (n > 0 && line[n - 1] == '\n') line[n - 1] = '\0';
        execute_line(line);
    }
    free(line);
    fclose(f);
    return last_status;
}
