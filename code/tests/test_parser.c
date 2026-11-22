/* Tests unitaires du parser (jalon 1). Lancer avec : make test */
#include "shell.h"

#include <stdio.h>
#include <string.h>

static int fails = 0, total = 0;

#define CHECK(cond, msg) do { total++; if (!(cond)) { fails++; \
    printf("  ECHEC: %s   [ligne: %s]\n", msg, line); } } while (0)

static void ok_line(const char *line, int ncmds, int bg, Pipeline *p)
{
    int rc = parse_line(line, p);
    CHECK(rc == 0, "doit etre accepte");
    CHECK(p->ncmds == ncmds, "nombre de commandes");
    CHECK(p->background == bg, "drapeau background");
}

static void bad_line(const char *line)
{
    Pipeline p;
    int rc = parse_line(line, &p);
    CHECK(rc == -1, "doit etre une erreur de syntaxe");
    free_pipeline(&p);
}

int main(void)
{
    Pipeline p;
    const char *line;

    printf("== tests du parser ==\n");

    line = "ls -l";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].argc == 2 && !strcmp(p.cmds[0].argv[1], "-l"), "arguments");
    CHECK(p.cmds[0].argv[2] == NULL, "argv termine par NULL");
    free_pipeline(&p);

    line = "show f.txt -> wc -l";
    ok_line(line, 2, 0, &p);
    CHECK(!strcmp(p.cmds[0].argv[0], "show") && !strcmp(p.cmds[1].argv[0], "wc"), "pipe ->");
    free_pipeline(&p);

    line = "a->b->c";
    ok_line(line, 3, 0, &p);                         /* sans espaces */
    free_pipeline(&p);

    line = "say hello > f.txt";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].outfile && !strcmp(p.cmds[0].outfile, "f.txt") && !p.cmds[0].append, "redirection >");
    CHECK(p.cmds[0].argc == 2, "le nom de fichier n'est pas un argument");
    free_pipeline(&p);

    line = "say hello >> f.txt";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].outfile && p.cmds[0].append == 1, "redirection >>");
    free_pipeline(&p);

    line = "say x>f.txt";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].outfile && !strcmp(p.cmds[0].outfile, "f.txt"), "redirection sans espace");
    free_pipeline(&p);

    line = "sort < in.txt -> uniq > out.txt &";
    ok_line(line, 2, 1, &p);
    CHECK(!strcmp(p.cmds[0].infile, "in.txt") && !strcmp(p.cmds[1].outfile, "out.txt"), "mix < -> > &");
    free_pipeline(&p);

    line = "sleep 10 &";
    ok_line(line, 1, 1, &p);
    free_pipeline(&p);

    line = "say \"a   b\" 'c d'";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].argc == 3 && !strcmp(p.cmds[0].argv[1], "a   b")
          && !strcmp(p.cmds[0].argv[2], "c d"), "guillemets");
    free_pipeline(&p);

    line = "say \"->\" '>' \"&\"";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].argc == 4 && !strcmp(p.cmds[0].argv[1], "->")
          && !strcmp(p.cmds[0].argv[2], ">") && p.cmds[0].outfile == NULL, "operateurs entre guillemets");
    free_pipeline(&p);

    line = "say \"\"";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].argc == 2 && !strcmp(p.cmds[0].argv[1], ""), "argument vide");
    free_pipeline(&p);

    line = "say a\\ b";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].argc == 2 && !strcmp(p.cmds[0].argv[1], "a b"), "backslash");
    free_pipeline(&p);

    line = "say salut # commentaire -> ignore";
    ok_line(line, 1, 0, &p);
    CHECK(p.cmds[0].argc == 2, "commentaire en fin de ligne");
    free_pipeline(&p);

    line = "";            ok_line(line, 0, 0, &p); free_pipeline(&p);
    line = "   \t  ";      ok_line(line, 0, 0, &p); free_pipeline(&p);
    line = "# juste un commentaire"; ok_line(line, 0, 0, &p); free_pipeline(&p);

    /* erreurs de syntaxe attendues (messages sur stderr) */
    bad_line("-> a");
    bad_line("a ->");
    bad_line("a -> -> b");
    bad_line("a >");
    bad_line("a > > b");
    bad_line("a & b");
    bad_line("&");
    bad_line("> f.txt");
    bad_line("a -> > f");
    bad_line("say \"non ferme");
    bad_line("say 'non ferme");

    printf("%d/%d verifications reussies\n", total - fails, total);
    return fails ? 1 : 0;
}
