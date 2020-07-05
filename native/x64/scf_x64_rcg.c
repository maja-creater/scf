#include"scf_x64.h"

static int _x64_rcg_node_cmp(const void* v0, const void* v1)
{
	const scf_graph_node_t* gn1 = v1;
	const x64_rcg_node_t*   rn1 = gn1->data;
	const x64_rcg_node_t*   rn0 = v0;

	if (rn0->dag_node || rn1->dag_node)
		return rn0->dag_node != rn1->dag_node;

	return rn0->reg != rn1->reg;
}

static int _x64_rcg_make_node(scf_graph_node_t** pp, scf_graph_t* g, scf_dag_node_t* dn, scf_register_x64_t* reg, scf_x64_OpCode_t* OpCode)
{
	x64_rcg_node_t* rn = calloc(1, sizeof(x64_rcg_node_t));
	if (!rn)
		return -ENOMEM;

	rn->dag_node = dn;
	rn->reg      = reg;
	rn->OpCode   = OpCode;

	scf_graph_node_t* gn = scf_vector_find_cmp(g->nodes, rn, _x64_rcg_node_cmp);
	if (!gn) {

		gn = scf_graph_node_alloc();
		if (!gn) {
			free(rn);
			return -ENOMEM;
		}

		gn->data = rn;

		if (reg)
			gn->color = reg->color;

		int ret = scf_graph_add_node(g, gn);
		if (ret < 0) {
			free(rn);
			scf_graph_node_free(gn);
			return ret;
		}
	} else {
		if (reg)
			gn->color = reg->color;

		free(rn);
		rn = NULL;
	}

	*pp = gn;
	return 0;
}

static int _x64_rcg_make(scf_3ac_code_t* c, scf_graph_t* g, scf_dag_node_t* dn,
		scf_register_x64_t* reg, scf_x64_OpCode_t* OpCode)
{
	scf_3ac_operand_t* src    = c->srcs->data[0];
	int ret;
	int i;
	scf_graph_node_t* gn_dst = NULL;

	ret = _x64_rcg_make_node(&gn_dst, g, dn, reg, OpCode);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_logd("g->nodes->size: %d, active_vars: %d\n", g->nodes->size, c->active_vars->size);

	for (i = 0; i < c->active_vars->size; i++) {

		scf_active_var_t* active    = c->active_vars->data[i];
		scf_dag_node_t*   dn_active = active->dag_node;
		scf_graph_node_t* gn_active = NULL;

		ret = _x64_rcg_make_node(&gn_active, g, dn_active, NULL, NULL);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		if (gn_active == gn_dst)
			continue;

		if (!scf_vector_find(gn_dst->neighbors, gn_active)) {
			assert(!scf_vector_find(gn_active->neighbors, gn_dst));

			if (dn) {
				scf_logd("v_%d_%d/%s, v_%d_%d/%s\n",
						dn_active->var->w->line, dn_active->var->w->pos, dn_active->var->w->text->data,
						dn->var->w->line, dn->var->w->pos, dn->var->w->text->data);
			} else if (reg) {
				scf_logd("v_%d_%d/%s, r/%s\n",
						dn_active->var->w->line, dn_active->var->w->pos, dn_active->var->w->text->data,
						reg->name);
			} else {
				scf_loge("-EINVAL\n");
				return -EINVAL;
			}

			ret = scf_graph_make_edge(gn_dst, gn_active);
			if (ret < 0)
				return ret;

			ret = scf_graph_make_edge(gn_active, gn_dst);
			if (ret < 0)
				return ret;
		} else
			assert(scf_vector_find(gn_active->neighbors, gn_dst));
	}

	return 0;
}

