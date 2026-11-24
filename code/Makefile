# Makefile d'ocsh (mini-shell en C).
#   make            compile ./ocsh
#   make run        compile puis lance le shell
#   make test       tests unitaires du parser + tests de fumee du shell
#   make clean      supprime les fichiers generes
#   make READLINE=0 force la compilation sans readline (fallback getline)
#
# readline (historique avec fleches + completion par Tab) est detectee
# automatiquement. Pour l'installer : sudo apt install libreadline-dev

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -g -std=gnu11
SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:.c=.o)

READLINE ?= auto
ifeq ($(READLINE),auto)
HAVE_RL := $(shell printf '\#include <stdio.h>\n\#include <readline/readline.h>\nint main(void){return 0;}\n' | $(CC) -x c - -lreadline -o /dev/null 2>/dev/null && echo 1)
else ifeq ($(READLINE),1)
HAVE_RL := 1
endif

ifeq ($(HAVE_RL),1)
CPPFLAGS += -DUSE_READLINE
LDLIBS   += -lreadline
endif

.PHONY: all run test clean

all: ocsh

ocsh: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# toute modification de shell.h recompile tout
%.o: %.c shell.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: ocsh
	./ocsh

test_parser: tests/test_parser.c parser.c shell.h
	$(CC) $(CFLAGS) -I. -o $@ tests/test_parser.c parser.c

test: ocsh test_parser
	./test_parser
	sh tests/smoke_ismail.sh ./ocsh

clean:
	rm -f ocsh test_parser *.o
