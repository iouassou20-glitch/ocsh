/* main.c : point d'entree et boucle REPL (Read - Eval - Print - Loop) d'ocsh.
 *
 *   ./ocsh              mode interactif (prompt "ocsh> ")
 *   ./ocsh script.ocsh  mode script (voir script.c)
 *   echo "ls" | ./ocsh  lit les commandes sur l'entree standard, sans prompt
 */
#include "shell.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef USE_READLINE
#include <readline/readline.h>
#endif

int last_status = 0;                    /* $? : code de la derniere commande */

static void usage(FILE *out)
{
    fprintf(out,
        "Usage: ocsh [script.ocsh]\n"
        "  sans argument : shell interactif (prompt \"%s\")\n"
        "  script.ocsh   : execute le script puis quitte\n"
        "  -h, --help    : affiche cette aide\n", PROMPT);
}

static int is_blank(const char *s)
{
    while (*s) {
        if (!isspace((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

/* Lit une ligne (memoire malloc'ee, sans '\n') ; NULL a la fin de fichier (Ctrl+D). */
static char *read_line(int interactive)
{
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;

#ifdef USE_READLINE
    if (interactive)
        return readline(PROMPT);        /* edition de ligne + historique (fleches) */
#endif
    if (interactive) {
        fputs(PROMPT, stdout);
        fflush(stdout);
    }
    n = getline(&line, &cap, stdin);
    if (n < 0) {
        free(line);
        return NULL;
    }
    if (n > 0 && line[n - 1] == '\n') line[n - 1] = '\0';
    return line;
}

/* Execute UNE ligne : affectation -> $(...) -> $VAR -> parsing -> execution.
 * Renvoie le code de retour (aussi stocke dans last_status). */
int execute_line(const char *line)
{
    Pipeline p;
    char *s1, *s2;
    const char *cur, *fin;
    int rc;

    if (!line) return last_status;

    if (try_assignment(line)) {         /* "X=5" : affectation, rien a lancer */
        last_status = 0;
        return last_status;
    }

    s1 = command_subst(line);           /* 1) $(...) et `...` */
    cur = s1 ? s1 : line;
    s2 = expand_vars(cur);              /* 2) $VAR ${VAR} $? $! */
    fin = s2 ? s2 : cur;

    rc = parse_line(fin, &p);           /* 3) texte -> Pipeline */
    if (rc != 0)
        last_status = 2;                /* erreur de syntaxe */
    else if (p.ncmds > 0)
        last_status = execute_pipeline(&p);   /* 4) fork / pipes / wait */

    free_pipeline(&p);
    free(s2);
    free(s1);
    return last_status;
}

int main(int argc, char **argv)
{
    int interactive;

    if (argc >= 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        usage(stdout);
        return 0;
    }
    if (argc > 2) {
        usage(stderr);
        return 2;
    }

    setup_signals();                    /* Abderrahmane : SIGINT / SIGCHLD */

    if (argc == 2)                      /* mode script (Ayman) */
        return run_script(argv[1]) & 0xFF;

    interactive = isatty(STDIN_FILENO);
    if (interactive)
        init_readline();                /* Omar : historique + completion */
    /* OCSH_FORCE_RC=1 : charge .ocshrc meme sans terminal (utile pour les tests) */
    if (interactive || getenv("OCSH_FORCE_RC"))
        load_rc();

    for (;;) {
        char *line = read_line(interactive);
        if (!line) {                    /* Ctrl+D */
            if (interactive) putchar('\n');
            break;
        }
        if (!is_blank(line)) {
            history_add(line);
            execute_line(line);
        }
        free(line);
    }
    return last_status & 0xFF;
}
