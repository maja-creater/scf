#include"scf_x64.h"

scf_register_x64_t	x64_registers[] = {

	{0, 1, "al",     X64_COLOR(0, 0, 0x1),  NULL},
	{0, 2, "ax",     X64_COLOR(0, 0, 0x3),  NULL},
	{0, 4, "eax",    X64_COLOR(0, 0, 0xf),  NULL},
	{0, 8, "rax",    X64_COLOR(0, 0, 0xff), NULL},

	{1, 1, "cl",     X64_COLOR(0, 1, 0x1),  NULL},
	{1, 2, "cx",     X64_COLOR(0, 1, 0x3),  NULL},
	{1, 4, "ecx",    X64_COLOR(0, 1, 0xf),  NULL},
	{1, 8, "rcx",    X64_COLOR(0, 1, 0xff), NULL},

	{2, 1, "dl",     X64_COLOR(0, 2, 0x1),  NULL},
	{2, 2, "dx",     X64_COLOR(0, 2, 0x3),  NULL},
	{2, 4, "edx",    X64_COLOR(0, 2, 0xf),  NULL},
	{2, 8, "rdx",    X64_COLOR(0, 2, 0xff), NULL},

	{3, 1, "bl",     X64_COLOR(0, 3, 0x1),  NULL},
	{3, 2, "bx",     X64_COLOR(0, 3, 0x3),  NULL},
	{3, 4, "ebx",    X64_COLOR(0, 3, 0xf),  NULL},
	{3, 8, "rbx",    X64_COLOR(0, 3, 0xff), NULL},

	{4, 2, "sp",     X64_COLOR(0, 4, 0x3),  NULL},
	{4, 4, "esp",    X64_COLOR(0, 4, 0xf),  NULL},
	{4, 8, "rsp",    X64_COLOR(0, 4, 0xff), NULL},

	{5, 2, "bp",     X64_COLOR(0, 5, 0x3),  NULL},
	{5, 4, "ebp",    X64_COLOR(0, 5, 0xf),  NULL},
	{5, 8, "rbp",    X64_COLOR(0, 5, 0xff), NULL},

	{6, 1, "sil",    X64_COLOR(0, 6, 0x1),  NULL},
	{6, 2, "si",     X64_COLOR(0, 6, 0x3),  NULL},
	{6, 4, "esi",    X64_COLOR(0, 6, 0xf),  NULL},
	{6, 8, "rsi",    X64_COLOR(0, 6, 0xff), NULL},

	{7, 1, "dil",    X64_COLOR(0, 7, 0x1),  NULL},
	{7, 2, "di",     X64_COLOR(0, 7, 0x3),  NULL},
	{7, 4, "edi",    X64_COLOR(0, 7, 0xf),  NULL},
	{7, 8, "rdi",    X64_COLOR(0, 7, 0xff), NULL},

	{4, 1, "ah",     X64_COLOR(0, 0, 0x2),  NULL},
	{5, 1, "ch",     X64_COLOR(0, 1, 0x2),  NULL},
	{6, 1, "dh",     X64_COLOR(0, 2, 0x2),  NULL},
	{7, 1, "bh",     X64_COLOR(0, 3, 0x2),  NULL},

	{0, 4, "mm0",    X64_COLOR(1, 0, 0xf),  NULL},
	{0, 8, "xmm0",   X64_COLOR(1, 0, 0xff), NULL},

	{1, 4, "mm1",    X64_COLOR(1, 1, 0xf),  NULL},
	{1, 8, "xmm1",   X64_COLOR(1, 1, 0xff), NULL},

	{2, 4, "mm2",    X64_COLOR(1, 2, 0xf),  NULL},
	{2, 8, "xmm2",   X64_COLOR(1, 2, 0xff), NULL},

	{3, 4, "mm3",    X64_COLOR(1, 3, 0xf),  NULL},
	{3, 8, "xmm3",   X64_COLOR(1, 3, 0xff), NULL},

	{4, 4, "mm4",    X64_COLOR(1, 4, 0xf),  NULL},
	{4, 8, "xmm4",   X64_COLOR(1, 4, 0xff), NULL},

	{5, 4, "mm5",    X64_COLOR(1, 5, 0xf),  NULL},
	{5, 8, "xmm5",   X64_COLOR(1, 5, 0xff), NULL},

	{6, 4, "mm6",    X64_COLOR(1, 6, 0xf),  NULL},
	{6, 8, "xmm6",   X64_COLOR(1, 6, 0xff), NULL},

	{7, 4, "mm7",    X64_COLOR(1, 7, 0xf),  NULL},
	{7, 8, "xmm7",   X64_COLOR(1, 7, 0xff), NULL},
};

