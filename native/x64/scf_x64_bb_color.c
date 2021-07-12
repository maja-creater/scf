#include"scf_x64.h"
#include"scf_elf.h"
#include"scf_basic_block.h"
#include"scf_3ac.h"

void x64_init_bb_colors(scf_basic_block_t* bb)
{
	scf_dag_node_t*   dn;
	scf_dn_status_t*  ds;

	int i;

	x64_registers_reset();

	for (i = 0; i < bb->dn_colors_entry->size; i++) {
		ds =        bb->dn_colors_entry->data[i];

		dn         = ds->dag_node;
		dn->color  = ds->color;
		dn->loaded = 0;

		if (0 == bb->index && dn->rabi)
			dn->loaded = 1;

		if (scf_vector_find(bb->dn_loads, dn)) {
			scf_variable_t* v = dn->var;
			if (v->w)
				scf_loge("v_%d_%d/%s, ", v->w->line, v->w->pos, v->w->text->data);
			else
				scf_loge("v_%#lx, ", 0xffff & (uintptr_t)v);

			printf("color: %ld, loaded: %d", dn->color, dn->loaded);

			if (dn->color > 0) {
				scf_register_x64_t* r = x64_find_register_color(dn->color);
				printf(", reg: %s", r->name);
			}
			printf("\n");
		}
	}
}

int x64_save_bb_colors(scf_vector_t* dn_colors, scf_bb_group_t* bbg, scf_basic_block_t* bb)
{
	scf_dag_node_t*   dn;
	scf_dn_status_t*  ds0;
	scf_dn_status_t*  ds1;

	int i;
	int j;

	for (i  = 0; i < bbg->pre->dn_colors_entry->size; i++) {

		ds0 = bbg->pre->dn_colors_entry->data[i];
		dn  = ds0->dag_node;

		for (j  = 0; j < dn_colors->size; j++) {
			ds1 =        dn_colors->data[j];

			if (ds1->dag_node == dn)
				break;
		}

		if (j == dn_colors->size) {
			ds1 = scf_dn_status_alloc(dn);
			if (!ds1)
				return -ENOMEM;

			int ret = scf_vector_add(dn_colors, ds1);
			if (ret < 0) {
				scf_dn_status_free(ds1);
				return ret;
			}
		}

		ds1->color  = dn->color;
		ds1->loaded = dn->loaded;
#if 0
		scf_variable_t* v = dn->var;
		if (v->w)
			scf_loge("bb: %d, v_%d_%d/%s, ", bb->index, v->w->line, v->w->pos, v->w->text->data);
		else
			scf_loge("bb: %d, v_%#lx, ", bb->index, 0xffff & (uintptr_t)v);
		printf("dn->color: %ld, dn->loaded: %d\n", dn->color, dn->loaded);
#endif
	}

	return 0;
}

intptr_t x64_bb_find_color(scf_vector_t* dn_colors, scf_dag_node_t* dn)
{
	scf_dn_status_t* ds = NULL;

	int j;
	for (j = 0; j < dn_colors->size; j++) {
		ds =        dn_colors->data[j];

		if (ds->dag_node == dn)
			break;
	}
	assert(j < dn_colors->size);

	return ds->color;
}

