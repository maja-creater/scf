#include"scf_native_x64.h"
#include"scf_elf.h"

extern scf_native_ops_t native_ops_x64;

scf_register_x64_t	x64_registers[] = {
	{{NULL, NULL}, 0, 1, "al",	NULL},
	{{NULL, NULL}, 0, 2, "ax",	NULL},
	{{NULL, NULL}, 0, 4, "eax",	NULL},
	{{NULL, NULL}, 0, 8, "rax",	NULL},

	{{NULL, NULL}, 1, 1, "cl",	NULL},
	{{NULL, NULL}, 1, 2, "cx",	NULL},
	{{NULL, NULL}, 1, 4, "ecx",	NULL},
	{{NULL, NULL}, 1, 8, "rcx",	NULL},

	{{NULL, NULL}, 2, 1, "dl",	NULL},
	{{NULL, NULL}, 2, 2, "dx",	NULL},
	{{NULL, NULL}, 2, 4, "edx",	NULL},
	{{NULL, NULL}, 2, 8, "rdx",	NULL},

	{{NULL, NULL}, 3, 1, "bl",	NULL},
	{{NULL, NULL}, 3, 2, "bx",	NULL},
	{{NULL, NULL}, 3, 4, "ebx",	NULL},
	{{NULL, NULL}, 3, 8, "rbx",	NULL},
#if 1
	{{NULL, NULL}, 4, 1, "ah",	NULL},
	{{NULL, NULL}, 4, 2, "sp",	NULL},
	{{NULL, NULL}, 4, 4, "esp",	NULL},
	{{NULL, NULL}, 4, 8, "rsp",	NULL},

	{{NULL, NULL}, 5, 1, "ch",	NULL},
	{{NULL, NULL}, 5, 2, "bp",	NULL},
	{{NULL, NULL}, 5, 4, "ebp",	NULL},
	{{NULL, NULL}, 5, 8, "rbp",	NULL},

	{{NULL, NULL}, 6, 1, "dh",	NULL},
	{{NULL, NULL}, 6, 2, "si",	NULL},
	{{NULL, NULL}, 6, 4, "esi",	NULL},
	{{NULL, NULL}, 6, 8, "rsi",	NULL},

	{{NULL, NULL}, 7, 1, "bh",	NULL},
	{{NULL, NULL}, 7, 2, "di",	NULL},
	{{NULL, NULL}, 7, 4, "edi",	NULL},
	{{NULL, NULL}, 7, 8, "rdi",	NULL},
#endif
};