static int _x64_reg_cached_vars(scf_register_x64_t* r)
{
	int nb_vars = 0;
	int i;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r2 = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r2->id || SCF_X64_REG_RBP == r2->id)
			continue;

		if (!X64_COLOR_CONFLICT(r->color, r2->color))
			continue;

		nb_vars += r2->dag_nodes->size;
	}

	return nb_vars;
}

int x64_registers_init()
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		assert(!r->dag_nodes);

		r->dag_nodes = scf_vector_alloc();
		if (!r->dag_nodes)
			return -ENOMEM;
	}

	return 0;
}

void x64_registers_clear()
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		if (r->dag_nodes) {
			scf_vector_free(r->dag_nodes);
			r->dag_nodes = NULL;
		}
	}
}

int x64_caller_save_regs(scf_vector_t* instructions, uint32_t* regs, int nb_regs, int stack_size)
{
	int i;
	int j;
	scf_register_x64_t* r;
	scf_register_x64_t* r2;
	scf_instruction_t*  inst;
	scf_register_x64_t* rsp  = x64_find_register("rsp");
	scf_x64_OpCode_t*   mov  = x64_find_OpCode(SCF_X64_MOV,  8,8, SCF_X64_G2E);
	scf_x64_OpCode_t*   push = x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);

	int size = 0;

	for (j = 0; j < nb_regs; j++) {
		r2 = x64_find_register_type_id_bytes(0, regs[j], 8);

		for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {
			r  = &(x64_registers[i]);

			if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
				continue;

			if (0 == r->dag_nodes->size)
				continue;

			if (X64_COLOR_CONFLICT(r2->color, r->color))
				break;
		}

		if (i == sizeof(x64_registers) / sizeof(x64_registers[0]))
			continue;

		if (stack_size > 0)
			inst = x64_make_inst_G2P(mov, rsp, size + stack_size, r2);
		else
			inst = x64_make_inst_G(push, r2);
		X64_INST_ADD_CHECK(instructions, inst);
		size += 8;
	}
	return size;
}

int x64_push_regs(scf_vector_t* instructions, uint32_t* regs, int nb_regs)
{
	int i;
	int j;
	scf_register_x64_t* r;
	scf_register_x64_t* r2;
	scf_instruction_t*  inst;
	scf_x64_OpCode_t*   push = x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);

	for (j = 0; j < nb_regs; j++) {
		r2 = x64_find_register_type_id_bytes(0, regs[j], 8);

		for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {
			r  = &(x64_registers[i]);

			if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
				continue;

			if (0 == r->dag_nodes->size)
				continue;

			if (X64_COLOR_CONFLICT(r2->color, r->color))
				break;
		}

		if (i == sizeof(x64_registers) / sizeof(x64_registers[0]))
			continue;

		inst = x64_make_inst_G(push, r2);
		X64_INST_ADD_CHECK(instructions, inst);
	}
	return 0;
}

int x64_pop_regs(scf_vector_t* instructions, uint32_t* regs, int nb_regs)
{
	int i;
	int j;
	scf_register_x64_t* r;
	scf_register_x64_t* r2;
	scf_instruction_t*  inst;
	scf_x64_OpCode_t*   pop = x64_find_OpCode(SCF_X64_POP, 8,8, SCF_X64_G);

	for (j = nb_regs - 1; j >= 0; j--) {
		r2 = x64_find_register_type_id_bytes(0, regs[j], 8);

		for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {
			r  = &(x64_registers[i]);

			if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
				continue;

			if (0 == r->dag_nodes->size)
				continue;

			if (X64_COLOR_CONFLICT(r2->color, r->color))
				break;
		}

		if (i == sizeof(x64_registers) / sizeof(x64_registers[0]))
			continue;

		inst = x64_make_inst_G(pop, r2);
		X64_INST_ADD_CHECK(instructions, inst);
	}
	return 0;
}

