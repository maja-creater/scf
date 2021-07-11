#ifndef SCF_X64_H
#define SCF_X64_H

#include"scf_native.h"
#include"scf_x64_util.h"
#include"scf_x64_reg.h"
#include"scf_x64_opcode.h"
#include"scf_graph.h"
#include"scf_elf.h"

#define X64_INST_ADD_CHECK(vec, inst) \
			do { \
				if (!(inst)) { \
					scf_loge("\n"); \
					return -ENOMEM; \
				} \
				int ret = scf_vector_add((vec), (inst)); \
				if (ret < 0) { \
					scf_loge("\n"); \
					free(inst); \
					return ret; \
				} \
			} while (0)

#define X64_RELA_ADD_CHECK(vec, rela, c, v, f) \
	do { \
		if (rela) { \
			(rela)->code = (c); \
			(rela)->var  = (v); \
			(rela)->func = (f); \
			(rela)->inst = (c)->instructions->data[(c)->instructions->size - 1]; \
			(rela)->addend = -4; \
			int ret = scf_vector_add((vec), (rela)); \
			if (ret < 0) { \
				scf_loge("\n"); \
				free(rela); \
				return ret; \
			} \
		} \
	} while (0)

#define X64_PEEPHOLE_DEL 1
#define X64_PEEPHOLE_OK  0

typedef struct {

	scf_function_t*     f;

} scf_x64_context_t;

typedef struct {
	scf_dag_node_t*      dag_node;

	scf_register_x64_t*  reg;

	scf_x64_OpCode_t*    OpCode;

} x64_rcg_node_t;

typedef struct {
	int 	type;
	int		(*func)(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g);
} x64_rcg_handler_t;

typedef struct {
	int 	type;
	int		(*func)(scf_native_t* ctx, scf_3ac_code_t* c);
} x64_inst_handler_t;

x64_rcg_handler_t*  scf_x64_find_rcg_handler(const int op_type);
x64_inst_handler_t* scf_x64_find_inst_handler(const int op_type);

int x64_rcg_find_node(scf_graph_node_t** pp, scf_graph_t* g, scf_dag_node_t* dn, scf_register_x64_t* reg);
int _x64_rcg_make_node(scf_graph_node_t** pp, scf_graph_t* g, scf_dag_node_t* dn, scf_register_x64_t* reg, scf_x64_OpCode_t* OpCode);

int scf_x64_open(scf_native_t* ctx);
int scf_x64_close(scf_native_t* ctx);
int scf_x64_select(scf_native_t* ctx);

int x64_optimize_peephole(scf_native_t* ctx, scf_function_t* f);

int scf_x64_graph_kcolor(scf_graph_t* graph, int k, scf_vector_t* colors);


intptr_t x64_bb_find_color (scf_vector_t* dn_colors, scf_dag_node_t* dn);
int      x64_save_bb_colors(scf_vector_t* dn_colors, scf_bb_group_t* bbg, scf_basic_block_t* bb);

int x64_bb_load_dn (intptr_t color, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_function_t* f);
int x64_bb_save_dn (intptr_t color, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_function_t* f);
int x64_bb_load_dn2(intptr_t color, scf_dag_node_t* dn, scf_basic_block_t* bb, scf_function_t* f);
int x64_bb_save_dn2(intptr_t color, scf_dag_node_t* dn, scf_basic_block_t* bb, scf_function_t* f);

int  x64_fix_bb_colors  (scf_basic_block_t* bb, scf_bb_group_t* bbg, scf_function_t* f);
int  x64_load_bb_colors (scf_basic_block_t* bb, scf_bb_group_t* bbg, scf_function_t* f);
int  x64_load_bb_colors2(scf_basic_block_t* bb, scf_bb_group_t* bbg, scf_function_t* f);
void x64_init_bb_colors (scf_basic_block_t* bb);


