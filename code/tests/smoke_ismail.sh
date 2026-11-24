#!/bin/sh
# Tests de fumee du jalon 1 : lance ocsh avec des commandes sur l'entree standard.
# Usage : sh tests/smoke_ismail.sh ./ocsh
OCSH=${1:-./ocsh}
fails=0
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

check() {   # check "nom" "attendu" "obtenu"
    if [ "$2" = "$3" ]; then echo "  ok   $1"; else
        echo "  ECHEC $1 : attendu [$2] obtenu [$3]"; fails=$((fails+1)); fi
}

echo "== tests de fumee ocsh =="

check "commande simple"      "bonjour"  "$(printf 'echo bonjour\n' | $OCSH)"
check "guillemets"           "a   b"    "$(printf 'echo "a   b"\n' | $OCSH)"
check "vide + commentaires"  "ok"       "$(printf '# c\n\n   \necho ok\n' | $OCSH)"

printf 'commandeinexistante\n' | $OCSH 2>"$TMP/err" >/dev/null; rc=$?
check "code 127 (introuvable)" "127" "$rc"
check "message introuvable"    "1"   "$(grep -c 'introuvable' "$TMP/err")"

printf 'ls ->\n' | $OCSH 2>"$TMP/err" >/dev/null; rc=$?
check "erreur de syntaxe -> code 2" "2" "$rc"
check "message de syntaxe"          "1" "$(grep -c 'syntaxe' "$TMP/err")"

printf 'false\n' | $OCSH >/dev/null 2>&1; check "statut de false" "1" "$?"
printf 'true\n'  | $OCSH >/dev/null 2>&1; check "statut de true"  "0" "$?"

check "quit arrete la boucle" "" "$(printf 'quit\necho non\n' | $OCSH 2>/dev/null)"

printf 'echo rc-charge\n' > "$TMP/.ocshrc"
check ".ocshrc charge" "rc-charge" "$(HOME=$TMP OCSH_FORCE_RC=1 $OCSH </dev/null)"
check ".ocshrc absent sans erreur" "" "$(HOME=$TMP/rien OCSH_FORCE_RC=1 $OCSH </dev/null 2>&1)"

printf 'echo un\necho deux\n' > "$TMP/s.ocsh"
check "mode script" "un
deux" "$($OCSH "$TMP/s.ocsh")"

$OCSH --help | grep -q Usage; check "--help" "0" "$?"

if [ $fails -eq 0 ]; then echo "Tous les tests de fumee passent."; else echo "$fails echec(s)"; fi
exit $fails
