#ifndef SCF_X64_H
#define SCF_X64_H

#include"scf_native.h"
#include"scf_x64_util.h"
#include"scf_graph.h"
#include"scf_elf.h"

#define X64_COLOR(type, id, mask)   ((type) << 24 | (id) << 16 | (mask))
#define X64_COLOR_TYPE(c)           ((c) >> 24)
#define X64_COLOR_ID(c)             (((c) >> 16) & 0xff)
#define X64_COLOR_MASK(c)           ((c) & 0xffff)
#define X64_COLOR_CONFLICT(c0, c1)  ( (c0) >> 16 == (c1) >> 16 && (c0) & (c1) & 0xffff )

#define X64_COLOR_BYTES(c) \
	({ \
	     int n = 0;\
	     intptr_t minor = (c) & 0xffff; \
	     while (minor) { \
	         minor &= minor - 1; \
	         n++;\
	     } \
	     n;\
	 })

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
			(rela)->inst_index = (c)->instructions->size - 1; \
			int ret = scf_vector_add((vec), (rela)); \
			if (ret < 0) { \
				scf_loge("\n"); \
				free(rela); \
				return ret; \
			} \
		} \
	} while (0)

// ABI: rdi rsi rdx rcx r8 r9
static uint32_t x64_abi_regs[] =
{
	SCF_X64_REG_RDI,
	SCF_X64_REG_RSI,
	SCF_X64_REG_RDX,
	SCF_X64_REG_RCX,
};
#define X64_ABI_NB (sizeof(x64_abi_regs) / sizeof(x64_abi_regs[0]))

typedef struct {
	int			type;

	char*		name;

	int			len;

	uint8_t		OpCodes[3];
	int			nb_OpCodes;

	// RegBytes only valid for immediate
	// same to OpBytes for E2G or G2E
	int			OpBytes;
	int			RegBytes;
	int			EG;

	uint8_t		ModRM_OpCode;
	int			ModRM_OpCode_used;

	int         nb_regs;
	uint32_t    regs[2];
} scf_x64_OpCode_t;

typedef struct {
	uint32_t		id;
	int				bytes;
	char*			name;

	intptr_t        color;

	scf_vector_t*	dag_nodes;

} scf_register_x64_t;


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

int scf_x64_open(scf_native_t* ctx);
int scf_x64_close(scf_native_t* ctx);
int scf_x64_select(scf_native_t* ctx);

int scf_x64_graph_kcolor(scf_graph_t* graph, int k, scf_vector_t* colors);

scf_register_x64_t*	x64_find_register(const char* name);
scf_register_x64_t* x64_find_register_type_id_bytes(uint32_t type, uint32_t id, int bytes);
scf_register_x64_t* x64_find_register_color(intptr_t color);
scf_register_x64_t*	x64_find_abi_register(int index, int bytes);

scf_register_x64_t* x64_select_overflowed_reg(scf_3ac_code_t* c, uint32_t type, int bytes);

int                 x64_overflow_reg(scf_register_x64_t* r, scf_3ac_code_t* c, scf_function_t* f);

scf_x64_OpCode_t*   x64_find_OpCode_by_type(const int type);
scf_x64_OpCode_t*   x64_find_OpCode(const int type, const int OpBytes, const int RegBytes, const int EG);
int                 x64_find_OpCodes(scf_vector_t* results, const int type, const int OpBytes, const int RegBytes, const int EG);

scf_instruction_t* x64_make_inst_G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r);
scf_instruction_t* x64_make_inst_E(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r);
scf_instruction_t* x64_make_inst_I(scf_x64_OpCode_t* OpCode, uint8_t* imm, int size);

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

#endif

