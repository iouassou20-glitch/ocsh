/* builtins.c : commandes internes de ocsh (executees dans le PARENT).
 * Auteur : Omar (module builtins / alias / redirections / historique).
 *
 * Correspondance : go=cd  quit=exit  path=pwd  say=echo  info=help
 *                  past=history  osinfo  export (fourni par Ayman).
 * Un builtin doit tourner dans le shell lui-meme : le repertoire courant est
 * un attribut du processus, un enfant qui ferait chdir ne changerait rien. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"
#include "omar.h"

typedef int (*builtin_fn)(Command *);

typedef struct {
    const char *name;
    builtin_fn fn;
} BuiltinEntry;

static int bi_go(Command *c);
static int bi_quit(Command *c);
static int bi_path(Command *c);
static int bi_say(Command *c);
static int bi_info(Command *c);
static int bi_past(Command *c);
static int bi_osinfo(Command *c);
static int bi_export(Command *c);

/* Table de dispatch : nom -> fonction. Pour ajouter jobs/fg/bg (si
 * Abderrahmane les fait), ajouter une ligne ici + un petit wrapper. */
static const BuiltinEntry builtin_table[] = {
    {"go",     bi_go},
    {"quit",   bi_quit},
    {"path",   bi_path},
    {"say",    bi_say},
    {"info",   bi_info},
    {"past",   bi_past},
    {"osinfo", bi_osinfo},
    {"export", bi_export},
};
#define NB_BUILTINS (sizeof(builtin_table) / sizeof(builtin_table[0]))

/* find_builtin : recherche lineaire dans la table, NULL si inconnu. */
static const BuiltinEntry *find_builtin(const char *name) {
    size_t i;

    if (name == NULL)
        return NULL;
    for (i = 0; i < NB_BUILTINS; i++)
        if (strcmp(name, builtin_table[i].name) == 0)
            return &builtin_table[i];
    return NULL;
}

/* is_builtin : 1 si name est un builtin (donc a executer dans le parent). */
int is_builtin(const char *name) {
    return find_builtin(name) != NULL;
}

/* builtin_name_at : i-eme nom de builtin (completion), NULL a la fin. */
const char *builtin_name_at(int i) {
    if (i < 0 || (size_t)i >= NB_BUILTINS)
        return NULL;
    return builtin_table[i].name;
}

/* run_builtin : execute le builtin c->argv[0] dans le PARENT et renvoie son
 * status (0 = succes). On vide stdout a la fin : sans cela, avec stdout
 * redirige vers un fichier/pipe (tampon complet), le texte pourrait sortir
 * apres celui d'une commande externe, ou apres la restauration du fd 1. */
int run_builtin(Command *c) {
    const BuiltinEntry *e;
    int status;

    if (c == NULL || c->argc < 1 || c->argv[0] == NULL)
        return 0;
    e = find_builtin(c->argv[0]);
    if (e == NULL)
        return 127;
    status = e->fn(c);
    if (fflush(stdout) != 0) { /* ex. say a > /dev/full */
        perror("ocsh: ecriture");
        clearerr(stdout);
        if (status == 0)
            status = 1;
    }
    return status;
}

/* go [dir] : chdir(). Sans argument -> $HOME. Met a jour PWD/OLDPWD. */
static int bi_go(Command *c) {
    const char *dir;
    char old[PATH_MAX], cur[PATH_MAX];

    if (c->argc > 2) {
        fprintf(stderr, "go: trop d'arguments\n");
        return 1;
    }
    if (c->argc == 2) {
        dir = c->argv[1];
    } else {
        dir = getenv("HOME");
        if (dir == NULL || *dir == '\0') {
            fprintf(stderr, "go: HOME n'est pas defini\n");
            return 1;
        }
    }
    if (getcwd(old, sizeof old) == NULL)
        old[0] = '\0';
    if (chdir(dir) != 0) {
        fprintf(stderr, "go: %s: %s\n", dir, strerror(errno));
        return 1;
    }
    if (old[0] != '\0')
        setenv("OLDPWD", old, 1);
    if (getcwd(cur, sizeof cur) != NULL)
        setenv("PWD", cur, 1);
    return 0;
}

