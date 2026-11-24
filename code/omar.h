/* omar.h : interface PRIVEE du module d'Omar (builtins, alias, redirections,
 * historique). Ce header n'est pas le contrat commun (shell.h) : il sert
 * uniquement a partager des petites fonctions d'aide entre nos 4 fichiers
 * (et a offrir deux utilitaires a Ismail pour le REPL). */
#ifndef OMAR_H
#define OMAR_H

/* Ecrit l'historique dans ~/.ocsh_history (appele par quit et par le REPL
 * quand l'utilisateur fait Ctrl-D). */
void history_save(void);

/* Expansion des raccourcis "!!" et "!n" (bonus). A appeler par le REPL AVANT
 * history_add et AVANT execute_line.
 *   - ligne normale        -> renvoie une copie (malloc) de la ligne
 *   - "!!" ou "!n" valide  -> affiche la commande retrouvee, renvoie une copie
 *   - evenement introuvable -> message d'erreur, renvoie NULL (ne rien executer)
 * Le resultat non NULL doit etre libere avec free(). */
char *history_bang(const char *line);

/* Parcours des noms de builtins / alias (pour la completion et "info").
 * Renvoient NULL quand i depasse la fin de la table. */
const char *builtin_name_at(int i);
const char *alias_name_at(int i);

#endif /* OMAR_H */
