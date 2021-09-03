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

static void _x64_argv_rabi(scf_function_t* f)
{
	scf_variable_t* v;

	f->args_int   = 0;
	f->args_float = 0;

	int bp_int    = -8;
	int bp_floats = -8 - (int)X64_ABI_NB * 8;
	int bp_others = 16;

	int i;
	for (i = 0; i < f->argv->size; i++) {
		v  =        f->argv->data[i];

		if (!v->arg_flag) {
			v ->arg_flag = 1;
			assert(f->inline_flag);
		}

		int is_float = scf_variable_float(v);
		int size     = x64_variable_size(v);

		if (is_float) {

			if (f->args_float < X64_ABI_NB) {

				v->rabi       = x64_find_register_type_id_bytes(is_float, x64_abi_float_regs[f->args_float], size);
				v->bp_offset  = bp_floats;
				bp_floats    -= 8;
				f->args_float++;
				continue;
			}
		} else if (f->args_int < X64_ABI_NB) {

			v->rabi       = x64_find_register_type_id_bytes(is_float, x64_abi_regs[f->args_int], size);
			v->bp_offset  = bp_int;
			bp_int       -= 8;
			f->args_int++;
			continue;
		}

		v->rabi       = NULL;
		v->bp_offset  = bp_others;
		bp_others    += 8;
	}
}

static int _x64_function_init(scf_function_t* f, scf_vector_t* local_vars)
{
	scf_variable_t* v;

	int ret = x64_registers_init();
	if (ret < 0)
		return ret;

	int i;
	for (i = 0; i < local_vars->size; i++) {
		v  =        local_vars->data[i];

		v->bp_offset = 0;
	}

	_x64_argv_rabi(f);

	int local_vars_size = 8 + X64_ABI_NB * 8 * 2;

	for (i = 0; i < local_vars->size; i++) {
		v  =        local_vars->data[i];

		if (v->arg_flag) {
			if (v->bp_offset != 0)
				continue;
		}

		int size = scf_variable_size(v);
		if (size < 0)
			return size;

		local_vars_size	+= size;

		if (local_vars_size & 0x7)
			local_vars_size = (local_vars_size + 7) >> 3 << 3;

		v->bp_offset	 = -local_vars_size;
		v->local_flag	 = 1;
	}

	return local_vars_size;
}

