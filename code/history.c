/* history.c : historique des commandes + completion avec GNU readline.
 * Auteur : Omar (module builtins / alias / redirections / historique).
 *
 * Stockage en memoire : la liste interne de readline (add_history). Fichier
 * persistant : ~/.ocsh_history (read_history au demarrage, write_history a la
 * sortie). Necessite libreadline-dev : sudo apt install libreadline-dev
 * et l'edition de liens avec -lreadline. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"
#include "omar.h"

#define HISTORY_MAX 1000

/* history_path : construit "$HOME/.ocsh_history". Renvoie 0 si OK, -1 sinon. */
static int history_path(char *buf, size_t size) {
    const char *home = getenv("HOME");
    int n;

    if (home == NULL || *home == '\0')
        return -1;
    n = snprintf(buf, size, "%s/.ocsh_history", home);
    return (n < 0 || (size_t)n >= size) ? -1 : 0;
}

/* history_add : memorise la ligne (add_history de readline) en ignorant les
 * lignes vides/blanches et les doublons consecutifs. */
void history_add(const char *line) {
    const char *p;
    HIST_ENTRY *last;

    if (line == NULL)
        return;
    for (p = line; *p != '\0' && isspace((unsigned char)*p); p++) {
    }
    if (*p == '\0')
        return;
    if (history_length > 0) {
        last = history_get(history_base + history_length - 1);
        if (last != NULL && strcmp(last->line, line) == 0)
            return;
    }
    add_history(line);
}

/* history_save : write_history vers ~/.ocsh_history (max 1000 lignes). */
void history_save(void) {
    char path[4096];

    if (history_path(path, sizeof path) != 0)
        return;
    stifle_history(HISTORY_MAX);
    if (write_history(path) != 0)
        fprintf(stderr, "ocsh: impossible d'ecrire %s: %s\n", path, strerror(errno));
}

/* history_bang : gere "!!" (derniere commande) et "!n" (n-ieme commande) quand
 * ils forment TOUTE la ligne. Tout le reste est rendu tel quel, donc un
 * "say salut!" n'est jamais modifie. Voir omar.h pour le contrat. */
char *history_bang(const char *line) {
    const char *p = line, *rest;
    HIST_ENTRY *e = NULL;
    size_t n, i;

    if (line == NULL)
        return NULL;
    while (*p != '\0' && isspace((unsigned char)*p))
        p++;
    if (*p != '!')
        return strdup(line);
    n = strcspn(p, " \t\r\n");
    rest = p + n;
    while (*rest != '\0' && isspace((unsigned char)*rest))
        rest++;
    if (*rest != '\0')
        return strdup(line);

    if (n == 2 && p[1] == '!') {
        if (history_length > 0)
            e = history_get(history_base + history_length - 1);
    } else if (n >= 2 && n <= 10) {
        for (i = 1; i < n; i++)
            if (!isdigit((unsigned char)p[i]))
                return strdup(line);
        e = history_get((int)strtol(p + 1, NULL, 10));
    } else {
        return strdup(line);
    }

    if (e == NULL) {
        fprintf(stderr, "ocsh: %.*s: evenement introuvable\n", (int)n, p);
        return NULL;
    }
    printf("%s\n", e->line);
    fflush(stdout);
    return strdup(e->line);
}

/* command_generator : propose, un par un, les noms de builtins puis d'alias
 * qui commencent par "text" (appele par rl_completion_matches). */
static char *command_generator(const char *text, int state) {
    static int ib, ia;
    static size_t len;
    const char *name;

    if (state == 0) {
        ib = 0;
        ia = 0;
        len = strlen(text);
    }
    while ((name = builtin_name_at(ib)) != NULL) {
        ib++;
        if (strncmp(name, text, len) == 0)
            return strdup(name);
    }
    while ((name = alias_name_at(ia)) != NULL) {
        ia++;
        if (strncmp(name, text, len) == 0)
            return strdup(name);
    }
    return NULL;
}

/* ocsh_completion : appelee par readline a chaque TAB. Si le mot est en
 * position de commande (debut de ligne ou juste apres "->"), on complete avec
 * les builtins/alias ; sinon (ou si rien ne correspond) on renvoie NULL et
 * readline utilise sa completion par defaut : les noms de fichiers. */
static char **ocsh_completion(const char *text, int start, int end) {
    int p = start - 1;

    (void)end;
    while (p >= 0 && isspace((unsigned char)rl_line_buffer[p]))
        p--;
    if (p < 0 || (p >= 1 && rl_line_buffer[p] == '>' && rl_line_buffer[p - 1] == '-'))
        return rl_completion_matches(text, command_generator);
    return NULL;
}

/* init_readline : a appeler une fois au demarrage du shell. Charge
 * ~/.ocsh_history (s'il existe) et installe la completion. */
void init_readline(void) {
    char path[4096];

    using_history();
    stifle_history(HISTORY_MAX);
    if (history_path(path, sizeof path) == 0)
        read_history(path); /* erreur ignoree : premier lancement = pas de fichier */
    rl_attempted_completion_function = ocsh_completion;
}
