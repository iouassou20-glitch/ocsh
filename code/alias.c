/* alias.c : traduction des alias de ocsh vers les vraies commandes Unix.
 * Auteur : Omar (module builtins / alias / redirections / historique). */
#include <string.h>
#include <sys/types.h>
#include "shell.h"
#include "omar.h"

typedef struct {
    const char *alias; /* nom tape par l'utilisateur dans ocsh */
    const char *real;  /* commande reelle executee par execvp  */
} Alias;

static const Alias alias_table[] = {
    {"list",    "ls"},
    {"show",    "cat"},
    {"copy",    "cp"},
    {"move",    "mv"},
    {"del",     "rm"},
    {"newdir",  "mkdir"},
    {"newfile", "touch"},
};
#define NB_ALIAS (sizeof(alias_table) / sizeof(alias_table[0]))

/* translate_alias : remplace argv[0] par la vraie commande si c'est un alias
 * (list -> ls, show -> cat ...). Les autres arguments ne sont pas touches :
 * "list -l" devient "ls -l". Aucun appel systeme ici : c'est execvp, appele
 * ensuite par child_exec, qui lance la vraie commande.
 * IMPORTANT : on remplace simplement le POINTEUR argv[0] (chaine litterale,
 * jamais free()). C'est sans danger car la fonction est appelee cote ENFANT,
 * juste avant execvp : l'enfant ne fait jamais free_pipeline. */
void translate_alias(Command *c) {
    size_t i;

    if (c == NULL || c->argc < 1 || c->argv[0] == NULL)
        return;
    for (i = 0; i < NB_ALIAS; i++) {
        if (strcmp(c->argv[0], alias_table[i].alias) == 0) {
            c->argv[0] = (char *)alias_table[i].real;
            return;
        }
    }
}

/* alias_name_at : i-eme nom d'alias (pour la completion), NULL a la fin. */
const char *alias_name_at(int i) {
    if (i < 0 || (size_t)i >= NB_ALIAS)
        return NULL;
    return alias_table[i].alias;
}