static int _x64_save_rabi(scf_function_t* f)
{
	scf_register_x64_t* rbp;
	scf_instruction_t*  inst;
	scf_x64_OpCode_t*   mov;

	scf_register_x64_t* rdi;
	scf_register_x64_t* rsi;
	scf_register_x64_t* rdx;
	scf_register_x64_t* rcx;
	scf_register_x64_t* r8;
	scf_register_x64_t* r9;

	scf_register_x64_t* xmm0;
	scf_register_x64_t* xmm1;
	scf_register_x64_t* xmm2;
	scf_register_x64_t* xmm3;

	if (f->vargs_flag) {

		inst = NULL;
		mov  = x64_find_OpCode(SCF_X64_MOV, 8,8, SCF_X64_G2E);

		rbp  = x64_find_register("rbp");

		rdi  = x64_find_register("rdi");
		rsi  = x64_find_register("rsi");
		rdx  = x64_find_register("rdx");
		rcx  = x64_find_register("rcx");
		r8   = x64_find_register("r8");
		r9   = x64_find_register("r9");

#define X64_SAVE_RABI(offset, rabi) \
		do { \
			inst = x64_make_inst_G2P(mov, rbp, offset, rabi); \
			X64_INST_ADD_CHECK(f->init_insts, inst); \
			f->init_code_bytes += inst->len; \
		} while (0)

		X64_SAVE_RABI(-8,  rdi);
		X64_SAVE_RABI(-16, rsi);
		X64_SAVE_RABI(-24, rdx);
		X64_SAVE_RABI(-32, rcx);
		X64_SAVE_RABI(-40, r8);
		X64_SAVE_RABI(-48, r9);

		mov  = x64_find_OpCode(SCF_X64_MOVSD, 8,8, SCF_X64_G2E);

		xmm0 = x64_find_register("xmm0");
		xmm1 = x64_find_register("xmm1");
		xmm2 = x64_find_register("xmm2");
		xmm3 = x64_find_register("xmm3");

		X64_SAVE_RABI(-56, xmm0);
		X64_SAVE_RABI(-64, xmm1);
		X64_SAVE_RABI(-72, xmm2);
		X64_SAVE_RABI(-80, xmm3);
	}

	return 0;
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
	scf_x64_OpCode_t*   sub  = x64_find_OpCode(SCF_X64_SUB,  4,8, SCF_X64_I2E);

	scf_register_x64_t* rsp  = x64_find_register("rsp");
	scf_register_x64_t* rbp  = x64_find_register("rbp");
	scf_register_x64_t* r;
	scf_instruction_t*  inst = NULL;

	if (f->bp_used_flag) {

		inst = x64_make_inst_G(push, rbp);
		X64_INST_ADD_CHECK(f->init_insts, inst);
		f->init_code_bytes  = inst->len;

		inst = x64_make_inst_G2E(mov, rbp, rsp);
		X64_INST_ADD_CHECK(f->init_insts, inst);
		f->init_code_bytes += inst->len;

		uint32_t local = f->local_vars_size;
		if (!(local & 0xf))
			local += 8;

		inst = x64_make_inst_I2E(sub, rsp, (uint8_t*)&local, 4);
		//inst = x64_make_inst_I2E(sub, rsp, (uint8_t*)&f->local_vars_size, 4);
		X64_INST_ADD_CHECK(f->init_insts, inst);
		f->init_code_bytes += inst->len;

		int ret = _x64_save_rabi(f);
		if (ret < 0)
			return ret;
	} else
		f->init_code_bytes = 0;

	int i;
	for (i = 0; i < X64_ABI_CALLEE_SAVES_NB; i++) {

		r  = x64_find_register_type_id_bytes(0, x64_abi_callee_saves[i], 8);

		inst = x64_make_inst_G(push, r);
		X64_INST_ADD_CHECK(f->init_insts, inst);

		f->init_code_bytes += inst->len;
	}

	x64_registers_clear();
	return 0;
}