static int _x64_rcg_call_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	scf_x64_context_t*  x64 = ctx->priv;
	scf_function_t*     f   = x64->f;
	scf_3ac_operand_t*  dst = c->dst;
	scf_register_x64_t* r   = x64_find_register_id_bytes(SCF_X64_REG_AX, c->dst->dag_node->var->size);

	int ret = _x64_rcg_make(c, g, c->dst->dag_node, r, NULL);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	int i;
	int j;
	for (i = 1; i < c->srcs->size - 1; i++) {
		scf_3ac_operand_t*  src0 = c->srcs->data[i];
		scf_graph_node_t*   gn0  = NULL;
		scf_register_x64_t* r0   = NULL;

		if (scf_variable_const(src0->dag_node->var))
			continue;

		if (i - 1 < X64_ABI_NB)
			r0 = x64_find_register_id_bytes(x64_abi_regs[i - 1], src0->dag_node->var->size);

		ret = _x64_rcg_make_node(&gn0, g, src0->dag_node, r0, NULL);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		for (j = i + 1; j < c->srcs->size; j++) {
			scf_3ac_operand_t*  src1 = c->srcs->data[j];
			scf_graph_node_t*   gn1  = NULL;
			scf_register_x64_t* r1   = NULL;

			if (scf_variable_const(src1->dag_node->var))
				continue;

			if (j - 1 < X64_ABI_NB)
				r1 = x64_find_register_id_bytes(x64_abi_regs[j - 1], src0->dag_node->var->size);

			ret = _x64_rcg_make_node(&gn1, g, src1->dag_node, r1, NULL);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			if (gn0 == gn1)
				continue;

			if (!scf_vector_find(gn0->neighbors, gn1)) {
				assert(!scf_vector_find(gn1->neighbors, gn0));

				ret = scf_graph_make_edge(gn0, gn1);
				if (ret < 0)
					return ret;

				ret = scf_graph_make_edge(gn1, gn0);
				if (ret < 0)
					return ret;
			} else
				assert(scf_vector_find(gn1->neighbors, gn0));
		}
	}

	return ret;
}

static int _x64_rcg_array_index_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	scf_loge("\n");
	return -1;
}

static int _x64_rcg_logic_not_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}
static int _x64_rcg_neg_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}

static int _x64_rcg_positive_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}

static int _x64_rcg_dereference_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}

static int _x64_rcg_address_of_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	printf("%s(),%d, error\n", __func__, __LINE__);
	return -1;
}

static int _x64_rcg_div_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 2)
		return -EINVAL;

	scf_3ac_operand_t* src0 = c->srcs->data[0];
	scf_3ac_operand_t* src1 = c->srcs->data[1];

	if (!src0 || !src0->dag_node)
		return -EINVAL;

	if (!src1 || !src1->dag_node)
		return -EINVAL;

	if (src0->dag_node->var->size != src1->dag_node->var->size)
		return -EINVAL;

	int size = src0->dag_node->var->size;

	scf_x64_context_t* x64 = ctx->priv;

	scf_register_x64_t* ax  = x64_find_register_id_bytes(SCF_X64_REG_AX, size);
	scf_register_x64_t* dx  = x64_find_register_id_bytes(SCF_X64_REG_DX, size);

	int ret = _x64_rcg_make(c, g, c->dst->dag_node, ax, NULL);
	if (ret < 0)
		return ret;

	ret = _x64_rcg_make(c, g, NULL, ax, NULL);
	if (ret < 0)
		return ret;

	ret = _x64_rcg_make(c, g, NULL, dx, NULL);
	if (ret < 0)
		return ret;

	return 0;
}

static int _x64_rcg_cast_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	scf_x64_context_t* x64    = ctx->priv;

	return _x64_rcg_make(c, g, c->dst->dag_node, NULL, NULL);
}

static int _x64_rcg_mul_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	if (!c->dst || !c->dst->dag_node)
		return -EINVAL;

	if (!c->srcs || c->srcs->size != 2)
		return -EINVAL;

	scf_3ac_operand_t* src0 = c->srcs->data[0];
	scf_3ac_operand_t* src1 = c->srcs->data[1];

	if (!src0 || !src0->dag_node)
		return -EINVAL;

	if (!src1 || !src1->dag_node)
		return -EINVAL;

	if (src0->dag_node->var->size != src1->dag_node->var->size)
		return -EINVAL;

	int size = src0->dag_node->var->size;

	scf_x64_context_t* x64 = ctx->priv;

	scf_register_x64_t* ax  = x64_find_register_id_bytes(SCF_X64_REG_AX, size);
	scf_register_x64_t* dx  = x64_find_register_id_bytes(SCF_X64_REG_DX, size);

	int ret = _x64_rcg_make(c, g, c->dst->dag_node, ax, NULL);
	if (ret < 0)
		return ret;

	ret = _x64_rcg_make(c, g, NULL, ax, NULL);
	if (ret < 0)
		return ret;

	ret = _x64_rcg_make(c, g, NULL, dx, NULL);
	if (ret < 0)
		return ret;

	return 0;
}

static int _x64_rcg_add_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	scf_x64_context_t* x64    = ctx->priv;

	return _x64_rcg_make(c, g, c->dst->dag_node, NULL, NULL);
}