int x64_registers_reset()
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		if (!r->dag_nodes)
			continue;

		int j = 0;
		while (j < r->dag_nodes->size) {
			scf_dag_node_t* dn = r->dag_nodes->data[j];
#if 0
			if (!scf_variable_const(dn->var)
				&& scf_type_is_var(dn->type)
				&& (dn->var->global_flag || dn->var->local_flag)) {

				scf_logw("keep: v_%d_%d/%s\n", dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
				j++;
				continue;
			}
#endif
			if (dn->var->w)
				scf_logw("drop: v_%d_%d/%s\n", dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
			else
				scf_logw("drop: v_%#lx\n", 0xffff & (uintptr_t)dn->var);

			int ret = scf_vector_del(r->dag_nodes, dn);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			dn->loaded     = 0;
			dn->color      = 0;
		}
	}

	return 0;
}

scf_register_x64_t* x64_find_register(const char* name)
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (!strcmp(r->name, name))
			return r;
	}
	return NULL;
}

scf_register_x64_t* x64_find_register_type_id_bytes(uint32_t type, uint32_t id, int bytes)
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (X64_COLOR_TYPE(r->color) == type && r->id == id && r->bytes == bytes)
			return r;
	}
	return NULL;
}

scf_register_x64_t* x64_find_register_color(intptr_t color)
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (r->color == color)
			return r;
	}
	return NULL;
}

scf_register_x64_t* x64_find_register_color_bytes(intptr_t color, int bytes)
{
	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (X64_COLOR_CONFLICT(r->color, color) && r->bytes == bytes)
			return r;
	}
	return NULL;
}

scf_vector_t* x64_register_colors()
{
	scf_vector_t* colors = scf_vector_alloc();
	if (!colors)
		return NULL;

	int i;
	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		int ret = scf_vector_add(colors, (void*)r->color);
		if (ret < 0) {
			scf_vector_free(colors);
			return NULL;
		}
	}
#if 0
	srand(time(NULL));
	for (i = 0; i < colors->size; i++) {
		int j = rand() % colors->size;

		void* t = colors->data[i];
		colors->data[i] = colors->data[j];
		colors->data[j] = t;
	}
#endif
	return colors;
}

int x64_save_var2(scf_dag_node_t* dn, scf_register_x64_t* r, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_variable_t*     v    = dn->var;
	scf_rela_t*         rela = NULL;
	scf_x64_OpCode_t*   mov;
	scf_instruction_t*  inst;

	int var_size = x64_variable_size(v);
	int is_float = scf_variable_float(v);

	assert(var_size == r->bytes);

	if (v->const_literal_flag) {
		scf_logw("const literal var: v_%s_%d_%d not save\n", v->w->text->data, v->w->line, v->w->pos);
		goto end;
	}

	// if temp var in register, alloc it in stack
//	if (!v->global_flag && !v->local_flag) {
	if (0 == v->bp_offset) {

		int local_vars_size  = f->local_vars_size;
		local_vars_size     += var_size;

		if (local_vars_size & 0x7)
			local_vars_size = (local_vars_size + 7) >> 3 << 3;

		v->bp_offset  = -local_vars_size;
		v->local_flag = 1;

		f->local_vars_size = local_vars_size;

		scf_logw("r: %s, temp var, ", r->name);
		if (v->w)
			printf("v_%d_%d/%s, bp_offset: %d\n", v->w->line, v->w->pos, v->w->text->data, v->bp_offset);
		else
			printf("v_%#lx, bp_offset: %d\n", 0xffff & (uintptr_t)v, v->bp_offset);

	} else {
		scf_logw("save var: v_%d_%d/%s, bp_offset: %d, r: %s\n", v->w->line, v->w->pos, v->w->text->data, v->bp_offset, r->name);
	}

	if (is_float) {
		if (SCF_VAR_FLOAT == dn->var->type)
			mov  = x64_find_OpCode(SCF_X64_MOVSS, r->bytes, r->bytes, SCF_X64_G2E);
		else if (SCF_VAR_DOUBLE == dn->var->type)
			mov  = x64_find_OpCode(SCF_X64_MOVSD, r->bytes, r->bytes, SCF_X64_G2E);
	} else
		mov  = x64_find_OpCode(SCF_X64_MOV, r->bytes, r->bytes, SCF_X64_G2E);

	inst = x64_make_inst_G2M(&rela, mov, v, NULL, r);
	X64_INST_ADD_CHECK(c->instructions, inst);
	X64_RELA_ADD_CHECK(f->data_relas, rela, c, v, NULL);

end:
	// if this var is function argment, it become a normal local var
	v->arg_flag =  0;
	dn->color   = -1;
	dn->loaded  =  0;

	int ret = scf_vector_del(r->dag_nodes, dn);
	if (ret < 0) {
		//return ret;
	}
	return 0;
}

