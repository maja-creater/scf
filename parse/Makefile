CFILES += ../util/scf_string.c
CFILES += ../util/scf_graph.c
CFILES += ../lex/scf_lex.c

CFILES += scf_parse_util.c
#CFILES += scf_parse.c
CFILES += scf_parse2.c
CFILES += scf_parse_test.c
CFILES += scf_operator_handler_semantic.c
CFILES += scf_operator_handler_expr.c
CFILES += scf_operator_handler_const.c

CFILES += ../native/scf_native.c
CFILES += ../native/x64/scf_x64.c
CFILES += ../native/x64/scf_x64_inst.c
CFILES += ../native/x64/scf_x64_inst_util.c
CFILES += ../native/x64/scf_x64_inst_shift.c
CFILES += ../native/x64/scf_x64_inst_common.c
CFILES += ../native/x64/scf_x64_inst_binary.c
CFILES += ../native/x64/scf_x64_inst_mul.c
CFILES += ../native/x64/scf_x64_inst_div.c
CFILES += ../native/x64/scf_x64_inst_cmp.c
CFILES += ../native/x64/scf_x64_inst_unary.c

CFILES += ../native/x64/scf_x64_rcg.c
CFILES += ../native/x64/scf_x64_graph.c
CFILES += ../native/x64/scf_x64_reg.c
CFILES += ../native/x64/scf_x64_opcode.c
CFILES += ../native/x64/scf_x64_peephole.c
CFILES += ../native/x64/scf_x64_bb_color.c

CFILES += ../elf/scf_elf.c
CFILES += ../elf/scf_elf_link.c
CFILES += ../elf/scf_elf_x64.c
CFILES += ../elf/scf_elf_x64_so.c
CFILES += ../elf/scf_dwarf.c
CFILES += ../elf/scf_dwarf_abbrev.c
CFILES += ../elf/scf_dwarf_info.c
CFILES += ../elf/scf_dwarf_line.c

CFILES += ../core/scf_lex_word.c
CFILES += ../core/scf_type.c
CFILES += ../core/scf_variable.c

CFILES += ../core/scf_node.c
CFILES += ../core/scf_label.c

CFILES += ../core/scf_expr.c
CFILES += ../core/scf_block.c
CFILES += ../core/scf_scope.c
CFILES += ../core/scf_function.c

CFILES += ../core/scf_operator.c
CFILES += ../core/scf_operator_handler.c

CFILES += ../core/scf_type_cast.c
CFILES += ../core/scf_type_cast_i8.c
CFILES += ../core/scf_type_cast_i16.c
CFILES += ../core/scf_type_cast_i32.c
CFILES += ../core/scf_type_cast_i64.c
CFILES += ../core/scf_type_cast_u8.c
CFILES += ../core/scf_type_cast_u16.c
CFILES += ../core/scf_type_cast_u32.c
CFILES += ../core/scf_type_cast_u64.c
CFILES += ../core/scf_type_cast_float.c
CFILES += ../core/scf_type_cast_double.c

CFILES += ../core/scf_operator_handler_3ac.c

CFILES += ../core/scf_dag.c
CFILES += ../core/scf_operator_dag.c
CFILES += ../core/scf_basic_block.c
CFILES += ../core/scf_3ac.c

CFILES += ../core/scf_pointer_alias.c

CFILES += ../core/scf_optimizer.c
CFILES += ../core/scf_optimizer_dag.c
CFILES += ../core/scf_optimizer_inline.c
CFILES += ../core/scf_optimizer_call.c
CFILES += ../core/scf_optimizer_pointer_alias.c
CFILES += ../core/scf_optimizer_active_vars.c
CFILES += ../core/scf_optimizer_pointer_aliases.c
CFILES += ../core/scf_optimizer_loads_saves.c
CFILES += ../core/scf_optimizer_auto_gc_find.c
CFILES += ../core/scf_optimizer_auto_gc.c
CFILES += ../core/scf_optimizer_dominators.c
CFILES += ../core/scf_optimizer_dominators_reverse.c
CFILES += ../core/scf_optimizer_basic_block.c
CFILES += ../core/scf_optimizer_const_teq.c
CFILES += ../core/scf_optimizer_loop.c
CFILES += ../core/scf_optimizer_group.c
CFILES += ../core/scf_optimizer_generate_loads_saves.c

CFILES += ../core/scf_ast.c

CFILES += ../core/scf_calculate.c

CFILES += scf_dfa.c
CFILES += scf_dfa_parse.c

CFILES += scf_dfa_include.c
CFILES += scf_dfa_call.c
CFILES += scf_dfa_create.c
CFILES += scf_dfa_sizeof.c
CFILES += scf_dfa_container.c
CFILES += scf_dfa_va_arg.c
CFILES += scf_dfa_expr.c

CFILES += scf_dfa_union.c
CFILES += scf_dfa_class.c

CFILES += scf_dfa_type.c
CFILES += scf_dfa_identity.c

CFILES += scf_dfa_function.c
CFILES += scf_dfa_operator.c
CFILES += scf_dfa_var.c

CFILES += scf_dfa_init_data.c
CFILES += scf_struct_array.c

CFILES += scf_dfa_if.c
CFILES += scf_dfa_while.c
CFILES += scf_dfa_for.c

CFILES += scf_dfa_break.c
CFILES += scf_dfa_continue.c
CFILES += scf_dfa_goto.c
CFILES += scf_dfa_return.c
CFILES += scf_dfa_error.c
CFILES += scf_dfa_label.c
CFILES += scf_dfa_async.c

CFILES += scf_dfa_block.c

CFLAGS += -g
#CFLAGS += -Wall 
CFLAGS += -I../util
CFLAGS += -I../core
CFLAGS += -I../lex
CFLAGS += -I../parse
CFLAGS += -I../elf
CFLAGS += -I../native
CFLAGS += -I../native/x64

LDFLAGS +=

all:
	gcc $(CFLAGS) $(CFILES) $(LDFLAGS)

clean:
	rm *.o
