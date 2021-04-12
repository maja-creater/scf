CFILES += ../util/scf_string.c
CFILES += scf_elf.c
CFILES += scf_elf_x64.c
CFILES += scf_elf_test3.c
#CFILES += scf_leb128_test.c
CFILES += scf_dwarf_line.c
CFILES += scf_dwarf_abbrev.c

CFLAGS += -g
#CFLAGS += -Wall 
CFLAGS += -I../util

LDFLAGS +=

all:
	gcc $(CFLAGS) $(CFILES) $(LDFLAGS)

