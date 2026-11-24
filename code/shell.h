/* shell.h : contrat commun du projet ocsh (mini-shell en C).
 *
 * Ce fichier est le "contrat" entre les 4 membres du groupe : chacun code
 * ses fonctions contre ces signatures. Il ne se modifie que par accord de
 * tout le groupe (sinon les fusions Git deviennent difficiles).
 */
#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>

#define PROMPT    "ocsh> "
#define MAX_ARGS  64      /* arguments max par commande (NULL final inclus) */
#define MAX_CMDS  16      /* commandes max dans un pipeline (separees par ->) */

/* Une commande simple : "sort -r < in.txt > out.txt" */
typedef struct {
    char *argv[MAX_ARGS];   /* argv[0] = commande ; tableau termine par NULL */
    int   argc;
    char *infile;           /* "<"  : nom de fichier ou NULL */
    char *outfile;          /* ">" ou ">>" : nom de fichier ou NULL */
    int   append;           /* 1 si ">>", 0 si ">" */
} Command;

/* Une ligne complete : "cmd1 -> cmd2 -> cmd3 &" */
typedef struct {
    Command cmds[MAX_CMDS]; /* separees par l'operateur "->" */
    int     ncmds;          /* 0 = ligne vide / commentaire */
    int     background;     /* 1 si "&" en fin de ligne */
} Pipeline;

extern int   last_status;   /* $?  (defini dans main.c) */
extern pid_t last_bg_pid;   /* $!  (defini dans pipeline.c) */

/* ------------------------------ ISMAIL ------------------------------ */
int   execute_line(const char *line);  /* assign -> subst -> expand -> parse -> exec */
int   parse_line(const char *line, Pipeline *p);  /* 0 = OK, -1 = erreur de syntaxe */
void  free_pipeline(Pipeline *p);
void  child_exec(Command *c);          /* redirs + alias + execvp ; ne retourne pas */
int   exec_simple(Command *c);         /* fork + child_exec + waitpid (commande seule) */
int   status_to_code(int wait_status); /* statut waitpid -> code $? (128+sig si signal) */
void  load_rc(void);                   /* lit ~/.ocshrc */

/* ------------------------------- OMAR ------------------------------- */
int   is_builtin(const char *name);
int   run_builtin(Command *c);         /* dans le parent ; renvoie le status */
int   run_builtin_redir(Command *c);   /* builtin + redirections (dup / dup2 / restore) */
void  apply_redirections(Command *c);  /* open + dup2 (cote enfant) */
void  translate_alias(Command *c);     /* list->ls, show->cat, copy->cp ... */
void  history_add(const char *line);
void  init_readline(void);             /* historique + completion */

/* --------------------------- ABDERRAHMANE --------------------------- */
int   execute_pipeline(Pipeline *p);   /* fork/pipe/dup2/wait, &, maj last_status */
void  setup_signals(void);             /* SIGINT, SIGCHLD */
char *command_subst(const char *line); /* $(...) et `...` -> texte (malloc) */

/* ------------------------------- AYMAN ------------------------------ */
char *expand_vars(const char *line);   /* $VAR ${VAR} $? $! (malloc) */
int   try_assignment(const char *line);/* "X=5" -> 1 si c'etait une affectation */
int   builtin_export(char **argv);
void  var_set(const char *k, const char *v);
const char *var_get(const char *k);
int   run_script(const char *path);    /* ocsh script.ocsh : if / for / while */

#endif /* SHELL_H */
