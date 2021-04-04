#include"scf_x64.h"
#include"scf_elf.h"
#include"scf_basic_block.h"
#include"scf_3ac.h"

extern scf_native_ops_t native_ops_x64;

int	scf_x64_open(scf_native_t* ctx)
{
	scf_x64_context_t* x64 = calloc(1, sizeof(scf_x64_context_t));
	if (!x64)
		return -ENOMEM;

	ctx->priv = x64;
	return 0;
}

int scf_x64_close(scf_native_t* ctx)
{
	scf_x64_context_t* x64 = ctx->priv;

	if (x64) {
		x64_registers_clear();

		free(x64);
		x64 = NULL;
	}
	return 0;
}

static int _x64_function_init(scf_function_t* f, scf_vector_t* local_vars)
{
	int ret = x64_registers_init();
	if (ret < 0)
		return ret;

	int i;
	int local_vars_size = 0;

	for (i = 0; i < local_vars->size; i++) {
		scf_variable_t* v = local_vars->data[i];

		int size = scf_variable_size(v);
		if (size < 0)
			return size;

		// local var in the low memory address based on rbp
		// (rbp) is old rbp, -8(rbp) is 1st local var
		local_vars_size	+= size;

		// align 8 bytes
		if (local_vars_size & 0x7)
			local_vars_size = (local_vars_size + 7) >> 3 << 3;

		v->bp_offset	 = -local_vars_size;
		v->local_flag	 = 1;
	}

	return local_vars_size;
}

static int _x64_function_finish(scf_function_t* f)
{
	if (!f->init_insts) {
		f->init_insts = scf_vector_alloc();
		if (!f->init_insts)
			return -ENOMEM;
	} else
		scf_vector_clear(f->init_insts, free);

	scf_x64_OpCode_t*   push = x64_find_OpCode(SCF_X64_PUSH, 8,8, SCF_X64_G);
	scf_x64_OpCode_t*   pop  = x64_find_OpCode(SCF_X64_POP,  8,8, SCF_X64_G);
	scf_x64_OpCode_t*   mov  = x64_find_OpCode(SCF_X64_MOV,  4,4, SCF_X64_G2E);
	scf_x64_OpCode_t*   sub  = x64_find_OpCode(SCF_X64_SUB,  4,4, SCF_X64_I2E);

	scf_register_x64_t* rsp  = x64_find_register("rsp");
	scf_register_x64_t* rbp  = x64_find_register("rbp");
	scf_instruction_t*  inst = NULL;

	inst = x64_make_inst_G(push, rbp);
	X64_INST_ADD_CHECK(f->init_insts, inst);
	f->init_code_bytes  = inst->len;

	inst = x64_make_inst_G2E(mov, rbp, rsp);
	X64_INST_ADD_CHECK(f->init_insts, inst);
	f->init_code_bytes += inst->len;

	inst = x64_make_inst_I2E(sub, rsp, (uint8_t*)&f->local_vars_size, 4);
	X64_INST_ADD_CHECK(f->init_insts, inst);
	f->init_code_bytes += inst->len;

	x64_registers_clear();
	return 0;
}

static void _x64_rcg_node_printf(x64_rcg_node_t* rn)
{
	if (rn->dag_node) {
		scf_variable_t* v = rn->dag_node->var;

		if (v->w) {
			scf_logw("v_%d_%d/%s, bp_offset: -%#x, color: %ld, type: %ld, id: %ld, mask: %ld",
					v->w->line, v->w->pos, v->w->text->data,
					-v->bp_offset,
					rn->dag_node->color,
					X64_COLOR_TYPE(rn->dag_node->color),
					X64_COLOR_ID(rn->dag_node->color),
					X64_COLOR_MASK(rn->dag_node->color));
		} else {
			scf_logw("v_%#lx, color: %ld, type: %ld, id: %ld, mask: %ld",
					(uintptr_t)v & 0xffff, rn->dag_node->color,
					X64_COLOR_TYPE(rn->dag_node->color),
					X64_COLOR_ID(rn->dag_node->color),
					X64_COLOR_MASK(rn->dag_node->color));
		}

		if (rn->dag_node->color > 0) {
			scf_register_x64_t* r = x64_find_register_color(rn->dag_node->color);
			printf(", reg: %s\n", r->name);
		} else {
			printf("\n");
		}
	} else if (rn->reg) {
		scf_logw("r/%s, color: %ld, type: %ld, major: %ld, minor: %ld\n",
				rn->reg->name, rn->reg->color,
				X64_COLOR_TYPE(rn->reg->color),
				X64_COLOR_ID(rn->reg->color),
				X64_COLOR_MASK(rn->reg->color));
	}
}

