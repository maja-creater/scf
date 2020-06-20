#ifndef SCF_NATIVE_X64_H
#define SCF_NATIVE_X64_H

#include"scf_native.h"
#include"scf_native_x64_util.h"

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

} scf_x64_OpCode_t;

typedef struct {
	scf_list_t		list;

	uint32_t		id;
	int				bytes;
	char*			name;

	scf_vector_t*	dag_nodes;

	scf_vector_t*	impacts;

} scf_register_x64_t;

typedef struct {
	char*		reg;
	char*		impacts[8];
	int			nb_impacts;
} scf_reg_impact_t;

typedef struct {
	scf_list_t			free_registers;
	scf_list_t			busy_registers;

	scf_register_x64_t*	colored_registers[2];
} scf_x64_context_t;

typedef struct {
	int 	type;
	int		(*handler)(scf_native_context_t* ctx, scf_3ac_code_t* _3ac);
} scf_x64_3ac_handler_t;

scf_x64_3ac_handler_t*	scf_x64_find_3ac_handler(const int _3ac_op_type);

int 			scf_x64_open(scf_native_context_t* ctx);
int 			scf_x64_close(scf_native_context_t* ctx);
int 			scf_x64_select(scf_native_context_t* ctx);

scf_register_t*	_x64_find_register(const char* name);

scf_register_t*	_x64_find_free_register(scf_x64_context_t* x64, const int bytes);
scf_register_t*	_x64_find_busy_register(scf_x64_context_t* x64);

void 			_x64_register_make_busy(scf_x64_context_t* x64, scf_register_x64_t* r);
void 			_x64_register_make_free(scf_x64_context_t* x64, scf_register_x64_t* r);

scf_x64_OpCode_t* _x64_find_OpCode(const int type, const int OpBytes, const int RegBytes, const int EG);

scf_instruction_t* _x64_make_instruction_G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r);
scf_instruction_t* _x64_make_instruction_I2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_variable_t* v_src);
scf_instruction_t* _x64_make_instruction_I2E_reg(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_variable_t* v_src);
scf_instruction_t* _x64_make_instruction_I2E_var(scf_x64_OpCode_t* OpCode, scf_variable_t* v_dst, scf_variable_t* v_src);
scf_instruction_t* _x64_make_instruction_I2E_reg_imm32(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, int32_t imm);
scf_instruction_t* _x64_make_instruction_G2E(scf_x64_OpCode_t* OpCode, scf_variable_t* v_dst, scf_register_x64_t* r_src);
scf_instruction_t* _x64_make_instruction_E2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_variable_t* v_src);
scf_instruction_t* _x64_make_instruction_E_var(scf_x64_OpCode_t* OpCode, scf_variable_t* v_src);
scf_instruction_t* _x64_make_instruction_E_reg(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_src);
scf_instruction_t* _x64_make_instruction_G2G(scf_x64_OpCode_t* OpCode, scf_register_x64_t* r_dst, scf_register_x64_t* r_src);

#endif