scf_instruction_t* x64_make_inst(scf_x64_OpCode_t* OpCode, int size);
scf_instruction_t* x64_make_inst_G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r);
scf_instruction_t* x64_make_inst_E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r);
scf_instruction_t* x64_make_inst_I(scf_x64_OpCode_t* OpCode, uint8_t* imm, int size);
void               x64_make_inst_I2(scf_instruction_t* inst, scf_x64_OpCode_t* OpCode, uint8_t* imm, int size);

scf_instruction_t* x64_make_inst_I2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, uint8_t* imm, int size);
scf_instruction_t* x64_make_inst_I2E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, uint8_t* imm, int size);

scf_instruction_t* x64_make_inst_M ( scf_rela_t** prela, scf_x64_OpCode_t* OpCode, scf_variable_t* v,         scf_register_x64_t* r_base);
scf_instruction_t* x64_make_inst_I2M(scf_rela_t** prela, scf_x64_OpCode_t* OpCode, scf_variable_t* v_dst,     scf_register_x64_t* r_base, uint8_t* imm, int32_t size);
scf_instruction_t* x64_make_inst_G2M(scf_rela_t** prela, scf_x64_OpCode_t* OpCode, scf_variable_t* v_dst,     scf_register_x64_t* r_base, scf_register_x64_t* r_src);
scf_instruction_t* x64_make_inst_M2G(scf_rela_t** prela, scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_base, scf_variable_t* v_src);

scf_instruction_t* x64_make_inst_G2E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_src);
scf_instruction_t* x64_make_inst_E2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_src);

scf_instruction_t* x64_make_inst_P2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_base, int32_t offset);
scf_instruction_t* x64_make_inst_G2P(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, int32_t offset, scf_register_x64_t* r_src);
scf_instruction_t* x64_make_inst_I2P(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, int32_t offset, uint8_t* imm, int size);

scf_instruction_t* x64_make_inst_SIB2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst,  scf_register_x64_t* r_base,  scf_register_x64_t* r_index, int32_t scale, int32_t disp);
scf_instruction_t* x64_make_inst_G2SIB(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, scf_register_x64_t* r_index, int32_t scale, int32_t disp, scf_register_x64_t* r_src);
scf_instruction_t* x64_make_inst_I2SIB(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, scf_register_x64_t* r_index, int32_t scale, int32_t disp, uint8_t* imm, int32_t size);

scf_instruction_t* x64_make_inst_SIB(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base,  scf_register_x64_t* r_index, int32_t scale, int32_t disp, int size);
scf_instruction_t* x64_make_inst_P(  scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_base, int32_t offset, int size);

int x64_float_OpCode_type(int OpCode_type, int var_type);


int x64_shift(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);

int x64_shift_assign(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);


int x64_binary_assign(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);

int x64_binary_assign_dereference(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);

int x64_binary_assign_pointer(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);

int x64_binary_assign_array_index(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);

int x64_unary_assign_dereference(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);

int x64_unary_assign_pointer(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);

int x64_unary_assign_array_index(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);


int x64_inst_int_mul(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f);
int x64_inst_int_div(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f, int mod_flag);

int x64_inst_pointer(scf_native_t* ctx, scf_3ac_code_t* c, int lea_flag);
int x64_inst_dereference(scf_native_t* ctx, scf_3ac_code_t* c);

int x64_inst_float_cast(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f);

int x64_inst_movx(scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f);

int x64_inst_op2(int OpCode_type, scf_dag_node_t* dst, scf_dag_node_t* src, scf_3ac_code_t* c, scf_function_t* f);

int x64_inst_jmp(scf_native_t* ctx, scf_3ac_code_t* c, int OpCode_type);

int x64_inst_teq(scf_native_t* ctx, scf_3ac_code_t* c);
int x64_inst_cmp(scf_native_t* ctx, scf_3ac_code_t* c);
int x64_inst_set(scf_native_t* ctx, scf_3ac_code_t* c, int setcc_type);

int x64_inst_cmp_set(scf_native_t* ctx, scf_3ac_code_t* c, int setcc_type);

#endif

