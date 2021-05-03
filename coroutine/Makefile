CFILES += scf_coroutine.c
CFILES += ../util/scf_rbtree.c
CFILES += scf_coroutine_asm.S

CFLAGS += -g -O3 -no-pie #-fno-stack-protector
CFLAGS += -I../util

LDFLAGS +=

all:
	gcc $(CFLAGS) $(CFILES) $(LDFLAGS)

