#ifndef SCF_NATIVE_H
#define SCF_NATIVE_H

#include"scf_3ac.h"
#include"scf_parse.h"

typedef struct scf_register_s	scf_register_t;
typedef struct scf_OpCode_s		scf_OpCode_t;
typedef struct scf_native_ops_s	scf_native_ops_t;

struct scf_register_s {
	uint32_t		id;
	int				bytes;
	char*			name;
};

struct scf_OpCode_s {
	int				type;
	char*			name;
};

typedef struct {

	scf_register_t* base;
	scf_register_t* index;
	int             scale;
	int             disp;

	uint64_t        imm;
	int             imm_size;

	uint8_t         flag;
} scf_inst_data_t;

typedef struct {

	scf_3ac_code_t* c;

	scf_OpCode_t*	OpCode;

	scf_inst_data_t src;
	scf_inst_data_t dst;

	uint8_t			code[32];
	int				len;

	int             nb_used;

} scf_instruction_t;

typedef struct {
	scf_3ac_code_t*     code;        // related 3ac code
	scf_function_t*     func;
	scf_variable_t*     var;
	scf_string_t*       name;

	scf_instruction_t*  inst;
	int                 inst_offset; // byte offset in instruction
	int64_t             text_offset; // byte offset in .text segment
	uint64_t            type;
	int                 addend;
} scf_rela_t;

typedef struct {
	scf_native_ops_t*	ops;
	void*				priv;

} scf_native_t;

struct scf_native_ops_s {
	const char*  name;

	int        (*open )(scf_native_t* ctx);
	int        (*close)(scf_native_t* ctx);

	int        (*select_inst)(scf_native_t* ctx, scf_function_t* f);
};

static inline int scf_inst_data_same(scf_inst_data_t* id0, scf_inst_data_t* id1)
{
	// global var, are considered as different.
	if ((id0->flag && !id0->base) || (id1->flag && !id1->base))
		return 0;

	if (id0->base == id1->base
			&& id0->scale == id1->scale
			&& id0->index == id1->index
			&& id0->disp  == id1->disp
			&& id0->flag  == id1->flag
			&& id0->imm   == id1->imm
			&& id0->imm_size == id1->imm_size)
		return 1;
	return 0;
}

void scf_instruction_print(scf_instruction_t* inst);

int scf_native_open(scf_native_t** pctx, const char* name);
int scf_native_close(scf_native_t* ctx);

int scf_native_select_inst(scf_native_t* ctx, scf_function_t* f);

int scf_native_write_elf(scf_native_t* ctx, const char* path, scf_function_t* f);

#endif