int x64_save_var(scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f)
{
	if (dn->color <= 0)
		return -EINVAL;

	scf_register_x64_t* r = x64_find_register_color(dn->color);

	return x64_save_var2(dn, r, c, f);
}

int x64_save_reg(scf_register_x64_t* r, scf_3ac_code_t* c, scf_function_t* f)
{
	int i = 0;
	while (i < r->dag_nodes->size) {

		scf_dag_node_t* dn = r->dag_nodes->data[i];

		scf_logw("r: %s, dn->var: %s\n", r->name, dn->var->w->text->data);
		int ret = x64_save_var(dn, c, f);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	return 0;
}

int x64_overflow_reg(scf_register_x64_t* r, scf_3ac_code_t* c, scf_function_t* f)
{
	int i;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r2 = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r2->id || SCF_X64_REG_RBP == r2->id)
			continue;

		if (!X64_COLOR_CONFLICT(r->color, r2->color))
			continue;

		int ret = x64_save_reg(r2, c, f);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	return 0;
}

int x64_overflow_reg2(scf_register_x64_t* r, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_register_x64_t*	r2;
	scf_dag_node_t*     dn2;

	int i;
	int j;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		r2 = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r2->id || SCF_X64_REG_RBP == r2->id)
			continue;

		if (!X64_COLOR_CONFLICT(r->color, r2->color))
			continue;

		for (j  = 0; j < r2->dag_nodes->size; ) {
			dn2 = r2->dag_nodes->data[j];

			if (dn2 == dn) {
				j++;
				continue;
			}

			int ret = x64_save_var(dn2, c, f);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int _x64_overflow_reg3(scf_register_x64_t* r, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_register_x64_t*	r2;
	scf_dn_status_t*    ds2;
	scf_dag_node_t*     dn2;

	int i;
	int j;
	int ret;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		r2 = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r2->id || SCF_X64_REG_RBP == r2->id)
			continue;

		if (!X64_COLOR_CONFLICT(r->color, r2->color))
			continue;

		for (j  = 0; j < r2->dag_nodes->size; ) {
			dn2 = r2->dag_nodes->data[j];

			if (dn2 == dn) {
				j++;
				continue;
			}

			ds2 = scf_vector_find_cmp(c->active_vars, dn2, scf_dn_status_cmp);
			if (!ds2) {
				j++;
				continue;
			}

			if (!ds2->active) {
				j++;
				continue;
			}
#if 1
			scf_variable_t* v  = dn->var;
			scf_variable_t* v2 = dn2->var;
			if (v->w)
				scf_loge("v_%d_%d/%s, bp_offset: %d\n", v->w->line, v->w->pos, v->w->text->data, v->bp_offset);
			else
				scf_loge("v_%#lx, bp_offset: %d\n", 0xffff & (uintptr_t)v, v->bp_offset);

			if (v2->w)
				scf_loge("v2_%d_%d/%s, bp_offset: %d\n", v2->w->line, v2->w->pos, v2->w->text->data, v2->bp_offset);
			else
				scf_loge("v2_%#lx, bp_offset: %d\n", 0xffff & (uintptr_t)v2, v2->bp_offset);
#endif
			int ret = x64_save_var(dn2, c, f);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

int x64_reg_used(scf_register_x64_t* r, scf_dag_node_t* dn)
{
	scf_register_x64_t*	r2;
	scf_dag_node_t*     dn2;

	int i;
	int j;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		r2 = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r2->id || SCF_X64_REG_RBP == r2->id)
			continue;

		if (!X64_COLOR_CONFLICT(r->color, r2->color))
			continue;

		for (j  = 0; j < r2->dag_nodes->size; j++) {
			dn2 = r2->dag_nodes->data[j];

			if (dn2 != dn)
				return 1;
		}
	}
	return 0;
}

