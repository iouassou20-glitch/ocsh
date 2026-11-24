/* exec.c : execution d'une commande externe (fork / execvp / wait).
 *
 * child_exec() est appelee DANS le processus enfant (apres fork) par
 * exec_simple() ci-dessous et par execute_pipeline() d'Abderrahmane.
 */
#include "shell.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Convertit un statut de waitpid() en code de retour shell ($?) :
 * sortie normale -> code de exit ; tue par signal -> 128 + numero du signal. */
int status_to_code(int st)
{
    if (WIFEXITED(st))   return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return 128 + WTERMSIG(st);
    return 1;
}

/* A appeler dans l'enfant : prepare puis REMPLACE l'image du processus par la
 * commande (execvp cherche dans $PATH). Ne retourne jamais :
 *   127 = commande introuvable, 126 = trouvee mais non executable. */
void child_exec(Command *c)
{
    /* Un signal ignore (SIG_IGN) par le shell reste ignore apres exec :
     * on remet les comportements par defaut pour que Ctrl+C tue la commande. */
    signal(SIGINT,  SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    apply_redirections(c);      /* Omar : open + dup2 sur les fd 0 et 1 */
    translate_alias(c);         /* Omar : list -> ls, show -> cat, ... */

    if (c->argc == 0 || c->argv[0] == NULL)
        _exit(0);

    execvp(c->argv[0], c->argv);

    /* On n'arrive ici que si execvp a echoue */
    if (errno == ENOENT) {
        fprintf(stderr, "ocsh: %s: commande introuvable\n", c->argv[0]);
        _exit(127);
    }
    fprintf(stderr, "ocsh: %s: %s\n", c->argv[0], strerror(errno));
    _exit(126);
}

/* Jalon 1 : executer UNE commande externe au premier plan.
 * fork() -> l'enfant fait child_exec() -> le parent attend avec waitpid(). */
int exec_simple(Command *c)
{
    int st;
    pid_t pid = fork();

    if (pid < 0) {
        perror("ocsh: fork");
        return 1;
    }
    if (pid == 0)
        child_exec(c);          /* ne retourne pas */

    while (waitpid(pid, &st, 0) < 0) {
        if (errno != EINTR) {   /* EINTR : interrompu par un signal, on reessaie */
            perror("ocsh: waitpid");
            return 1;
        }
    }
    return status_to_code(st);
}