static void _x64_inst_printf(scf_3ac_code_t* c)
{
	if (!c->instructions)
		return;

	int i;
	for (i = 0; i < c->instructions->size; i++) {
		scf_instruction_t* inst = c->instructions->data[i];
		int j;
		for (j = 0; j < inst->len; j++)
			printf("%02x ", inst->code[j]);
		printf("\n");
	}
	printf("\n");
}

static int _x64_argv_prepare(scf_graph_t* g, scf_basic_block_t* bb, scf_function_t* f)
{
	int i;
	int nb_ints   = 0;
	int nb_floats = 0;

	for (i = 0; i < f->argv->size; i++) {
		scf_variable_t*     v = f->argv->data[i];

		if (!v->arg_flag)
			continue;

		scf_list_t*         l;
		scf_graph_node_t*   gn;
		scf_dag_node_t*     dn;
		scf_register_x64_t* rabi = NULL;

		int is_float = scf_variable_float(v);
		int size     = x64_variable_size(v);

		if (is_float) {
			if (nb_floats < X64_ABI_NB) {
				rabi = x64_find_register_type_id_bytes(is_float, x64_abi_float_regs[nb_floats], size);
				nb_floats++;
			}
		} else if (nb_ints < X64_ABI_NB) {
			rabi = x64_find_register_type_id_bytes(is_float, x64_abi_regs[nb_ints], size);
			nb_ints++;
		}

		for (l = scf_list_head(&f->dag_list_head); l != scf_list_sentinel(&f->dag_list_head);
				l = scf_list_next(l)) {

			dn = scf_list_data(l, scf_dag_node_t, list);
			if (dn->var == v)
				break;
		}

		// if arg is not found in dag, it's not used in function, ignore.
		if (l == scf_list_sentinel(&f->dag_list_head))
			continue;

		int ret = _x64_rcg_make_node(&gn, g, dn, rabi, NULL);
		if (ret < 0)
			return ret;

		dn->rabi   = rabi;

		if (dn->rabi)
			dn->loaded =  1;
		else
			dn->color  = -1;
	}

	return 0;
}

