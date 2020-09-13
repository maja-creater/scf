#ifndef SCF_X64_REG_H
#define SCF_X64_REG_H

#include"scf_native.h"
#include"scf_x64_util.h"

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

#define X64_SELECT_REG_CHECK(pr, dn, c, f, load_flag) \
	do {\
		int ret = x64_select_reg(pr, dn, c, f, load_flag); \
		if (ret < 0) { \
			scf_loge("\n"); \
			return ret; \
		} \
		assert(dn->color > 0); \
	} while (0)

// ABI: rdi rsi rdx rcx r8 r9
static uint32_t x64_abi_regs[] =
{
	SCF_X64_REG_RDI,
	SCF_X64_REG_RSI,
	SCF_X64_REG_RDX,
	SCF_X64_REG_RCX,
};

static uint32_t x64_abi_float_regs[] =
{
	SCF_X64_REG_XMM0,
	SCF_X64_REG_XMM1,
	SCF_X64_REG_XMM2,
	SCF_X64_REG_XMM3,
};
#define X64_ABI_NB (sizeof(x64_abi_regs) / sizeof(x64_abi_regs[0]))

typedef struct {
	uint32_t		id;
	int				bytes;
	char*			name;

	intptr_t        color;

	scf_vector_t*	dag_nodes;

} scf_register_x64_t;

typedef struct {
	scf_register_x64_t* base;
	scf_register_x64_t* index;

	int32_t             scale;
	int32_t             disp;

} x64_sib_t;

static inline int x64_variable_size(scf_variable_t* v)
{
	return v->nb_dimentions > 0 ? 8 : v->size;
}

typedef int         (*x64_sib_fill_pt)(x64_sib_t* sib, scf_dag_node_t* base, scf_dag_node_t* index, scf_3ac_code_t* c, scf_function_t* f);

int                 x64_registers_init();
int                 x64_registers_reset();
void                x64_registers_clear();
scf_vector_t*       x64_register_colors();

scf_register_x64_t*	x64_find_register(const char* name);

scf_register_x64_t* x64_find_register_type_id_bytes(uint32_t type, uint32_t id, int bytes);

scf_register_x64_t* x64_find_register_color(intptr_t color);

scf_register_x64_t* x64_find_register_color_bytes(intptr_t color, int bytes);

scf_register_x64_t*	x64_find_abi_register(int index, int bytes);

scf_register_x64_t* x64_select_overflowed_reg(scf_dag_node_t* dn, scf_3ac_code_t* c);

int                 x64_save_var(scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f);

int                 x64_save_var2(scf_dag_node_t* dn, scf_register_x64_t* r, scf_3ac_code_t* c, scf_function_t* f);

int                 x64_save_reg(scf_register_x64_t* r, scf_3ac_code_t* c, scf_function_t* f);

int                 x64_load_reg(scf_register_x64_t* r, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f);

int                 x64_overflow_reg(scf_register_x64_t* r, scf_3ac_code_t* c, scf_function_t* f);

int                 x64_select_reg(scf_register_x64_t** preg, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f, int load_flag);

int                 x64_dereference_reg(x64_sib_t* sib, scf_dag_node_t* base, scf_dag_node_t* member, scf_3ac_code_t* c, scf_function_t* f);

int                 x64_pointer_reg(x64_sib_t* sib, scf_dag_node_t* base, scf_dag_node_t* member, scf_3ac_code_t* c, scf_function_t* f);

int                 x64_array_index_reg(x64_sib_t* sib, scf_dag_node_t* base, scf_dag_node_t* index, scf_dag_node_t* scale, scf_3ac_code_t* c, scf_function_t* f);

#endif