int x64_bb_load_dn(intptr_t color, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_function_t* f)
{
	scf_variable_t*     v = dn->var;
	scf_register_x64_t* r;
	scf_instruction_t*  inst;

	int inst_bytes;
	int ret;
	int i;

	if (!c->instructions) {
		c->instructions = scf_vector_alloc();
		if (!c->instructions)
			return -ENOMEM;
	}

	dn->loaded = 0;

	r   = x64_find_register_color(color);

	ret = x64_load_reg(r, dn, c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	scf_logd("add load: v_%d_%d/%s, color: %ld, r: %s\n",
			v->w->line, v->w->pos, v->w->text->data, color, r->name);
	return 0;
}

int x64_bb_save_dn(intptr_t color, scf_dag_node_t* dn, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_function_t* f)
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

	r   = x64_find_register_color(color);

	ret = x64_save_var2(dn, r, c, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	return 0;
}

int x64_bb_load_dn2(intptr_t color, scf_dag_node_t* dn, scf_basic_block_t* bb, scf_function_t* f)
{
	scf_3ac_code_t*    c;
	scf_list_t*        l;

	scf_variable_t* v = dn->var;
	if (v->w)
		scf_logd("bb: %d, v: %d/%s, bp_offset: -%#x\n", bb->index, v->w->line, v->w->text->data, -v->bp_offset);

	l = scf_list_tail(&bb->code_list_head);
	c = scf_list_data(l, scf_3ac_code_t, list);

	return x64_bb_load_dn(color, dn, c, bb, f);
}

int x64_bb_save_dn2(intptr_t color, scf_dag_node_t* dn, scf_basic_block_t* bb, scf_function_t* f)
{
	scf_3ac_operand_t* src;
	scf_3ac_code_t*    c;
	scf_list_t*        l;

	for (l = scf_list_head(&bb->save_list_head); l != scf_list_sentinel(&bb->save_list_head); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		assert(1 == c->srcs->size);

		src = c->srcs->data[0];

		if (src->dag_node == dn) {

			if (x64_bb_save_dn(color, dn, c, bb, f) < 0) {
				scf_loge("\n");
				return -1;
			}

			scf_list_del(&c->list);
			scf_list_add_tail(&bb->code_list_head, &c->list);
			break;
		}
	}

	return 0;
}

int x64_load_bb_colors(scf_basic_block_t* bb, scf_bb_group_t* bbg, scf_function_t* f)
{
	scf_basic_block_t* prev;
	scf_dn_status_t*   ds;
	scf_dn_status_t*   ds2;
	scf_dag_node_t*    dn;
	scf_variable_t*    v;

	int i;
	int j;
	int k;

	if (x64_save_bb_colors(bb->dn_colors_entry, bbg, bb) < 0) {
		scf_loge("\n");
		return -1;
	}

	for (i = 0; i < bb->dn_colors_entry->size; i++) {
		ds =        bb->dn_colors_entry->data[i];

		dn = ds->dag_node;

		intptr_t color = dn->color;
		int      first = 0;

		for (j = 0; j < bb->prevs->size; j++) {
			prev      = bb->prevs->data[j];

			if (prev->index > bb->index)
				continue;

			for (k = 0; k < prev->dn_colors_exit->size; k++) {
				ds2       = prev->dn_colors_exit->data[k];

				if (ds2->dag_node == dn)
					break;
			}

			assert(k < prev->dn_colors_exit->size);

			if (0 == first) {
				first      = 1;
				dn->color  = ds2->color;
				dn->loaded = ds2->loaded;
				continue;
			}

			if (dn->color != ds2->color)
				break;
		}

		if (j < bb->prevs->size) {

			for (j = 0; j < bb->prevs->size; j++) {
				prev      = bb->prevs->data[j];

				if (prev->index > bb->index)
					continue;

				for (k = 0; k < prev->dn_colors_exit->size; k++) {
					ds2       = prev->dn_colors_exit->data[k];

					if (ds2->dag_node == dn)
						break;
				}

				assert(k < prev->dn_colors_exit->size);

				if (x64_bb_save_dn2(ds2->color, dn, prev, f) < 0) {
					scf_loge("\n");
					return -1;
				}
			}

			dn->color  = -1;
			dn->loaded =  0;
		}

		if (color != dn->color && color > 0) {
			scf_register_x64_t* r = x64_find_register_color(color);

			scf_vector_del(r->dag_nodes, dn);
		}
	}
#if 0
	for (i = 0; i < bb->dn_colors_entry->size; i++) {
		ds =        bb->dn_colors_entry->data[i];

		dn = ds->dag_node;
		v  = dn->var;

		scf_logw("j: %d, bb: %d, color: %ld, loaded: %d, ", j, bb->index, dn->color, dn->loaded);
		if (v->w)
			printf("v_%d/%s", v->w->line, v->w->text->data);
		else
			printf("v_%#lx", 0xffff & (uintptr_t)v);

		if (dn->color > 0) {
			scf_register_x64_t* r = x64_find_register_color(dn->color);

			printf(", %s", r->name);
		}
		printf("\n");
	}
#endif

	return 0;
}

int x64_fix_bb_colors(scf_basic_block_t* bb, scf_bb_group_t* bbg, scf_function_t* f)
{
	scf_basic_block_t* prev;
	scf_dn_status_t*   ds;
	scf_dn_status_t*   ds2;
	scf_dag_node_t*    dn;
	scf_variable_t*    v;

	int i;
	int j;
	int k;

	for (i = 0; i < bb->dn_colors_entry->size; i++) {
		ds =        bb->dn_colors_entry->data[i];

		dn = ds->dag_node;

		for (j = 0; j < bb->prevs->size; j++) {
			prev      = bb->prevs->data[j];

			if (prev->index < bb->index)
				continue;

			assert(prev->back_flag);

			for (k = 0; k < prev->dn_colors_exit->size; k++) {
				ds2       = prev->dn_colors_exit->data[k];

				if (ds2->dag_node == dn)
					break;
			}

			assert(k < prev->dn_colors_exit->size);

			v = dn->var;
			if (v->w)
				scf_logd("bb: %d, prev: %d, v: %d/%s\n", bb->index, prev->index, v->w->line, v->w->text->data);

			if (ds->color == ds2->color)
				continue;

			if (!scf_vector_find(prev->exit_dn_actives, dn)
					&& !scf_vector_find(prev->exit_dn_aliases, dn))
				continue;

			if (ds2->color > 0) {
				if (x64_bb_save_dn2(ds2->color, dn, prev, f) < 0) {
					scf_loge("\n");
					return -1;
				}
			}

			if (ds->color > 0) {
				if (x64_bb_load_dn2(ds->color, dn, prev, f) < 0) {
					scf_loge("\n");
					return -1;
				}
			}
		}
	}

	return 0;
}

int x64_load_bb_colors2(scf_basic_block_t* bb, scf_bb_group_t* bbg, scf_function_t* f)
{
	scf_register_x64_t* r;
	scf_basic_block_t*  prev;
	scf_dn_status_t*    ds;
	scf_dn_status_t*    ds2;
	scf_dag_node_t*     dn;
	scf_variable_t*     v;

	int i;
	int j;
	int k;

	if (x64_save_bb_colors(bb->dn_colors_entry, bbg, bb) < 0) {
		scf_loge("\n");
		return -1;
	}

	for (i = 0; i < bb->dn_colors_entry->size; i++) {
		ds =        bb->dn_colors_entry->data[i];

		dn = ds->dag_node;

		intptr_t color = dn->color;
		int      first = 0;

		for (j = 0; j < bb->prevs->size; j++) {
			prev      = bb->prevs->data[j];

			if (!scf_vector_find(bbg->body, prev))
				continue;

			for (k = 0; k < prev->dn_colors_exit->size; k++) {
				ds2       = prev->dn_colors_exit->data[k];

				if (ds2->dag_node == dn)
					break;
			}

			assert(k < prev->dn_colors_exit->size);

			if (0 == first) {
				first      = 1;
				dn->color  = ds2->color;
				dn->loaded = ds2->loaded;
				continue;
			}

			if (dn->color != ds2->color)
				break;
		}

		if (j < bb->prevs->size) {

			for (j = 0; j < bb->prevs->size; j++) {
				prev      = bb->prevs->data[j];

				if (!scf_vector_find(bbg->body, prev))
					continue;

				for (k = 0; k < prev->dn_colors_exit->size; k++) {
					ds2       = prev->dn_colors_exit->data[k];

					if (ds2->dag_node == dn)
						break;
				}

				assert(k < prev->dn_colors_exit->size);

				if (x64_bb_save_dn2(ds2->color, dn, prev, f) < 0) {
					scf_loge("\n");
					return -1;
				}
			}

			dn->color  = -1;
			dn->loaded =  0;
		}

		if (color != dn->color && color > 0) {
			r = x64_find_register_color(color);

			scf_vector_del(r->dag_nodes, dn);
		}

		if (dn->color < 0)
			dn->loaded =  0;
	}

	for (i = 0; i < bb->dn_colors_entry->size; i++) {
		ds =        bb->dn_colors_entry->data[i];

		dn = ds->dag_node;
		v  = dn->var;
#if 0
		scf_logw("j: %d, bb: %d, color: %ld, loaded: %d, ", j, bb->index, dn->color, dn->loaded);
		if (v->w)
			printf("v_%d/%s", v->w->line, v->w->text->data);
		else
			printf("v_%#lx", 0xffff & (uintptr_t)v);
#endif
		if (dn->color > 0) {
			r = x64_find_register_color(dn->color);

			if (dn->loaded)
				assert(0 == scf_vector_add_unique(r->dag_nodes, dn));
			else
				scf_vector_del(r->dag_nodes, dn);
			//	printf(", %s", r->name);
		}
		//printf("\n");
	}
	return 0;
}

