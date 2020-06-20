#ifndef SCF_NATIVE_H
#define SCF_NATIVE_H

#include"scf_3ac.h"
#include"scf_parse.h"

typedef struct scf_register_s	scf_register_t;
typedef struct scf_OpCode_s		scf_OpCode_t;

struct scf_register_s {
	scf_list_t		list;

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

typedef struct scf_native_ops_s	scf_native_ops_t;

typedef struct {
	scf_native_ops_t*	ops;
	void*				priv;

	scf_list_t*			_3ac_list_head;
	scf_function_t*		function;

} scf_native_context_t;

struct scf_native_ops_s {
	const char*			name;

	int					(*open)(scf_native_context_t* ctx);
	int					(*close)(scf_native_context_t* ctx);

	int 				(*select_instruction)(scf_native_context_t* ctx);

	int					(*write_elf)(scf_native_context_t* ctx, const char* path);
};

int scf_native_open(scf_native_context_t** pctx, const char* name);
int scf_native_close(scf_native_context_t* ctx);

int scf_native_select_instruction(scf_native_context_t* ctx, scf_list_t* _3ac_list_head, scf_function_t* f);

int scf_native_write_elf(scf_native_context_t* ctx, const char* path, scf_list_t* _3ac_list_head, scf_function_t* f);

#endif