static scf_register_x64_t* _x64_reg_cached_min_vars(scf_register_x64_t** regs, int nb_regs)
{
	scf_register_x64_t* r_min = NULL;

	int min = 0;
	int i;

	for (i = 0; i < nb_regs; i++) {
		scf_register_x64_t*	r = regs[i];

		int nb_vars = _x64_reg_cached_vars(r);

		if (!r_min) {
			r_min = r;
			min   = nb_vars;
			continue;
		}

		if (min > nb_vars) {
			r_min = r;
			min   = nb_vars;
		}
	}
	return r_min;
}

scf_register_x64_t* x64_select_overflowed_reg(scf_dag_node_t* dn, scf_3ac_code_t* c)
{
	scf_vector_t*       neighbors = NULL;
	scf_graph_node_t*   gn        = NULL;

	scf_register_x64_t* free_regs[sizeof(x64_registers) / sizeof(x64_registers[0])];

	int nb_free_regs = 0;
	int is_float     = scf_variable_float(dn->var);
	int bytes        = x64_variable_size(dn->var);
	int ret;
	int i;
	int j;

	assert(c->rcg);

	ret = x64_rcg_find_node(&gn, c->rcg, dn, NULL);
	if (ret < 0)
		neighbors = c->rcg->nodes;
	else
		neighbors = gn->neighbors;

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		if (r->bytes < bytes || X64_COLOR_TYPE(r->color) != is_float)
			continue;

		for (j = 0; j < neighbors->size; j++) {

			scf_graph_node_t* neighbor = neighbors->data[j];
			x64_rcg_node_t*   rn       = neighbor->data;

			if (rn->dag_node) {
				if (rn->dag_node->color <= 0)
					continue;

				if (X64_COLOR_CONFLICT(r->color, rn->dag_node->color))
					break;
			} else {
				assert(rn->reg);

				if (X64_COLOR_CONFLICT(r->color, rn->reg->color))
					break;
			}
		}

		if (j == neighbors->size)
			free_regs[nb_free_regs++] = r;
	}

	if (nb_free_regs > 0)
		return _x64_reg_cached_min_vars(free_regs, nb_free_regs);

	for (i = 0; i < sizeof(x64_registers) / sizeof(x64_registers[0]); i++) {

		scf_register_x64_t*	r = &(x64_registers[i]);

		if (SCF_X64_REG_RSP == r->id || SCF_X64_REG_RBP == r->id)
			continue;

		if (r->bytes < bytes || X64_COLOR_TYPE(r->color) != is_float)
			continue;

		if (c->dst && c->dst->dag_node
				&& c->dst->dag_node->color > 0
				&& X64_COLOR_CONFLICT(r->color, c->dst->dag_node->color))
			continue;

		if (c->srcs) {
			for (j = 0; j < c->srcs->size; j++) {
				scf_3ac_operand_t* src = c->srcs->data[j];

				if (src->dag_node && src->dag_node->color > 0
						&& X64_COLOR_CONFLICT(r->color, src->dag_node->color))
					break;
			}

			if (j < c->srcs->size)
				continue;
		}

		return r;
	}

	scf_loge("\n");
	return NULL;
}

int x64_load_reg(scf_register_x64_t* r, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f)
{
	if (dn->loaded)
		return 0;

	scf_x64_OpCode_t*   mov;
	scf_instruction_t*  inst;
	scf_rela_t*         rela = NULL;

	int is_float = scf_variable_float(dn->var);
	int var_size = x64_variable_size(dn->var);

	if (!is_float) {
		if (scf_variable_const(dn->var)) {
			mov  = x64_find_OpCode(SCF_X64_MOV, var_size, var_size, SCF_X64_I2G);
			inst = x64_make_inst_I2G(mov, r, (uint8_t*)&dn->var->data, dn->var->size);
			X64_INST_ADD_CHECK(c->instructions, inst);

			dn->loaded = 1;
			return 0;
		}

		if (!dn->var->global_flag && !dn->var->local_flag)
			return 0;

		if (scf_variable_const_string(dn->var)) {

			dn->var->global_flag = 1;
			dn->var->local_flag  = 0;

			mov = x64_find_OpCode(SCF_X64_LEA, var_size, var_size, SCF_X64_E2G);

			scf_variable_t* v = dn->var;
			scf_loge("v_%d_%d/%s\n", v->w->line, v->w->pos, v->w->text->data);
		} else {
			if ((dn->var->nb_dimentions > 0 && dn->var->const_literal_flag)
					|| (dn->var->type >= SCF_STRUCT && 0 == dn->var->nb_pointers))
				mov  = x64_find_OpCode(SCF_X64_LEA, var_size, var_size, SCF_X64_E2G);
			else
				mov  = x64_find_OpCode(SCF_X64_MOV, var_size, var_size, SCF_X64_E2G);
		}
	} else {
		if (!dn->var->global_flag && !dn->var->local_flag)
			return 0;

		if (SCF_VAR_FLOAT == dn->var->type)
			mov  = x64_find_OpCode(SCF_X64_MOVSS, var_size, var_size, SCF_X64_E2G);
		else
			mov  = x64_find_OpCode(SCF_X64_MOVSD, var_size, var_size, SCF_X64_E2G);
	}

	if (!mov)
		return -EINVAL;

	inst = x64_make_inst_M2G(&rela, mov, r, NULL, dn->var);
	X64_INST_ADD_CHECK(c->instructions, inst);
	X64_RELA_ADD_CHECK(f->data_relas, rela, c, dn->var, NULL);

	dn->loaded = 1;
	return 0;
}

