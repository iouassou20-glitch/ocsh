/* parser.c : decoupe une ligne de commande en structure Pipeline.
 *
 * Grammaire reconnue (hors guillemets) :
 *     ligne    := commande ( "->" commande )* [ "&" ]
 *     commande := mot+ avec redirections "<" fichier, ">" fichier, ">>" fichier
 *
 * Points importants :
 *   - "->" est teste AVANT ">" : sinon la pipe serait lue comme une redirection.
 *   - les operateurs n'ont pas besoin d'espaces autour : "a->b" , "say x>f".
 *   - '...' et "..." protegent les operateurs et les espaces ; les guillemets
 *     sont retires du resultat. Le backslash echappe le caractere suivant.
 *   - '#' en debut de mot : le reste de la ligne est un commentaire.
 *   - les expansions ($VAR, $(...)) ont deja ete faites AVANT le parsing.
 */
#include "shell.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { TK_WORD, TK_PIPE, TK_IN, TK_OUT, TK_APPEND, TK_BG, TK_END, TK_ERR } TokType;

/* Petit tampon de caracteres extensible pour construire un mot. */
typedef struct { char *s; size_t len, cap; } Buf;

static int buf_push(Buf *b, char ch)
{
    if (b->len + 1 >= b->cap) {
        size_t ncap = b->cap ? b->cap * 2 : 32;
        char *n = realloc(b->s, ncap);
        if (!n) return -1;
        b->s = n;
        b->cap = ncap;
    }
    b->s[b->len++] = ch;
    b->s[b->len] = '\0';
    return 0;
}

/* Vrai si *s commence un operateur ("->", ">", ">>", "<", "&"). */
static int is_op_start(const char *s)
{
    return *s == '<' || *s == '>' || *s == '&' || (s[0] == '-' && s[1] == '>');
}

/* Lit le prochain jeton. Pour TK_WORD, *text recoit une chaine malloc'ee.
 * Pour TK_ERR, *err recoit le message. *sp avance apres le jeton. */
static TokType next_token(const char **sp, char **text, const char **err)
{
    const char *s = *sp;
    Buf b = {0, 0, 0};

    *text = NULL;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;

    if (*s == '\0') { *sp = s; return TK_END; }
    if (*s == '#')  { *sp = s + strlen(s); return TK_END; }   /* commentaire */

    /* operateurs : "->" et ">>" AVANT ">" */
    if (s[0] == '-' && s[1] == '>') { *sp = s + 2; return TK_PIPE; }
    if (s[0] == '>' && s[1] == '>') { *sp = s + 2; return TK_APPEND; }
    if (*s == '>') { *sp = s + 1; return TK_OUT; }
    if (*s == '<') { *sp = s + 1; return TK_IN; }
    if (*s == '&') { *sp = s + 1; return TK_BG; }

    /* mot : on accumule jusqu'a un espace ou un operateur non protege */
    while (*s && !isspace((unsigned char)*s) && !is_op_start(s)) {
        if (*s == '\'') {                       /* '...' : tout est litteral */
            s++;
            while (*s && *s != '\'')
                if (buf_push(&b, *s++) < 0) goto oom;
            if (!*s) { *err = "guillemet simple non ferme"; free(b.s); return TK_ERR; }
            s++;
        } else if (*s == '"') {                 /* "..." : \" \\ \$ \` echappables */
            s++;
            while (*s && *s != '"') {
                if (*s == '\\' && (s[1] == '"' || s[1] == '\\' || s[1] == '$' || s[1] == '`'))
                    s++;
                if (buf_push(&b, *s++) < 0) goto oom;
            }
            if (!*s) { *err = "guillemet double non ferme"; free(b.s); return TK_ERR; }
            s++;
        } else if (*s == '\\' && s[1]) {        /* \x : caractere suivant litteral */
            s++;
            if (buf_push(&b, *s++) < 0) goto oom;
        } else {
            if (buf_push(&b, *s++) < 0) goto oom;
        }
    }
    if (!b.s) {                                 /* mot vide : "" ou '' */
        b.s = strdup("");
        if (!b.s) goto oom;
    }
    *sp = s;
    *text = b.s;
    return TK_WORD;

oom:
    *err = "memoire insuffisante";
    free(b.s);
    return TK_ERR;
}

void free_pipeline(Pipeline *p)
{
    for (int i = 0; i < MAX_CMDS; i++) {
        Command *c = &p->cmds[i];
        for (int j = 0; j < c->argc; j++) free(c->argv[j]);
        free(c->infile);
        free(c->outfile);
    }
    memset(p, 0, sizeof *p);
}

int parse_line(const char *line, Pipeline *p)
{
    const char *s = line;
    const char *msg = NULL;
    char *text = NULL;
    int ci = 0;               /* index de la commande en cours de remplissage */
    int cmd_has = 0;          /* la commande en cours a-t-elle recu quelque chose ? */
    int bg = 0;               /* '&' deja rencontre ? */
    TokType redir = TK_END;   /* TK_END = aucune redirection en attente de nom */
    Command *c;

    memset(p, 0, sizeof *p);
    c = &p->cmds[0];

    for (;;) {
        TokType t = next_token(&s, &text, &msg);
        if (t == TK_ERR) goto fail;
        if (t == TK_END) break;
        if (bg) { msg = "'&' doit etre en fin de ligne"; goto fail; }

        switch (t) {
        case TK_WORD:
            if (redir != TK_END) {              /* ce mot est le nom du fichier */
                char **dst = (redir == TK_IN) ? &c->infile : &c->outfile;
                free(*dst);
                *dst = text;
                if (redir == TK_APPEND) c->append = 1;
                else if (redir == TK_OUT) c->append = 0;
                redir = TK_END;
            } else {
                if (c->argc >= MAX_ARGS - 1) { msg = "trop d'arguments"; goto fail; }
                c->argv[c->argc++] = text;
            }
            text = NULL;                        /* possede maintenant par la commande */
            cmd_has = 1;
            break;
        case TK_IN: case TK_OUT: case TK_APPEND:
            if (redir != TK_END) { msg = "redirection sans nom de fichier"; goto fail; }
            redir = t;
            cmd_has = 1;
            break;
        case TK_PIPE:
            if (redir != TK_END) { msg = "redirection sans nom de fichier"; goto fail; }
            if (c->argc == 0)    { msg = "commande vide avant '->'"; goto fail; }
            if (ci + 1 >= MAX_CMDS) { msg = "trop de commandes dans le pipeline"; goto fail; }
            c = &p->cmds[++ci];
            cmd_has = 0;
            break;
        case TK_BG:
            if (redir != TK_END) { msg = "redirection sans nom de fichier"; goto fail; }
            if (!cmd_has)        { msg = "rien a lancer en arriere-plan"; goto fail; }
            bg = 1;
            break;
        default:
            break;
        }
    }

    if (redir != TK_END)       { msg = "redirection sans nom de fichier"; goto fail; }
    if (!cmd_has && ci == 0)   { p->ncmds = 0; return 0; }     /* ligne vide */
    if (!cmd_has)              { msg = "commande vide apres '->'"; goto fail; }
    if (c->argc == 0)          { msg = "commande manquante"; goto fail; }

    p->ncmds = ci + 1;
    p->background = bg;
    return 0;

fail:
    free(text);
    free_pipeline(p);
    fprintf(stderr, "ocsh: erreur de syntaxe: %s\n", msg);
    return -1;
}