static int _x64_rcg_sub_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	scf_x64_context_t* x64    = ctx->priv;

	return _x64_rcg_make(c, g, c->dst->dag_node, NULL, NULL);
}

static int _x64_rcg_cmp_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	int i;
	int ret = 0;
	for (i = 0; i < c->active_vars->size; i++) {

		scf_active_var_t* active    = c->active_vars->data[i];
		scf_dag_node_t*   dn_active = active->dag_node;
		scf_graph_node_t* gn_active = NULL;

		ret = _x64_rcg_make_node(&gn_active, g, dn_active, NULL, NULL);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	return 0;
}

static int _x64_rcg_teq_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return 0;
}

static int _x64_rcg_eq_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return _x64_rcg_make(c, g, c->dst->dag_node, NULL, NULL);
}

static int _x64_rcg_ne_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return _x64_rcg_make(c, g, c->dst->dag_node, NULL, NULL);
}

static int _x64_rcg_gt_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return _x64_rcg_make(c, g, c->dst->dag_node, NULL, NULL);
}

static int _x64_rcg_lt_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return _x64_rcg_make(c, g, c->dst->dag_node, NULL, NULL);
}

static int _x64_rcg_assign_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	scf_x64_context_t* x64    = ctx->priv;

	return _x64_rcg_make(c, g, c->dst->dag_node, NULL, NULL);
}

static int _x64_rcg_return_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	assert(1 == c->srcs->size);

	scf_x64_context_t* x64 = ctx->priv;

	return 0;
}

static int _x64_rcg_jz_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return 0;
}

static int _x64_rcg_jnz_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return 0;
	return -1;
}

static int _x64_rcg_jgt_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return 0;
	return -1;
}

static int _x64_rcg_jge_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return 0;
	return -1;
}

static int _x64_rcg_jlt_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return 0;
	return -1;
}

static int _x64_rcg_jle_handler(scf_native_t* ctx, scf_3ac_code_t* c, scf_graph_t* g)
{
	return 0;
}

static x64_rcg_handler_t x64_rcg_handlers[] = {

	{SCF_OP_CALL,			_x64_rcg_call_handler},
	{SCF_OP_ARRAY_INDEX, 	_x64_rcg_array_index_handler},

	{SCF_OP_TYPE_CAST,      _x64_rcg_cast_handler},
	{SCF_OP_LOGIC_NOT, 		_x64_rcg_logic_not_handler},
	{SCF_OP_NEG, 			_x64_rcg_neg_handler},
	{SCF_OP_POSITIVE, 		_x64_rcg_positive_handler},

	{SCF_OP_DEREFERENCE, 	_x64_rcg_dereference_handler},
	{SCF_OP_ADDRESS_OF, 	_x64_rcg_address_of_handler},

	{SCF_OP_MUL, 			_x64_rcg_mul_handler},
	{SCF_OP_DIV, 			_x64_rcg_div_handler},
	{SCF_OP_ADD, 			_x64_rcg_add_handler},
	{SCF_OP_SUB, 			_x64_rcg_sub_handler},

	{SCF_OP_EQ, 			_x64_rcg_eq_handler},
	{SCF_OP_NE, 			_x64_rcg_ne_handler},
	{SCF_OP_GT, 			_x64_rcg_gt_handler},
	{SCF_OP_LT, 			_x64_rcg_lt_handler},

	{SCF_OP_ASSIGN, 		_x64_rcg_assign_handler},

	{SCF_OP_RETURN, 		_x64_rcg_return_handler},

	{SCF_OP_3AC_CMP,         _x64_rcg_cmp_handler},
	{SCF_OP_3AC_TEQ,         _x64_rcg_teq_handler},

	{SCF_OP_3AC_JZ,         _x64_rcg_jz_handler},
	{SCF_OP_3AC_JNZ,        _x64_rcg_jnz_handler},
	{SCF_OP_3AC_JGT,        _x64_rcg_jgt_handler},
	{SCF_OP_3AC_JGE,        _x64_rcg_jge_handler},
	{SCF_OP_3AC_JLT,        _x64_rcg_jlt_handler},
	{SCF_OP_3AC_JLE,        _x64_rcg_jle_handler},
};

x64_rcg_handler_t* scf_x64_find_rcg_handler(const int op_type)
{
	int i;
	for (i = 0; i < sizeof(x64_rcg_handlers) / sizeof(x64_rcg_handlers[0]); i++) {

		x64_rcg_handler_t* h = &(x64_rcg_handlers[i]);

		if (op_type == h->type)
			return h;
	}
	return NULL;
}
