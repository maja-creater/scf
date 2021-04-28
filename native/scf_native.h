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

	scf_OpCode_t*	OpCode;

	uint8_t			code[32];
	int				len;

} scf_instruction_t;

typedef struct {
	scf_3ac_code_t*     code;        // related 3ac code
	scf_function_t*     func;
	scf_variable_t*     var;
	scf_string_t*       name;

	int                 inst_index;  // index in code->instructions
	int                 inst_offset; // byte offset in instruction
	int64_t             text_offset; // byte offset in .text segment
	int                 addend;
} scf_rela_t;

typedef struct {
	scf_native_ops_t*	ops;
	void*				priv;

} scf_native_t;

struct scf_native_ops_s {
	const char*			name;

	int					(*open)(scf_native_t* ctx);
	int					(*close)(scf_native_t* ctx);

	int 				(*select_inst)(scf_native_t* ctx, scf_function_t* f);

	int					(*write_elf)(scf_native_t* ctx, const char* path);
};

int scf_native_open(scf_native_t** pctx, const char* name);
int scf_native_close(scf_native_t* ctx);

int scf_native_select_inst(scf_native_t* ctx, scf_function_t* f);

int scf_native_write_elf(scf_native_t* ctx, const char* path, scf_function_t* f);

#endif

