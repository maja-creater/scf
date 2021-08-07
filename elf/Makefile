CFILES += ../util/scf_string.c
CFILES += scf_elf.c
CFILES += scf_elf_x64.c
CFILES += scf_elf_x64_so.c
#CFILES += scf_leb128_test.c
CFILES += scf_dwarf_line.c
CFILES += scf_dwarf_abbrev.c
CFILES += scf_dwarf_info.c
#CFILES += scf_dwarf_line_test.c
#CFILES += scf_dwarf_info_test.c
#CFILES += scf_dwarf_abbrev_test.c
CFILES += scf_elf_link.c

CFLAGS += -g
#CFLAGS += -Wall 
CFLAGS += -I../util
CFLAGS += -I../native
CFLAGS += -I../core
CFLAGS += -I../lex
CFLAGS += -I../parse
CFLAGS += -I./

LDFLAGS +=

all:
	gcc $(CFLAGS) $(CFILES) $(LDFLAGS)