int x64_select_reg(scf_register_x64_t** preg, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_function_t* f, int load_flag)
{
	if (0 == dn->color)
		return -EINVAL;

	scf_register_x64_t* r;

	int ret;
	int is_float = scf_variable_float(dn->var);
	int var_size = x64_variable_size(dn->var);

	if (dn->color > 0) {
		r   = x64_find_register_color(dn->color);
#if 1
		ret = _x64_overflow_reg3(r, dn, c, f);
		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}
#endif
	} else {
		r   = x64_select_overflowed_reg(dn, c);
		if (!r) {
			scf_loge("\n");
			return -1;
		}

		ret = x64_overflow_reg(r, c, f);
		if (ret < 0) {
			scf_loge("overflow reg failed\n");
			return ret;
		}
		assert(0 == r->dag_nodes->size);

		r = x64_find_register_type_id_bytes(is_float, r->id, var_size);
		assert(0 == r->dag_nodes->size);

		dn->color = r->color;
	}

	ret = scf_vector_add_unique(r->dag_nodes, dn);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	if (load_flag) {
		ret = x64_load_reg(r, dn, c, f);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	} else
		dn->loaded = 1;

	*preg = r;
	return 0;
}

int x64_dereference_reg(x64_sib_t* sib, scf_dag_node_t* base, scf_dag_node_t* member, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_register_x64_t* rb = NULL;

	scf_logw("base->color: %ld\n", base->color);

	int ret  = x64_select_reg(&rb, base, c, f, 1);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}
	scf_logw("base->color: %ld\n", base->color);

	sib->base  = rb;
	sib->index = NULL;
	sib->scale = 0;
	sib->disp  = 0;
	return 0;
}

int x64_pointer_reg(x64_sib_t* sib, scf_dag_node_t* base, scf_dag_node_t* member, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_variable_t*     vb = base  ->var;
	scf_variable_t*     vm = member->var;
	scf_register_x64_t* rb = NULL;

	scf_x64_OpCode_t*   lea;
	scf_x64_OpCode_t*   mov;
	scf_instruction_t*  inst;

	int     ret;
	int32_t disp = 0;

	if (vb->nb_pointers > 0 && 0 == vb->nb_dimentions) {
		ret = x64_select_reg(&rb, base, c, f, 1);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	} else if (vb->local_flag) {
		rb   = x64_find_register("rbp");
		disp = vb->bp_offset;

	} else if (vb->global_flag) {
		scf_rela_t* rela = NULL;

		ret  = x64_select_reg(&rb, base, c, f, 0);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		lea  = x64_find_OpCode(SCF_X64_LEA, 8, 8, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, lea, rb, NULL, vb);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, vb, NULL);

	} else {
		ret = x64_select_reg(&rb, base, c, f, 0);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	disp += vm->offset;

	sib->base  = rb;
	sib->index = NULL;
	sib->scale = 0;
	sib->disp  = disp;
	return 0;
}