static void _x64_rcg_node_printf(x64_rcg_node_t* rn)
{
	if (rn->dag_node) {
		scf_variable_t* v = rn->dag_node->var;

		if (v->w) {
			scf_logw("v_%d_%d/%s, %p,%p, ",
					v->w->line, v->w->pos, v->w->text->data,
					v, rn->dag_node);

			if (v->bp_offset < 0)
				printf("bp_offset: -%#x, ", -v->bp_offset);
			else
				printf("bp_offset:  %#x, ",  v->bp_offset);

			printf("color: %ld, type: %ld, id: %ld, mask: %ld",
					rn->dag_node->color,
					X64_COLOR_TYPE(rn->dag_node->color),
					X64_COLOR_ID(rn->dag_node->color),
					X64_COLOR_MASK(rn->dag_node->color));

		} else {
			scf_logw("v_%#lx, %p,%p,  color: %ld, type: %ld, id: %ld, mask: %ld",
					(uintptr_t)v & 0xffff, v, rn->dag_node,
					rn->dag_node->color,
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
	scf_graph_node_t* gn;
	scf_dag_node_t*   dn;
	scf_variable_t*   v;
	scf_list_t*       l;

	int i;
	for (i = 0; i < f->argv->size; i++) {
		v  =        f->argv->data[i];

		assert(v->arg_flag);

		for (l = scf_list_head(&f->dag_list_head); l != scf_list_sentinel(&f->dag_list_head);
				l = scf_list_next(l)) {

			dn = scf_list_data(l, scf_dag_node_t, list);
			if (dn->var == v)
				break;
		}

		// if arg is not found in dag, it's not used in function, ignore.
		if (l == scf_list_sentinel(&f->dag_list_head))
			continue;

		int ret = _x64_rcg_make_node(&gn, g, dn, v->rabi, NULL);
		if (ret < 0)
			return ret;

		dn->rabi = v->rabi;

		if (dn->rabi)
			dn->loaded =  1;
		else
			dn->color  = -1;
	}

	return 0;
}

static int _x64_argv_save(scf_basic_block_t* bb, scf_function_t* f)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;

	if (!scf_list_empty(&bb->code_list_head)) {

		l = scf_list_head(&bb->code_list_head);
		c = scf_list_data(l, scf_3ac_code_t, list);
	} else {
		c = scf_3ac_code_alloc();
		if (!c)
			return -ENOMEM;

		c->op = scf_3ac_find_operator(SCF_OP_3AC_NOP);
		c->basic_block = bb;

		scf_list_add_tail(&bb->code_list_head, &c->list);
	}

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	int i;
	for (i = 0; i < f->argv->size; i++) {
		scf_variable_t*     v = f->argv->data[i];

		assert(v->arg_flag);

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

		for (j = 0; j < bb->dn_colors_entry->size; j++) {
			active    = bb->dn_colors_entry->data[j];
			dn2       = active->dag_node;

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
		scf_dn_status_t*    ds;

		if (!dn) {
			_x64_rcg_node_printf(rn);
			continue;
		}

		ds = scf_dn_status_alloc(dn);
		if (!ds)
			return -ENOMEM;

		int ret = scf_vector_add(bb->dn_colors_entry, ds);
		if (ret < 0) {
			scf_dn_status_free(ds);
			return ret;
		}
		ds->color = gn->color;
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

	int ret = 0;
	int i;

	if (0 == bb->index) {
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

	ret = scf_x64_graph_kcolor(g, 16, colors);
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
	scf_x64_context_t*	x64 = ctx->priv;
	scf_function_t*     f   = x64->f;

	scf_graph_t* g = scf_graph_alloc();
	if (!g)
		return -ENOMEM;

	scf_vector_t*      colors = NULL;
	scf_basic_block_t* bb;

	int ret = 0;
	int i;

	if (0 == bbg->pre->index) {
		ret = _x64_argv_prepare(g, bb, f);
		if (ret < 0)
			goto error;
	}

	for (i = 0; i < bbg->body->size; i++) {
		bb =        bbg->body->data[i];

		ret = _x64_make_bb_rcg(g, bb, ctx);
		if (ret < 0)
			goto error;
	}

	colors = x64_register_colors();
	if (!colors) {
		ret = -ENOMEM;
		goto error;
	}

	ret = scf_x64_graph_kcolor(g, 16, colors);
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

			scf_3ac_operand_t* dst    = c->dsts->data[0];
			scf_basic_block_t* cur_bb = c->basic_block;
			scf_basic_block_t* dst_bb = dst->bb;

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
		scf_instruction_t* inst   = rela->inst;
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
		for (j = 0; j < c->instructions->size; j++) {
			inst2     = c->instructions->data[j];

			if (inst2 == inst)
				break;

			bytes += inst2->len;
		}

		rela->inst_offset += bytes;
	}
}

static int _x64_bbg_fix_saves(scf_bb_group_t* bbg, scf_function_t* f)
{
	scf_basic_block_t* pre;
	scf_basic_block_t* post;
	scf_basic_block_t* bb;
	scf_3ac_operand_t* src;
	scf_dag_node_t*    dn;
	scf_dag_node_t*    dn2;
	scf_3ac_code_t*    c;
	scf_3ac_code_t*    c2;
	scf_list_t*        l;
	scf_list_t*        l2;

	int i;
//	int j;

	pre  = bbg->pre;
	post = bbg->posts->data[0];

	for (l = scf_list_head(&post->save_list_head); l != scf_list_sentinel(&post->save_list_head); ) {
		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		assert(c->srcs && 1 == c->srcs->size);
		src = c->srcs->data[0];
		dn  = src->dag_node;

		scf_variable_t* v = dn->var;

		intptr_t color   = x64_bb_find_color(pre->dn_colors_exit, dn);
		int      updated = 0;

		for (i = 0; i < bbg->body->size; i++) {
			bb =        bbg->body->data[i];

			intptr_t color2 = x64_bb_find_color(bb->dn_colors_exit, dn);

			if (color2 != color)
				updated++;
		}

		if (0 == updated) {
			if (color <= 0)
				continue;

			int ret = x64_bb_save_dn(color, dn, c, post, f);
			if (ret < 0)
				return ret;

			scf_list_del(&c->list);
			scf_list_add_tail(&post->code_list_head, &c->list);

			for (i = 1; i < bbg->posts->size; i++) {
				bb =        bbg->posts->data[i];

				l2 = scf_list_head(&bb->save_list_head);
				c2  = scf_list_data(l2, scf_3ac_code_t, list);

				assert(c2->srcs && 1 == c2->srcs->size);
				src = c2->srcs->data[0];
				dn2 = src->dag_node;

				ret = x64_bb_save_dn(color, dn2, c2, bb, f);
				if (ret < 0)
					return ret;

				scf_list_del(&c2->list);
				scf_list_add_tail(&bb->code_list_head, &c2->list);
			}
		} else {
			scf_list_del(&c->list);
			scf_3ac_code_free(c);
			c = NULL;

			for (i = 1; i < bbg->posts->size; i++) {
				bb =        bbg->posts->data[i];

				l2 = scf_list_head(&bb->save_list_head);
				c2  = scf_list_data(l2, scf_3ac_code_t, list);

				scf_list_del(&c2->list);

				scf_3ac_code_free(c2);
				c2 = NULL;
			}
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

				intptr_t color = x64_bb_find_color(bb->dn_colors_exit, dn);
				if (color <= 0)
					continue;

				int ret = x64_bb_save_dn(color, dn, c, bb, f);
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
	scf_3ac_operand_t* dst;
	scf_dn_status_t*   ds;
	scf_dag_node_t*    dn;
	scf_3ac_code_t*    c;
	scf_list_t*        l;

	int i;
	int j;

	pre = bbg->pre;
	bb  = bbg->body->data[0];

	for (i = 0; i < pre->dn_colors_exit->size; i++) {
		ds =        pre->dn_colors_exit->data[i];

		dn = ds->dag_node;

		if (ds->color <= 0)
			continue;

		if (ds->color == dn->color)
			continue;
#if 1
		scf_variable_t* v = dn->var;
		if (v->w)
			scf_logw("v_%d_%d/%s, ", v->w->line, v->w->pos, v->w->text->data);
		else
			scf_logw("v_%#lx, ", 0xffff & (uintptr_t)v);
		printf("ds->color: %ld, dn->color: %ld\n", ds->color, dn->color);
#endif
		for (l = scf_list_head(&pre->code_list_head); l != scf_list_sentinel(&pre->code_list_head); ) {
			c  = scf_list_data(l, scf_3ac_code_t, list);
			l  = scf_list_next(l);

			dst = c->dsts->data[0];

			if (dst->dag_node == dn) {
				scf_list_del(&c->list);
				scf_list_add_front(&bb->code_list_head, &c->list);
				c->basic_block = bb;
				break;
			}
		}
	}
}

static void _x64_set_offsets(scf_function_t* f)
{
	scf_instruction_t* inst;
	scf_basic_block_t* bb;
	scf_3ac_code_t*    c;
	scf_list_t*        l;
	scf_list_t*        l2;

	int i;

	for (l = scf_list_head(&f->basic_block_list_head); l != scf_list_sentinel(&f->basic_block_list_head);
			l = scf_list_next(l)) {

		bb = scf_list_data(l, scf_basic_block_t, list);

		bb->code_bytes = 0;

		for (l2 = scf_list_head(&bb->code_list_head); l2 != scf_list_sentinel(&bb->code_list_head);
				l2 = scf_list_next(l2)) {

			c = scf_list_data(l2, scf_3ac_code_t, list);

			c->inst_bytes = 0;
			c->bb_offset  = bb->code_bytes;

			if (!c->instructions)
				continue;

			for (i = 0; i < c->instructions->size; i++) {
				inst      = c->instructions->data[i];

				c->inst_bytes += inst->len;
			}

			bb->code_bytes += c->inst_bytes;
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

		if (bb->group_flag || bb->loop_flag)
			continue;

		ret = _x64_select_bb_regs(bb, ctx);
		if (ret < 0)
			return ret;

		x64_init_bb_colors(bb);

		if (0 == bb->index) {
			ret = _x64_argv_save(bb, f);
			if (ret < 0)
				return ret;
		}

		ret = _x64_make_insts_for_list(ctx, &bb->code_list_head, 0);
		if (ret < 0)
			return ret;
	}

	for (i  = 0; i < f->bb_groups->size; i++) {
		bbg =        f->bb_groups->data[i];

		ret = _x64_select_bb_group_regs(bbg, ctx);
		if (ret < 0)
			return ret;

		x64_init_bb_colors(bbg->pre);

		if (0 == bbg->pre->index) {
			ret = _x64_argv_save(bbg->pre, f);
			if (ret < 0)
				return ret;
		}

		int j;
		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			assert(!bb->native_flag);

			if (0 != j) {
				ret = x64_load_bb_colors2(bb, bbg, f);
				if (ret < 0)
					return ret;
			}

			scf_loge("************ bb: %d\n", bb->index);
			ret = _x64_make_insts_for_list(ctx, &bb->code_list_head, 0);
			if (ret < 0)
				return ret;
			bb->native_flag = 1;
			scf_loge("************ bb: %d\n", bb->index);

			ret = x64_save_bb_colors(bb->dn_colors_exit, bbg, bb);
			if (ret < 0)
				return ret;
		}
	}

	for (i  = 0; i < f->bb_loops->size; i++) {
		bbg =        f->bb_loops->data[i];

		ret = _x64_select_bb_group_regs(bbg, ctx);
		if (ret < 0)
			return ret;

		x64_init_bb_colors(bbg->pre);

		if (0 == bbg->pre->index) {
			ret = _x64_argv_save(bbg->pre, f);
			if (ret < 0)
				return ret;
		}

		ret = _x64_make_insts_for_list(ctx, &bbg->pre->code_list_head, 0);
		if (ret < 0)
			return ret;

		ret = x64_save_bb_colors(bbg->pre->dn_colors_exit, bbg, bbg->pre);
		if (ret < 0)
			return ret;

		int j;
		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			assert(!bb->native_flag);

			ret = x64_load_bb_colors(bb, bbg, f);
			if (ret < 0)
				return ret;

			ret = _x64_make_insts_for_list(ctx, &bb->code_list_head, 0);
			if (ret < 0)
				return ret;
			bb->native_flag = 1;

			ret = x64_save_bb_colors(bb->dn_colors_exit, bbg, bb);
			if (ret < 0)
				return ret;
		}

		_x64_bbg_fix_loads(bbg);

		ret = _x64_bbg_fix_saves(bbg, f);
		if (ret < 0)
			return ret;

		for (j = 0; j < bbg->body->size; j++) {
			bb =        bbg->body->data[j];

			ret = x64_fix_bb_colors(bb, bbg, f);
			if (ret < 0)
				return ret;
		}
	}
#if 0
	if (x64_optimize_peephole(ctx, f) < 0) {
		scf_loge("\n");
		return -1;
	}
#endif
	_x64_set_offsets(f);

	_x64_set_offset_for_jmps( ctx, f);

	return 0;
}

static int _find_local_vars(scf_node_t* node, void* arg, scf_vector_t* results)
{
	scf_block_t* b = (scf_block_t*)node;

	if ((SCF_OP_BLOCK == b->node.type || SCF_FUNCTION == b->node.type) && b->scope) {

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

	int local_vars_size = _x64_function_init(f, local_vars);
	if (local_vars_size < 0)
		return -1;

	int i;
	for (i = 0; i < local_vars->size; i++) {
		scf_variable_t* v = local_vars->data[i];
		assert(v->w);
		scf_logi("v: %p, name: %s_%d_%d, size: %d, bp_offset: %d, arg_flag: %d\n",
				v, v->w->text->data, v->w->line, v->w->pos,
				scf_variable_size(v), v->bp_offset, v->arg_flag);
	}

	f->local_vars_size = local_vars_size;
	f->bp_used_flag    = 1;

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

scf_native_ops_t	native_ops_x64 = {
	.name            = "x64",

	.open            = scf_x64_open,
	.close           = scf_x64_close,

	.select_inst     = scf_x64_select_inst,
};

