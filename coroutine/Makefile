CFILES += ../util/scf_rbtree.c
CFILES += scf_coroutine.c
CFILES += scf_coroutine_run.c
CFILES += scf_coroutine_asm.S
CFILES += scf_coroutine_test.c

CFLAGS += -g -O3 -no-pie
CFLAGS += -I../util

LDFLAGS +=

all:
	gcc $(CFLAGS) $(CFILES) $(LDFLAGS)
