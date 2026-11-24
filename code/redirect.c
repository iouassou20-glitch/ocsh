/* redirect.c : redirections  <  >  >>  avec open() et dup2().
 * Auteur : Omar (module builtins / alias / redirections / historique).
 *
 * Idee : dup2(fd, n) fait pointer le descripteur n sur le meme fichier que fd.
 * Les redirections se font AVANT exec et survivent a exec (les fd restent
 * ouverts), donc la commande lancee ecrit/lit dans le fichier sans le savoir. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "shell.h"
#include "omar.h"

/* open_infile : open(infile, O_RDONLY). Renvoie le fd, ou -1 apres avoir
 * affiche un message (fichier inexistant, permission refusee...). */
static int open_infile(const Command *c) {
    int fd = open(c->infile, O_RDONLY);
    if (fd < 0)
        fprintf(stderr, "ocsh: %s: %s\n", c->infile, strerror(errno));
    return fd;
}

/* open_outfile : open(outfile, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0644).
 * ">" ecrase le fichier (O_TRUNC), ">>" ajoute a la fin (O_APPEND).
 * Mode 0644 = rw-r--r-- avant application de l'umask. */
static int open_outfile(const Command *c) {
    int flags = O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC);
    int fd = open(c->outfile, flags, 0644);
    if (fd < 0)
        fprintf(stderr, "ocsh: %s: %s\n", c->outfile, strerror(errno));
    return fd;
}

/* apply_redirections : COTE ENFANT (apres fork, avant execvp). Ouvre les
 * fichiers puis dup2 sur 0 (stdin) et/ou 1 (stdout), et ferme le fd d'origine.
 * En cas d'erreur : message + _exit(1) (on n'a pas besoin de restaurer quoi que
 * ce soit, l'enfant va mourir ; _exit evite de vider deux fois les tampons
 * stdio herites du parent). */
void apply_redirections(Command *c) {
    int fd;

    if (c == NULL)
        return;
    if (c->infile != NULL) {
        fd = open_infile(c);
        if (fd < 0)
            _exit(1);
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("ocsh: dup2");
            _exit(1);
        }
        if (fd != STDIN_FILENO)
            close(fd);
    }
    if (c->outfile != NULL) {
        fd = open_outfile(c);
        if (fd < 0)
            _exit(1);
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("ocsh: dup2");
            _exit(1);
        }
        if (fd != STDOUT_FILENO)
            close(fd);
    }
}

/* run_builtin_redir : COTE PARENT. Un builtin (say, path...) s'execute dans le
 * shell lui-meme : si on redirigeait stdout sans le restaurer, tout le reste de
 * la session partirait dans le fichier. On fait donc :
 *   saved = dup(1)  ->  dup2(fd, 1)  ->  run_builtin  ->  dup2(saved, 1)
 * (idem pour stdin avec dup(0)). Appels systemes : open, dup, dup2, close.
 * Renvoie le status du builtin, ou 1 si une redirection echoue (dans ce cas le
 * builtin n'est pas execute, comme dans un vrai shell). */
int run_builtin_redir(Command *c) {
    int fd_in = -1, fd_out = -1, saved_in = -1, saved_out = -1;
    int status = 1;

    if (c == NULL)
        return 1;
    if (c->infile == NULL && c->outfile == NULL)
        return run_builtin(c);

    /* 1) on ouvre tout d'abord : si un fichier pose probleme, rien n'a encore
     *    ete modifie dans les descripteurs du shell. */
    if (c->infile != NULL) {
        fd_in = open_infile(c);
        if (fd_in < 0)
            return 1;
    }
    if (c->outfile != NULL) {
        fd_out = open_outfile(c);
        if (fd_out < 0) {
            if (fd_in >= 0)
                close(fd_in);
            return 1;
        }
    }

    /* 2) sauvegarde + redirection. On vide stdio avant de toucher au fd 1 pour
     *    ne pas envoyer dans le fichier du texte destine a l'ecran. */
    fflush(stdout);
    if (fd_in >= 0) {
        saved_in = dup(STDIN_FILENO);
        if (saved_in < 0 || dup2(fd_in, STDIN_FILENO) < 0) {
            perror("ocsh: redirection");
            goto restore;
        }
    }
    if (fd_out >= 0) {
        saved_out = dup(STDOUT_FILENO);
        if (saved_out < 0 || dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("ocsh: redirection");
            goto restore;
        }
    }

    /* 3) execution dans le parent (run_builtin vide stdout a la fin, donc le
     *    texte part bien dans le fichier AVANT la restauration). */
    status = run_builtin(c);

restore:
    /* 4) restauration : le fd 1 repointe vers le terminal d'origine. */
    fflush(stdout);
    if (saved_out >= 0) {
        dup2(saved_out, STDOUT_FILENO);
        close(saved_out);
    }
    if (saved_in >= 0) {
        dup2(saved_in, STDIN_FILENO);
        close(saved_in);
    }
    if (fd_in >= 0)
        close(fd_in);
    if (fd_out >= 0)
        close(fd_out);
    return status;
}