static int _x64_argv_save(scf_basic_block_t* bb, scf_function_t* f)
{
	assert(!scf_list_empty(&bb->code_list_head));

	scf_list_t*     l = scf_list_head(&bb->code_list_head);
	scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	int i;
	for (i = 0; i < f->argv->size; i++) {
		scf_variable_t*     v = f->argv->data[i];

		if (!v->arg_flag)
			continue;

		scf_dag_node_t*     dn;
		scf_dag_node_t*     dn2;
		scf_dn_status_t*   active;
		scf_register_x64_t* rabi;

		for (l = scf_list_head(&f->dag_list_head); l != scf_list_sentinel(&f->dag_list_head);
				l = scf_list_next(l)) {

			dn = scf_list_data(l, scf_dag_node_t, list);
			if (dn->var == v)
				break;
		}

		if (l == scf_list_sentinel(&f->dag_list_head))
			continue;

		if (!dn->rabi)
			continue;

		rabi = dn->rabi;

		int save_flag = 0;
		int j;

		if (dn->color > 0) {
			if (dn->color != rabi->color)
				save_flag = 1;
		} else
			save_flag = 1;

		for (j = 0; j < bb->dn_colors->size; j++) {
			active = bb->dn_colors->data[j];
			dn2    = active->dag_node;

			if (dn2 != dn && dn2->color > 0
					&& X64_COLOR_CONFLICT(dn2->color, rabi->color)) {
				save_flag = 1;
				break;
			}
		}

		if (save_flag) {
			int ret = x64_save_var2(dn, rabi, c, f);
			if (ret < 0)
				return ret;
		} else {
			assert(0 == rabi->dag_nodes->size);

			int ret = scf_vector_add(rabi->dag_nodes, dn);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int _x64_make_bb_rcg(scf_graph_t* g, scf_basic_block_t* bb, scf_native_t* ctx)
{
	scf_list_t*        l;
	scf_3ac_code_t*    c;
	x64_rcg_handler_t* h;

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); l = scf_list_next(l)) {

		c = scf_list_data(l, scf_3ac_code_t, list);

		h = scf_x64_find_rcg_handler(c->op->type);
		if (!h) {
			scf_loge("3ac operator '%s' not supported\n", c->op->name);
			return -EINVAL;
		}

		int ret = h->func(ctx, c, g);
		if (ret < 0)
			return ret;
	}

	scf_logd("g->nodes->size: %d\n", g->nodes->size);
	return 0;
}

static int _x64_bb_regs_from_graph(scf_basic_block_t* bb, scf_graph_t* g)
{
	int i;
	for (i = 0; i < g->nodes->size; i++) {
		scf_graph_node_t*   gn = g->nodes->data[i];
		x64_rcg_node_t*     rn = gn->data;
		scf_dag_node_t*     dn = rn->dag_node;
		scf_register_x64_t* r  = NULL;
		scf_dn_status_t*   v;

		if (!dn) {
			_x64_rcg_node_printf(rn);
			continue;
		}

		v = scf_dn_status_alloc(dn);
		if (!v)
			return -ENOMEM;

		int ret = scf_vector_add(bb->dn_colors, v);
		if (ret < 0) {
			scf_dn_status_free(v);
			return ret;
		}
		v ->color = gn->color;
		dn->color = gn->color;

		_x64_rcg_node_printf(rn);
	}
	printf("\n");

	return 0;
}

static int _x64_select_bb_regs(scf_basic_block_t* bb, scf_native_t* ctx)
{
	scf_x64_context_t*	x64 = ctx->priv;
	scf_function_t*     f   = x64->f;

	scf_graph_t* g = scf_graph_alloc();
	if (!g)
		return -ENOMEM;

	scf_vector_t*   colors = NULL;
	scf_list_t*     l;
	scf_3ac_code_t* c;

	int ret = 0;
	int i;

	if (0 == bb->index) {
		scf_logw("bb->index: %d\n", bb->index);

		ret = _x64_argv_prepare(g, bb, f);
		if (ret < 0)
			goto error;
	}

	ret = _x64_make_bb_rcg(g, bb, ctx);
	if (ret < 0)
		goto error;

	colors = x64_register_colors();
	if (!colors) {
		ret = -ENOMEM;
		goto error;
	}

	ret = scf_x64_graph_kcolor(g, 8, colors);
	if (ret < 0)
		goto error;

	ret = _x64_bb_regs_from_graph(bb, g);

error:
	if (colors)
		scf_vector_free(colors);

	scf_graph_free(g);
	g = NULL;
	return ret;
}


static int _x64_select_bb_group_regs(scf_bb_group_t* bbg, scf_native_t* ctx)
{
	scf_graph_t* g = scf_graph_alloc();
	if (!g)
		return -ENOMEM;

	scf_vector_t*      colors = NULL;
	scf_list_t*        l;
	scf_3ac_code_t*    c;
	scf_basic_block_t* bb;

	int ret = 0;
	int i;

	for (i = 0; i < bbg->body->size; i++) {
		bb  = bbg->body->data[i];

		ret = _x64_make_bb_rcg(g, bb, ctx);
		if (ret < 0)
			goto error;
	}

	colors = x64_register_colors();
	if (!colors) {
		ret = -ENOMEM;
		goto error;
	}

	ret = scf_x64_graph_kcolor(g, 8, colors);
	if (ret < 0)
		goto error;

	ret = _x64_bb_regs_from_graph(bbg->pre, g);
	if (ret < 0)
		goto error;

error:
	if (colors)
		scf_vector_free(colors);

	scf_graph_free(g);
	g = NULL;
	return ret;
}

static int _x64_make_insts_for_list(scf_native_t* ctx, scf_list_t* h, int bb_offset)
{
	scf_list_t* l;
	int ret;

	for (l = scf_list_head(h); l != scf_list_sentinel(h); l = scf_list_next(l)) {

		scf_3ac_code_t* c = scf_list_data(l, scf_3ac_code_t, list);

		x64_inst_handler_t* h = scf_x64_find_inst_handler(c->op->type);
		if (!h) {
			scf_loge("3ac operator '%s' not supported\n", c->op->name);
			return -EINVAL;
		}

		ret = h->func(ctx, c);
		if (ret < 0) {
			scf_3ac_code_print(c, NULL);
			scf_loge("3ac op '%s' make inst failed\n", c->op->name);
			return ret;
		}

		if (!c->instructions)
			continue;

		int inst_bytes = 0;
		int i;
		for (i = 0; i < c->instructions->size; i++) {
			scf_instruction_t* inst = c->instructions->data[i];

			inst_bytes += inst->len;
		}
		c->inst_bytes  = inst_bytes;
		c->bb_offset   = bb_offset;
		bb_offset     += inst_bytes;

		scf_3ac_code_print(c, NULL);
		_x64_inst_printf(c);
	}

	return bb_offset;
}

static void _x64_set_offset_for_jmps(scf_native_t* ctx, scf_function_t* f)
{
	while (1) {
		int drop_bytes = 0;
		int i;

		for (i = 0; i < f->jmps->size; i++) {
			scf_3ac_code_t*    c      = f->jmps->data[i];

			scf_basic_block_t* cur_bb = c->basic_block;
			scf_basic_block_t* dst_bb = c->dst->bb;

			scf_basic_block_t* bb     = NULL;
			scf_list_t*        l      = NULL;
			int32_t            bytes  = 0;

			if (cur_bb->index < dst_bb->index) {
				for (l = scf_list_next(&cur_bb->list); l != &dst_bb->list; l = scf_list_next(l)) {

					bb     = scf_list_data(l, scf_basic_block_t, list);

					bytes += bb->code_bytes;
				}
			} else {
				for (l = &cur_bb->list; l != scf_list_prev(&dst_bb->list); l = scf_list_prev(l)) {

					bb     = scf_list_data(l, scf_basic_block_t, list);

					bytes -= bb->code_bytes;
				}
			}

			assert(c->instructions && 1 == c->instructions->size);

			int nb_bytes;
			if (-128 <= bytes && bytes <= 127)
				nb_bytes = 1;
			else
				nb_bytes = 4;

			scf_instruction_t* inst = c->instructions->data[0];
			scf_x64_OpCode_t*  jcc  = x64_find_OpCode(inst->OpCode->type, nb_bytes, nb_bytes, SCF_X64_I);

			int old_len = inst->len;
			x64_make_inst_I2(inst, jcc, (uint8_t*)&bytes, nb_bytes);
			int diff    = old_len - inst->len;
			assert(diff >= 0);

			cur_bb->code_bytes -= diff;
			c->inst_bytes      -= diff;
			drop_bytes         += diff;
		}

		if (0 == drop_bytes)
			break;
	}
}

static void _x64_set_offset_for_relas(scf_native_t* ctx, scf_function_t* f, scf_vector_t* relas)
{
	int i;
	for (i = 0; i < relas->size; i++) {

		scf_rela_t*        rela   = relas->data[i];
		scf_3ac_code_t*    c      = rela->code;
		scf_instruction_t* inst   = c->instructions->data[rela->inst_index];
		scf_basic_block_t* cur_bb = c->basic_block;

		scf_instruction_t* inst2;
		scf_basic_block_t* bb;
		scf_list_t*        l;

		int bytes = f->init_code_bytes;

		for (l = scf_list_head(&f->basic_block_list_head); l != &cur_bb->list;
				l = scf_list_next(l)) {

			bb     = scf_list_data(l, scf_basic_block_t, list);
			bytes += bb->code_bytes;
		}

		bytes += c->bb_offset;

		int j;
		for (j = 0; j < rela->inst_index; j++) {
			inst2  = c->instructions->data[j];
			bytes += inst2->len;
		}

		rela->inst_offset += bytes;
	}
}

static void _x64_init_bb_colors(scf_basic_block_t* bb)
{
	scf_dag_node_t*   dn;
	scf_dn_status_t*  ds;

	int i;

	x64_registers_reset();

	for (i = 0; i < bb->dn_colors->size; i++) {
		ds = bb->dn_colors->data[i];
		dn = ds->dag_node;

		dn->color  = ds->color;
		dn->loaded = 0;

		if (0 == bb->index && dn->rabi)
			dn->loaded = 1;
	}
}

static int _x64_save_bb_colors(scf_basic_block_t* bb, scf_bb_group_t* bbg)
{
	scf_dag_node_t*   dn;
	scf_dn_status_t*  ds0;
	scf_dn_status_t*  ds1;

	int i;
	int j;

	for (i  = 0; i < bbg->pre->dn_colors->size; i++) {

		ds0 = bbg->pre->dn_colors->data[i];
		dn  = ds0->dag_node;

		for (j  = 0; j < bb->dn_colors->size; j++) {
			ds1 =        bb->dn_colors->data[j];

			if (ds1->dag_node == dn)
				break;
		}

		if (j == bb->dn_colors->size) {
			ds1 = scf_dn_status_alloc(dn);
			if (!ds1)
				return -ENOMEM;

			int ret = scf_vector_add(bb->dn_colors, ds1);
			if (ret < 0) {
				scf_dn_status_free(ds1);
				return ret;
			}
		}

		ds1->color = dn->color;

		scf_variable_t* v = dn->var;
		scf_logd("bb: %p, v_%d_%d/%s, dn->color: %ld\n",
				bb, v->w->line, v->w->pos, v->w->text->data,
				dn->color);
	}
	return 0;
}

static intptr_t _x64_bb_find_color(scf_basic_block_t* bb, scf_dag_node_t* dn)
{
	scf_dn_status_t* ds = NULL;

	int j;
	for (j = 0; j < bb->dn_colors->size; j++) {
		ds =        bb->dn_colors->data[j];

		if (ds->dag_node == dn)
			break;
	}
	assert(j < bb->dn_colors->size);

	return ds->color;
}

static int _x64_bb_save_dn(intptr_t color, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_function_t* f)
{
	scf_variable_t*     v = dn->var;
	scf_register_x64_t* r;
	scf_instruction_t*  inst;

	int inst_bytes;
	int ret;
	int i;

	scf_logd("add save: v_%d_%d/%s, color: %ld\n",
			v->w->line, v->w->pos, v->w->text->data, color);

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	inst_bytes = c->inst_bytes;

	r   = x64_find_register_color(color);

	ret = x64_save_var2(dn, r, c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	c->inst_bytes = 0;
	for (i = 0; i < c->instructions->size; i++) {
		inst      = c->instructions->data[i];
		c->inst_bytes += inst->len;
	}
	bb->code_bytes += c->inst_bytes - inst_bytes;
	return 0;
}

static int _x64_bbg_fix_saves(scf_bb_group_t* bbg, scf_function_t* f)
{
	scf_basic_block_t* pre;
	scf_basic_block_t* post;
	scf_basic_block_t* bb;
	scf_3ac_operand_t* src;
	scf_dag_node_t*    dn;
	scf_3ac_code_t*    c;
	scf_list_t*        l;
	scf_list_t*        l2;

	int i;
	int j;

	pre  = bbg->pre;
	post = bbg->post;

	for (l = scf_list_head(&post->save_list_head); l != scf_list_sentinel(&post->save_list_head); ) {
		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		assert(c->srcs && 1 == c->srcs->size);
		src = c->srcs->data[0];
		dn  = src->dag_node;

		scf_variable_t* v = dn->var;

		intptr_t color   = _x64_bb_find_color(pre, dn);
		int      updated = 0;

		for (i = 0; i < bbg->body->size; i++) {
			bb =        bbg->body->data[i];

			intptr_t color2 = _x64_bb_find_color(bb, dn);

			if (color2 != color)
				updated++;
		}

		if (0 == updated) {
			if (color <= 0)
				continue;

			int ret = _x64_bb_save_dn(color, dn, c, post, f);
			if (ret < 0)
				return ret;

			scf_list_del(&c->list);
			scf_list_add_tail(&post->code_list_head, &c->list);
		} else {
			scf_list_del(&c->list);
			scf_3ac_code_free(c);
			c = NULL;
#if 0
			scf_variable_t* v = dn->var;
			scf_loge("save: v_%d_%d/%s, dn->color: %ld\n",
					v->w->line, v->w->pos, v->w->text->data, dn->color);
#endif
			for (i = 0; i < bbg->body->size; i++) {
				bb =        bbg->body->data[i];

				for (l2 = scf_list_head(&bb->save_list_head); l2 != scf_list_sentinel(&bb->save_list_head); l2 = scf_list_next(l2)) {
					c   = scf_list_data(l2, scf_3ac_code_t, list);

					assert(c->srcs && 1 == c->srcs->size);
					src = c->srcs->data[0];

					if (dn == src->dag_node)
						break;
				}
				if (l2 == scf_list_sentinel(&bb->save_list_head))
					continue;

				intptr_t color = _x64_bb_find_color(bb, dn);
				if (color <= 0)
					continue;

				int ret = _x64_bb_save_dn(color, dn, c, bb, f);
				if (ret < 0)
					return ret;

				scf_list_del(&c->list);
				scf_list_add_tail(&bb->code_list_head, &c->list);
			}
		}
	}

	return 0;
}

static void _x64_bbg_fix_loads(scf_bb_group_t* bbg)
{
	if (0 == bbg->body->size)
		return;

	scf_basic_block_t* pre;
	scf_basic_block_t* bb;
	scf_dn_status_t*   ds;
	scf_dag_node_t*    dn;
	scf_3ac_code_t*    c;
	scf_list_t*        l;

	int i;
	int j;

	pre = bbg->pre;
	bb  = bbg->body->data[0];

	for (i = 0; i < pre->dn_colors->size; i++) {
		ds = pre->dn_colors->data[i];
		dn = ds->dag_node;

		if (ds->color <= 0)
			continue;

		if (ds->color == dn->color)
			continue;

		scf_variable_t* v = dn->var;
		scf_logw("v_%d_%d/%s, ds->color: %ld, dn->color: %ld\n",
				v->w->line, v->w->pos, v->w->text->data,
				ds->color, dn->color);

		for (l = scf_list_head(&pre->code_list_head); l != scf_list_sentinel(&pre->code_list_head); ) {
			c  = scf_list_data(l, scf_3ac_code_t, list);
			l  = scf_list_next(l);

			if (c->dst->dag_node == dn) {
				scf_list_del(&c->list);
				scf_list_add_front(&bb->code_list_head, &c->list);

				pre->code_bytes -= c->inst_bytes;
				bb ->code_bytes += c->inst_bytes;

				scf_3ac_code_t* c2;
				scf_list_t*     l2;

				for (l2 = scf_list_next(&c->list); l2 != scf_list_sentinel(&bb->code_list_head); ) {
					c2  = scf_list_data(l2, scf_3ac_code_t, list);
					l2  = scf_list_next(l2);

					c2->bb_offset += c->inst_bytes;
				}
				break;
			}
		}
	}
}

int	_scf_x64_select_inst(scf_native_t* ctx)
{
	scf_x64_context_t*	x64 = ctx->priv;
	scf_function_t*     f   = x64->f;
	scf_basic_block_t*  bb;
	scf_bb_group_t*     bbg;

	int i;
	int ret = 0;
	scf_list_t* l;

	for (l = scf_list_head(&f->basic_block_list_head); l != scf_list_sentinel(&f->basic_block_list_head);
			l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		if (bb->group_flag)
			continue;

		ret = _x64_select_bb_regs(bb, ctx);
		if (ret < 0)
			return ret;

		_x64_init_bb_colors(bb);

		if (0 == bb->index) {
			ret = _x64_argv_save(bb, f);
			if (ret < 0)
				return ret;
		}

		ret = _x64_make_insts_for_list(ctx, &bb->code_list_head, 0);
		if (ret < 0)
			return ret;
		bb->code_bytes  = ret;
	}

	for (i = 0; i < f->bb_loops->size; i++) {
		bbg = f->bb_loops->data[i];

		if (bbg->loop_layers > 1)
			continue;

		ret = _x64_select_bb_group_regs(bbg, ctx);
		if (ret < 0)
			return ret;

		_x64_init_bb_colors(bbg->pre);

		ret = _x64_make_insts_for_list(ctx, &bbg->pre->code_list_head, 0);
		if (ret < 0)
			return ret;
		bbg->pre->code_bytes  = ret;

		int j;
		for (j = 0; j < bbg->body->size; j++) {
			bb  = bbg->body->data[j];

			scf_logd("j: %d, bb: %p, bb->index: %d\n", j, bb, bb->index);

			if (bb->native_flag)
				continue;

			ret = _x64_make_insts_for_list(ctx, &bb->code_list_head, 0);
			if (ret < 0)
				return ret;
			bb->code_bytes  = ret;
			bb->native_flag = 1;

			ret = _x64_save_bb_colors(bb, bbg);
			if (ret < 0)
				return ret;
		}

		_x64_bbg_fix_loads(bbg);

		ret = _x64_bbg_fix_saves(bbg, f);
		if (ret < 0)
			return ret;
	}

	_x64_set_offset_for_jmps( ctx, f);

	return 0;
}

static int _find_local_vars(scf_node_t* node, void* arg, scf_vector_t* results)
{
	scf_block_t* b = (scf_block_t*)node;

	if (SCF_OP_BLOCK == b->node.type && b->scope) {

		int i;
		for (i = 0; i < b->scope->vars->size; i++) {

			scf_variable_t* var = b->scope->vars->data[i];

			int ret = scf_vector_add(results, var);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

int scf_x64_select_inst(scf_native_t* ctx, scf_function_t* f)
{
	scf_x64_context_t* x64 = ctx->priv;

	x64->f = f;

	scf_vector_t* local_vars = scf_vector_alloc();
	if (!local_vars)
		return -ENOMEM;

	int ret = scf_node_search_bfs((scf_node_t*)f, NULL, local_vars, -1, _find_local_vars);
	if (ret < 0)
		return ret;

	// ABI: rdi rsi rdx rcx r8 r9
	int i;
	int nb_ints   = 0;
	int nb_floats = 0;
	int nb_locals = 0;

	for (i = 0; i < f->argv->size; i++) {
		scf_variable_t* v = f->argv->data[i];

		int is_float = scf_variable_float(v);
		int size     = x64_variable_size(v);

		if (is_float) {
			if (nb_floats < X64_ABI_NB) {
				nb_floats++;

				ret = scf_vector_add(local_vars, v);
				if (ret < 0)
					return ret;
				continue;
			}
		} else if (nb_ints < X64_ABI_NB) {
			nb_ints++;

			ret = scf_vector_add(local_vars, v);
			if (ret < 0)
				return ret;
			continue;
		}

		v->bp_offset = 16 + 8 * nb_locals++;
	}

	int local_vars_size = _x64_function_init(f, local_vars);
	if (local_vars_size < 0)
		return -1;

	for (i = 0; i < local_vars->size; i++) {
		scf_variable_t* v = local_vars->data[i];
		assert(v->w);
		scf_logi("v: %p, name: %s_%d_%d, size: %d, bp_offset: %d, arg_flag: %d\n",
				v, v->w->text->data, v->w->line, v->w->pos,
				scf_variable_size(v), v->bp_offset, v->arg_flag);
	}
	scf_logi("local_vars_size: %d\n", local_vars_size);

	f->local_vars_size = local_vars_size;

	ret = _scf_x64_select_inst(ctx);
	if (ret < 0)
		return ret;

	ret = _x64_function_finish(f);
	if (ret < 0)
		return ret;

	_x64_set_offset_for_relas(ctx, f, f->text_relas);
	_x64_set_offset_for_relas(ctx, f, f->data_relas);
	return 0;
}

int scf_x64_write_elf(scf_native_t* ctx, const char* path)
{
	scf_loge("\n");
	return -1;
}

scf_native_ops_t	native_ops_x64 = {
	.name				= "x64",

	.open				= scf_x64_open,
	.close				= scf_x64_close,

	.select_inst        = scf_x64_select_inst,
	.write_elf			= scf_x64_write_elf,
};