int x64_array_index_reg(x64_sib_t* sib, scf_dag_node_t* base, scf_dag_node_t* index, scf_dag_node_t* scale, scf_3ac_code_t* c, scf_function_t* f)
{
	scf_variable_t*     vb    = base ->var;
	scf_variable_t*     vi    = index->var;

	scf_register_x64_t* rb    = NULL;
	scf_register_x64_t* ri    = NULL;

	scf_x64_OpCode_t*   xor;
	scf_x64_OpCode_t*   add;
	scf_x64_OpCode_t*   shl;
	scf_x64_OpCode_t*   lea;
	scf_x64_OpCode_t*   mov;
	scf_instruction_t*  inst;

	int ret;
	int i;

	int32_t disp = 0;

	if (vb->nb_pointers > 0 && 0 == vb->nb_dimentions) {
		ret = x64_select_reg(&rb, base, c, f, 1);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	} else if (vb->local_flag) {
		rb   = x64_find_register("rbp");
		disp = vb->bp_offset;

	} else if (vb->global_flag) {
		scf_rela_t* rela = NULL;

		ret  = x64_select_reg(&rb, base, c, f, 0);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		lea  = x64_find_OpCode(SCF_X64_LEA, 8, 8, SCF_X64_E2G);
		inst = x64_make_inst_M2G(&rela, lea, rb, NULL, vb);
		X64_INST_ADD_CHECK(c->instructions, inst);
		X64_RELA_ADD_CHECK(f->data_relas, rela, c, vb, NULL);
	} else {
		ret = x64_select_reg(&rb, base, c, f, 0);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	int32_t s = scale->var->data.i;
	assert(s > 0);

	if (0 == index->color) {
		disp += vi->data.i * s;

		sib->base  = rb;
		sib->index = NULL;
		sib->scale = 0;
		sib->disp  = disp;
		return 0;
	}

	ret = x64_select_reg(&ri, index, c, f, 1);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_register_x64_t* ri2 = x64_find_register_color_bytes(ri->color, 8);

	if (ri->bytes < ri2->bytes) {

		if (scf_variable_signed(index->var)) {
			mov = x64_find_OpCode(SCF_X64_MOVSX, ri->bytes, ri2->bytes, SCF_X64_E2G);

		} else if (ri->bytes <= 2) {
			mov = x64_find_OpCode(SCF_X64_MOVZX, ri->bytes, ri2->bytes, SCF_X64_E2G);

		} else {
			assert(4 == ri->bytes);

			xor  = x64_find_OpCode(SCF_X64_XOR, 8, 8, SCF_X64_G2E);
			inst = x64_make_inst_G2E(xor, ri2, ri2);
			X64_INST_ADD_CHECK(c->instructions, inst);

			mov  = x64_find_OpCode(SCF_X64_MOV, 4, 4, SCF_X64_E2G);
		}

		inst = x64_make_inst_E2G(mov, ri2, ri);
		X64_INST_ADD_CHECK(c->instructions, inst);

		ri = ri2;
	}

	if (1 != s
			&& 2 != s
			&& 4 != s
			&& 8 != s) {

		assert(8 == scale->var->size);

		scf_register_x64_t* rs = NULL;

		ret = x64_select_reg(&rs, scale, c, f, 0);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		mov  = x64_find_OpCode(SCF_X64_MOV, 8, 8, SCF_X64_G2E);
		inst = x64_make_inst_G2E(mov, rs, ri2);
		X64_INST_ADD_CHECK(c->instructions, inst);

		shl  = x64_find_OpCode(SCF_X64_SHL, 1, 8, SCF_X64_I2E);
		add  = x64_find_OpCode(SCF_X64_ADD, 8, 8, SCF_X64_G2E);

		scf_logw("s: %d\n", s);

		int32_t count = 0;
		int i;
		for (i = 30; i >= 0; i--) {
			if (0 == (s & 1 << i))
				continue;
			scf_logw("s: %d, i: %d\n", s, i);

			if (0 == count) {
				scf_logw("s: %d, i: %d\n", s, i);
				count = i;
				continue;
			}

			count -= i;

			inst = x64_make_inst_I2E(shl, rs, (uint8_t*)&count, 1);
			X64_INST_ADD_CHECK(c->instructions, inst);

			inst = x64_make_inst_G2E(add, rs, ri2);
			X64_INST_ADD_CHECK(c->instructions, inst);

			count = i;
		}

		if (count > 0) {
			inst = x64_make_inst_I2E(shl, rs, (uint8_t*)&count, 1);
			X64_INST_ADD_CHECK(c->instructions, inst);
		}

		ri = rs;
		s  = 1;
	}

	sib->base  = rb;
	sib->index = ri;
	sib->scale = s;
	sib->disp  = disp;
	return 0;
}