/* quit [code] : sauvegarde l'historique puis exit(code). Sans argument, le
 * code de sortie est celui de la derniere commande ($?), comme bash. */
static int bi_quit(Command *c) {
    int code = last_status;

    if (c->argc > 1) {
        char *end;
        long v = strtol(c->argv[1], &end, 10);
        if (*c->argv[1] == '\0' || *end != '\0') {
            fprintf(stderr, "quit: %s: argument numerique requis\n", c->argv[1]);
            code = 2;
        } else {
            code = (int)(v & 0xFF);
        }
    }
    history_save();
    exit(code);
}

/* path : affiche le repertoire courant avec getcwd(). */
static int bi_path(Command *c) {
    char buf[PATH_MAX];

    (void)c;
    if (getcwd(buf, sizeof buf) == NULL) {
        perror("path");
        return 1;
    }
    printf("%s\n", buf);
    return 0;
}

/* say [-n] args... : affiche les arguments separes par un espace, puis \n
 * (sauf avec -n). */
static int bi_say(Command *c) {
    int first = 1, i, newline = 1;

    if (c->argc > 1 && strcmp(c->argv[1], "-n") == 0) {
        newline = 0;
        first = 2;
    }
    for (i = first; i < c->argc; i++) {
        if (i > first)
            putchar(' ');
        fputs(c->argv[i], stdout);
    }
    if (newline)
        putchar('\n');
    return 0;
}

/* info : liste des builtins, alias et operateurs, une phrase chacun. */
static int bi_info(Command *c) {
    (void)c;
    puts("ocsh : mini-shell. Commandes internes (builtins) :");
    puts("  go [dir]      change de repertoire (sans argument : $HOME)       [cd]");
    puts("  quit [code]   quitte le shell avec le code donne                 [exit]");
    puts("  path          affiche le repertoire courant                      [pwd]");
    puts("  say [-n] ...  affiche ses arguments (-n : sans retour a la ligne) [echo]");
    puts("  info          affiche cette aide                                 [help]");
    puts("  past          affiche l'historique numerote                      [history]");
    puts("  osinfo        affiche le nom du cours et de l'ecole");
    puts("  export V=val  exporte une variable vers l'environnement");
    puts("Alias (traduits vers la vraie commande) :");
    puts("  list=ls  show=cat  copy=cp  move=mv  del=rm  newdir=mkdir  newfile=touch");
    puts("Operateurs :");
    puts("  cmd > f       redirige la sortie vers f (ecrase)");
    puts("  cmd >> f      redirige la sortie vers f (ajoute a la fin)");
    puts("  cmd < f       lit l'entree depuis f");
    puts("  cmd1 -> cmd2  pipe : la sortie de cmd1 devient l'entree de cmd2");
    puts("  cmd &         execute cmd en arriere-plan");
    puts("Historique : !! rejoue la derniere commande, !n la n-ieme (voir past).");
    return 0;
}

/* past : affiche l'historique numerote ("1  ls", "2  go .."). */
static int bi_past(Command *c) {
    int i;

    (void)c;
    for (i = 0; i < history_length; i++) {
        HIST_ENTRY *e = history_get(history_base + i);
        if (e != NULL)
            printf("%5d  %s\n", history_base + i, e->line);
    }
    return 0;
}

/* osinfo : texte impose par le sujet. */
static int bi_osinfo(Command *c) {
    (void)c;
    puts("Systèmes d'Exploitation -- UIR/ESIN");
    return 0;
}

/* export : delegue a builtin_export (Ayman). */
static int bi_export(Command *c) {
    return builtin_export(c->argv);
}
