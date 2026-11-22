/* rc.c : fichier de configuration ~/.ocshrc, execute au demarrage.
 * Chaque ligne est executee comme si elle etait tapee au prompt
 * (ex. "export PATH=...", "say Bienvenue dans ocsh"). */
#include "shell.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_rc(void)
{
    const char *home = getenv("HOME");
    char path[4096];
    FILE *f;
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;

    if (!home) return;
    snprintf(path, sizeof path, "%s/.ocshrc", home);

    f = fopen(path, "r");
    if (!f) return;                    /* pas de .ocshrc : pas une erreur */

    while ((n = getline(&line, &cap, f)) >= 0) {
        char *p = line;
        if (n > 0 && line[n - 1] == '\n') line[n - 1] = '\0';
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;
        execute_line(p);
    }
    free(line);
    fclose(f);
}