scf_reg_impact_t	x64_reg_impacts[] = {
	{"al", 		{"ax", "eax", "rax", NULL, NULL, NULL, NULL, NULL}, 3},
	{"ax", 		{"al", "ah", "eax", "rax", NULL, NULL, NULL, NULL}, 4},
	{"eax", 	{"al", "ah", "ax", "rax", NULL, NULL, NULL, NULL}, 4},
	{"rax", 	{"al", "ah", "ax", "eax", NULL, NULL, NULL, NULL}, 4},

	{"cl", 		{"cx", "ecx", "rcx", NULL, NULL, NULL, NULL, NULL}, 3},
	{"cx", 		{"cl", "ch", "ecx", "rcx", NULL, NULL, NULL, NULL}, 4},
	{"ecx", 	{"cl", "ch", "cx", "rcx", NULL, NULL, NULL, NULL}, 4},
	{"rcx", 	{"cl", "ch", "cx", "ecx", NULL, NULL, NULL, NULL}, 4},

	{"dl", 		{"dx", "edx", "rdx", NULL, NULL, NULL, NULL, NULL}, 3},
	{"dx", 		{"dl", "dh", "edx", "rdx", NULL, NULL, NULL, NULL}, 4},
	{"edx", 	{"dl", "dh", "dx", "rdx", NULL, NULL, NULL, NULL}, 4},
	{"rdx", 	{"dl", "dh", "dx", "edx", NULL, NULL, NULL, NULL}, 4},

	{"bl", 		{"bx", "ebx", "rbx", NULL, NULL, NULL, NULL, NULL}, 3},
	{"bx", 		{"bl", "bh", "ebx", "rbx", NULL, NULL, NULL, NULL}, 4},
	{"ebx", 	{"bl", "bh", "bx", "rbx", NULL, NULL, NULL, NULL}, 4},
	{"rbx", 	{"bl", "bh", "bx", "ebx", NULL, NULL, NULL, NULL}, 4},
#if 1
	{"ah", 		{"ax", "eax", "rax", NULL, NULL, NULL, NULL, NULL}, 3},
	{"ch", 		{"cx", "ecx", "rcx", NULL, NULL, NULL, NULL, NULL}, 3},
	{"dh", 		{"dx", "edx", "rdx", NULL, NULL, NULL, NULL, NULL}, 3},
	{"bh", 		{"bx", "ebx", "rbx", NULL, NULL, NULL, NULL, NULL}, 3},

	{"sp", 		{"esp", "rsp", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
	{"esp", 	{"sp", "rsp", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
	{"rsp", 	{"sp", "esp", NULL, NULL, NULL, NULL, NULL, NULL}, 2},

	{"bp", 		{"ebp", "rbp", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
	{"ebp", 	{"bp", "rbp", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
	{"rbp", 	{"bp", "ebp", NULL, NULL, NULL, NULL, NULL, NULL}, 2},

	{"si", 		{"esi", "rsi", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
	{"esi", 	{"si", "rsi", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
	{"rsi", 	{"si", "esi", NULL, NULL, NULL, NULL, NULL, NULL}, 2},

	{"di", 		{"edi", "rdi", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
	{"edi", 	{"di", "rdi", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
	{"rdi", 	{"di", "edi", NULL, NULL, NULL, NULL, NULL, NULL}, 2},
#endif
};

scf_x64_OpCode_t	x64_OpCodes[] = {
	{SCF_X64_PUSH, "push", 1, {0x50, 0x0, 0x0},1,  8,8, SCF_X64_G,  0,0},
	{SCF_X64_POP,  "pop",  1, {0x58, 0x0, 0x0},1,  8,8, SCF_X64_G,  0,0},

	{SCF_X64_INC,  "inc",  2, {0xff, 0x0, 0x0},1,  4,4, SCF_X64_G,  0x0,1},
	{SCF_X64_DEC,  "dec",  2, {0xff, 0x0, 0x0},1,  4,4, SCF_X64_G,  0x1,1},

	{SCF_X64_XOR,  "xor",  2, {0x30, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0},
	{SCF_X64_XOR,  "xor",  2, {0x31, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0},
	{SCF_X64_XOR,  "xor",  2, {0x32, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0},
	{SCF_X64_XOR,  "xor",  2, {0x33, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0},

	{SCF_X64_CALL, "call", 5, {0xe8, 0x0, 0x0},1,  4,4, SCF_X64_I,  0,0},
	{SCF_X64_CALL, "call", 2, {0xff, 0x0, 0x0},1,  8,8, SCF_X64_E,  0x2,1},

	{SCF_X64_RET,  "ret",  1, {0xc3, 0x0, 0x0},1,  8,8, SCF_X64_G,  0,0},

	{SCF_X64_ADD,  "add",  2, {0x00, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0},
	{SCF_X64_ADD,  "add",  2, {0x01, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0},
	{SCF_X64_ADD,  "add",  2, {0x02, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0},
	{SCF_X64_ADD,  "add",  2, {0x03, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0},

	{SCF_X64_ADD,  "add",  2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 0x0,1},
	{SCF_X64_ADD,  "add",  2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 0x0,1},
	{SCF_X64_ADD,  "add",  2, {0x83, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 0x0,1},

	{SCF_X64_SUB,  "sub",  2, {0x28, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0},
	{SCF_X64_SUB,  "sub",  2, {0x29, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0},
	{SCF_X64_SUB,  "sub",  2, {0x2a, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0},
	{SCF_X64_SUB,  "sub",  2, {0x2b, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0},

	{SCF_X64_SUB,  "sub",  2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 0x5,1},
	{SCF_X64_SUB,  "sub",  2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 0x5,1},
	{SCF_X64_SUB,  "sub",  2, {0x83, 0x0, 0x0},1,  1,4, SCF_X64_I2E, 0x5,1},

	{SCF_X64_MUL,  "mul",  2, {0xf6, 0x0, 0x0},1,  1,1, SCF_X64_E, 0x4,1},
	{SCF_X64_MUL,  "mul",  2, {0xf7, 0x0, 0x0},1,  2,2, SCF_X64_E, 0x4,1},
	{SCF_X64_MUL,  "mul",  2, {0xf7, 0x0, 0x0},1,  4,4, SCF_X64_E, 0x4,1},

	{SCF_X64_MOV,  "mov",  2, {0x88, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0},
	{SCF_X64_MOV,  "mov",  2, {0x89, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0},
	{SCF_X64_MOV,  "mov",  2, {0x8a, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0},
	{SCF_X64_MOV,  "mov",  2, {0x8b, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0},

	{SCF_X64_MOV,  "mov",  2, {0xb0, 0x0, 0x0},1,  1,1, SCF_X64_I2G, 0,0},
	{SCF_X64_MOV,  "mov",  2, {0xc6, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 0,1},
	{SCF_X64_MOV,  "mov",  2, {0xc7, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 0,1},
	{SCF_X64_MOV,  "mov",  2, {0xb8, 0x0, 0x0},1,  4,4, SCF_X64_I2G, 0,0},
	{SCF_X64_MOV,  "mov",  2, {0xb8, 0x0, 0x0},1,  8,8, SCF_X64_I2G, 0,0},

	{SCF_X64_MOVSX,  "movsx",  2, {0x0f, 0xbe, 0x0},2,  1,2, SCF_X64_E2G, 0,0},
	{SCF_X64_MOVSX,  "movsx",  2, {0x0f, 0xbe, 0x0},2,  1,4, SCF_X64_E2G, 0,0},
	{SCF_X64_MOVSX,  "movsx",  2, {0x0f, 0xbe, 0x0},2,  2,4, SCF_X64_E2G, 0,0},
	{SCF_X64_MOVSX,  "movsx",  2, {0x63, 0x0, 0x0},1,  4,8, SCF_X64_E2G, 0,0},

	{SCF_X64_MOVZX,  "movzx",  2, {0x0f, 0xb6, 0x0},2,  1,2, SCF_X64_E2G, 0,0},
	{SCF_X64_MOVZX,  "movzx",  2, {0x0f, 0xb6, 0x0},2,  1,4, SCF_X64_E2G, 0,0},
	{SCF_X64_MOVZX,  "movzx",  2, {0x0f, 0xb7, 0x0},2,  2,4, SCF_X64_E2G, 0,0},

	{SCF_X64_CMP,  "cmp",  2, {0x38, 0x0, 0x0},1,  1,1, SCF_X64_G2E, 0,0},
	{SCF_X64_CMP,  "cmp",  2, {0x39, 0x0, 0x0},1,  4,4, SCF_X64_G2E, 0,0},
	{SCF_X64_CMP,  "cmp",  2, {0x3a, 0x0, 0x0},1,  1,1, SCF_X64_E2G, 0,0},
	{SCF_X64_CMP,  "cmp",  2, {0x3b, 0x0, 0x0},1,  4,4, SCF_X64_E2G, 0,0},

	{SCF_X64_CMP,  "cmp",  2, {0x80, 0x0, 0x0},1,  1,1, SCF_X64_I2E, 0x7,1},
	{SCF_X64_CMP,  "cmp",  2, {0x81, 0x0, 0x0},1,  4,4, SCF_X64_I2E, 0x7,1},

	{SCF_X64_SETZ,  "setz",  3, {0x0f, 0x94, 0x0},2,  1,1, SCF_X64_E, 0,0},
	{SCF_X64_SETNZ, "setnz", 3, {0x0f, 0x95, 0x0},2,  1,1, SCF_X64_E, 0,0},

	{SCF_X64_SETG,  "setg",  3, {0x0f, 0x9f, 0x0},2,  1,1, SCF_X64_E, 0,0},
	{SCF_X64_SETGE, "setge", 3, {0x0f, 0x9d, 0x0},2,  1,1, SCF_X64_E, 0,0},

	{SCF_X64_SETL,  "setl",  3, {0x0f, 0x9c, 0x0},2,  1,1, SCF_X64_E, 0,0},
	{SCF_X64_SETLE, "setle", 3, {0x0f, 0x9e, 0x0},2,  1,1, SCF_X64_E, 0,0},

	{SCF_X64_JZ,   "jz",   2, {0x74, 0x0, 0x0},1,  1,0, SCF_X64_I, 0,0},
	{SCF_X64_JZ,   "jz",   6, {0x0f, 0x84, 0x0},2, 4,0, SCF_X64_I, 0,0},
	{SCF_X64_JNZ,  "jnz",  2, {0x75, 0x0, 0x0},1,  1,0, SCF_X64_I, 0,0},
	{SCF_X64_JNZ,  "jnz",  6, {0x0f, 0x85, 0x0},2, 4,0, SCF_X64_I, 0,0},

	{SCF_X64_JMP,  "jmp",  2, {0xeb, 0x0, 0x0},1,  1,0, SCF_X64_I, 0,0},
	{SCF_X64_JMP,  "jmp",  2, {0xe9, 0x0, 0x0},1,  4,0, SCF_X64_I, 0,0},
	{SCF_X64_JMP,  "jmp",  2, {0xff, 0x0, 0x0},1,  8,8, SCF_X64_E, 0x4,1},

};

scf_x64_OpCode_t* _x64_find_OpCode(const int type, const int OpBytes, const int RegBytes, const int EG)
{
	int i;
	for (i = 0; i < sizeof(x64_OpCodes) / sizeof(x64_OpCodes[0]); i++) {

		scf_x64_OpCode_t* OpCode = &(x64_OpCodes[i]);

		if (type == OpCode->type && OpBytes == OpCode->OpBytes
				&& EG == OpCode->EG) {

			if (SCF_X64_I2G == EG || SCF_X64_I2E == EG) {

				if (RegBytes == OpCode->RegBytes) {
					return OpCode;
				}
			} else {
				return OpCode;
			}
		}
	}
	return NULL;
}

scf_register_t* _x64_find_register(const char* name)
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (!strcmp(r->name, name))
			return (scf_register_t*)r;
	}
	return NULL;
}

scf_register_t*	_x64_find_free_register(scf_x64_context_t* x64, int bytes)
{
	scf_register_x64_t* good = NULL;
	int nb_impacts = -1;

	scf_list_t* l;
	for (l = scf_list_head(&x64->free_registers); l != scf_list_sentinel(&x64->free_registers);
			l = scf_list_next(l)) {

		scf_register_x64_t* r = scf_list_data(l, scf_register_x64_t, list);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id) {
			continue;
		}

		if (r->bytes != bytes)
			continue;

		if (!good) {
			good = r;
			nb_impacts = r->impacts->size;
			continue;
		}

		if (r->impacts->size < nb_impacts) {
			good = r;
			nb_impacts = r->impacts->size;
		}
	}

	return (scf_register_t*)good;
}

scf_register_t*	_x64_find_busy_register(scf_x64_context_t* x64)
{
	scf_register_x64_t* good = NULL;
	int	nb_impacts = -1;

	scf_list_t* l;
	for (l = scf_list_head(&x64->busy_registers); l != scf_list_sentinel(&x64->busy_registers);
			l = scf_list_next(l)) {

		scf_register_x64_t* r = scf_list_data(l, scf_register_x64_t, list);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id) {
			continue;
		}

		if (!good) {
			good = r;
			nb_impacts = r->impacts->size;
			continue;
		}

		if (r->impacts->size > nb_impacts) {
			good = r;
			nb_impacts = r->impacts->size;
		}
	}

	if (good) {
		int i;
		for (i = 0; i < good->impacts->size; i++) {
			scf_register_x64_t* r = good->impacts->data[i];
			if (8 == r->bytes) {
				good = r;
				break;
			}
		}
		assert(8 == good->bytes);
	}

	return (scf_register_t*)good;
}

int	scf_x64_open(scf_native_context_t* ctx)
{
	scf_x64_context_t* x64 = calloc(1, sizeof(scf_x64_context_t));
	assert(x64);

	scf_list_init(&x64->free_registers);
	scf_list_init(&x64->busy_registers);

	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {
		scf_register_x64_t*	r = &(x64_registers[i]);

		r->dag_nodes = scf_vector_alloc();
		r->impacts   = scf_vector_alloc();
		scf_list_add_tail(&x64->free_registers, &r->list);
	}

	for (i = 0; i < sizeof(x64_reg_impacts) / sizeof(x64_reg_impacts[0]); i++) {

		scf_reg_impact_t*	impact	= &(x64_reg_impacts[i]);
		scf_register_x64_t*	r		= (scf_register_x64_t*)_x64_find_register(impact->reg);
		assert(r);

		int j;
		for (j = 0; j < impact->nb_impacts; j++) {
			scf_register_x64_t* r2 = (scf_register_x64_t*)_x64_find_register(impact->impacts[j]);
			assert(r2);
			scf_vector_add(r->impacts, r2);
		}
	}

#if 0
	scf_list_t* l;
	for (l = scf_list_head(&x64->free_registers); l != scf_list_sentinel(&x64->free_registers);
			l = scf_list_next(l)) {

		scf_register_x64_t* r = scf_list_data(l, scf_register_x64_t, list);

		printf("%s(),%d, id: %u, bytes: %u, name: %s\n", __func__, __LINE__,
				r->id, r->bytes, r->name);

		int j;
		for (j = 0; j < r->impacts->size; j++) {
			scf_register_x64_t* r2 = r->impacts->data[j];
			printf("%s(),%d, impact: id: %u, bytes: %u, name: %s\n", __func__, __LINE__,
					r2->id, r2->bytes, r2->name);
		}
		printf("\n");
	}
#endif

	ctx->priv = x64;
	return 0;
}

int scf_x64_close(scf_native_context_t* ctx)
{
	scf_x64_context_t* x64 = ctx->priv;

	if (x64) {
		free(x64);
		x64 = NULL;
	}
	return 0;
}

static void* _find_local_vars(scf_node_t* node, void* arg, scf_vector_t* vec)
{
	scf_block_t* b = (scf_block_t*)node;

	if (SCF_OP_BLOCK == b->node.type) {
		assert(b->scope);

		scf_variable_t*	var = NULL;

		int i;
		for (i = 0; i < b->scope->vars->size; i++) {

			var = b->scope->vars->data[i];
			scf_vector_add(vec, var);
		}

		return var;
	}

	return NULL;
}

void _x64_register_make_busy(scf_x64_context_t* x64, scf_register_x64_t* r)
{
//	assert(r->dag_nodes->size > 0);
	scf_list_del(&r->list);
	scf_list_add_front(&x64->busy_registers, &r->list);

	int i;
	for (i = 0; i < r->impacts->size; i++) {
		scf_register_x64_t* r2 = r->impacts->data[i];

		assert(0 == r2->dag_nodes->size);
		scf_list_del(&r2->list);
		scf_list_add_front(&x64->busy_registers, &r2->list);
	}
}

void _x64_register_make_free(scf_x64_context_t* x64, scf_register_x64_t* r)
{
	r->dag_nodes->size = 0;
	scf_list_del(&r->list);
	scf_list_add_front(&x64->free_registers, &r->list);

	int i;
	for (i = 0; i < r->impacts->size; i++) {
		scf_register_x64_t* r2 = r->impacts->data[i];

		r2->dag_nodes->size = 0;
		scf_list_del(&r2->list);
		scf_list_add_front(&x64->free_registers, &r2->list);
	}
}

static int _x64_function_init(scf_native_context_t* ctx, scf_function_t* f, scf_vector_t* local_vars)
{
	int i;
	int local_vars_size = 0;

	for (i = 0; i < local_vars->size; i++) {
		scf_variable_t* v = local_vars->data[i];

		// align 8 bytes
		if (local_vars_size & 0x7) {
			local_vars_size = (local_vars_size + 7) >> 3 << 3;
		}

		v->bp_offset	= local_vars_size;
		v->local_flag	= 1;
		local_vars_size	+= v->size;
	}

	scf_list_t*     l = scf_list_head(ctx->_3ac_list_head);
	scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

	if (!c->instructions)
		c->instructions = scf_vector_alloc();

	scf_x64_OpCode_t* push = _x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* pop  = _x64_find_OpCode(SCF_X64_POP, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* mov  = _x64_find_OpCode(SCF_X64_MOV, 4,4, SCF_X64_G2E);
	scf_x64_OpCode_t* sub  = _x64_find_OpCode(SCF_X64_SUB, 4,4, SCF_X64_I2E);

	scf_register_x64_t* rsp = (scf_register_x64_t*)_x64_find_register("rsp");
	scf_register_x64_t* rbp = (scf_register_x64_t*)_x64_find_register("rbp");

	scf_vector_add(c->instructions, _x64_make_instruction_G(push, rbp));
	scf_vector_add(c->instructions, _x64_make_instruction_G2G(mov, rbp, rsp));
	scf_vector_add(c->instructions, _x64_make_instruction_I2E_reg_imm32(sub, rsp, local_vars_size));

	return local_vars_size;
}

static void _x64_function_finish(scf_native_context_t* ctx)
{
	scf_list_t*     l = scf_list_tail(ctx->_3ac_list_head);
	scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

	if (!c->instructions)
		c->instructions = scf_vector_alloc();

	scf_x64_OpCode_t* push = _x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* pop  = _x64_find_OpCode(SCF_X64_POP, 8,8, SCF_X64_G);
	scf_x64_OpCode_t* mov  = _x64_find_OpCode(SCF_X64_MOV, 4,4, SCF_X64_G2E);

	scf_register_x64_t* rsp = (scf_register_x64_t*)_x64_find_register("rsp");
	scf_register_x64_t* rbp = (scf_register_x64_t*)_x64_find_register("rbp");

	scf_vector_add(c->instructions, _x64_make_instruction_G2G(mov, rsp, rbp));
	scf_vector_add(c->instructions, _x64_make_instruction_G(pop, rbp));
}

int	_scf_x64_select_instruction(scf_native_context_t* ctx)
{
	scf_x64_context_t*	x64 = ctx->priv;

	scf_list_t* l;

	scf_graph_t* graph = scf_graph_alloc();

	for (l = scf_list_head(ctx->_3ac_list_head); l != scf_list_sentinel(ctx->_3ac_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		if (SCF_OP_CALL == c->op->type
				|| SCF_OP_RETURN  == c->op->type
				|| SCF_OP_3AC_JE  == c->op->type
				|| SCF_OP_3AC_JNE == c->op->type
				|| SCF_OP_GOTO    == c->op->type)
			continue;

		scf_3ac_code_make_register_conflict_graph(c, graph);
	}

	scf_vector_t* colors = scf_vector_alloc();
	int i;
	for (i = 0; i < sizeof(x64->colored_registers) / sizeof(x64->colored_registers[0]); i++)
		scf_vector_add(colors, (void*)(intptr_t)(i + 1));

	scf_graph_k_color(graph, colors->size, colors);

	for (i = 0; i < graph->nodes->size; i++) {
		scf_graph_node_t* gn = graph->nodes->data[i];
		scf_dag_node_t*   dn = gn->data;
		dn->color = gn->color;
#if 1
		printf("v_%d_%d/%s, color: %d\n",
				dn->var->w->line, dn->var->w->pos, dn->var->w->text->data, dn->color);
#endif
	}

	scf_graph_free(graph);
	graph = NULL;

	scf_vector_free(colors);
	colors = NULL;

	for (l = scf_list_head(ctx->_3ac_list_head); l != scf_list_sentinel(ctx->_3ac_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		scf_x64_3ac_handler_t* h = scf_x64_find_3ac_handler(c->op->type);
		h->handler(ctx, c);
	}

	int bytes = 0;
	for (l = scf_list_head(ctx->_3ac_list_head); l != scf_list_sentinel(ctx->_3ac_list_head);
			l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);
		int i;
#if 1
		_3ac_code_print(ctx->_3ac_list_head, c);
		for (i = 0; i < c->instructions->size; i++) {
			scf_instruction_t* inst = c->instructions->data[i];
			int j;
			for (j = 0; j < inst->len; j++) {
				printf("%02x ", inst->code[j]);
			}
			printf("\n");
		}
		printf("\n");
#endif
	}

	return 0;
}

int scf_x64_select_instruction(scf_native_context_t* ctx)
{
	scf_x64_context_t* x64 = ctx->priv;

	scf_function_t* f = ctx->function;

	scf_vector_t* local_vars = scf_vector_alloc();

	scf_node_search_bfs((scf_node_t*)f, _find_local_vars, NULL, local_vars);

	int local_vars_size = _x64_function_init(ctx, f, local_vars);

	int i;
	for (i = 0; i < local_vars->size; i++) {
		scf_variable_t* v = local_vars->data[i];
		assert(v->w);
		printf("%s(),%d, v: %p, name: %s, line: %d, pos: %d, size: %d, bp_offset: %d\n",
				__func__, __LINE__,
				v, v->w->text->data, v->w->line, v->w->pos,
				v->size, v->bp_offset);
	}
	printf("%s(),%d, local_vars_size: %d\n", __func__, __LINE__, local_vars_size);

	int ret = _scf_x64_select_instruction(ctx);

	_x64_function_finish(ctx);
	return 0;
}

int scf_x64_write_elf(scf_native_context_t* ctx, const char* path)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}

scf_native_ops_t	native_ops_x64 = {
	.name				= "x64",

	.open				= scf_x64_open,
	.close				= scf_x64_close,

	.select_instruction	= scf_x64_select_instruction,
	.write_elf			= scf_x64_write_elf,
};


























