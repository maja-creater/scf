#include"scf_optimizer.h"
#include"scf_pointer_alias.h"

static scf_3ac_operand_t* _auto_gc_operand_alloc_pf(scf_ast_t* ast, scf_function_t* f)
{
	scf_3ac_operand_t* src;
	scf_dag_node_t*    dn;
	scf_variable_t*    v;
	scf_type_t*        t;

	t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, SCF_FUNCTION_PTR);
	assert(0 == ret);
	assert(t);

	if (f)
		v = SCF_VAR_ALLOC_BY_TYPE(f->node.w, t, 1, 1, f);
	else
		v = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 1, 1, NULL);

	if (!v)
		return NULL;
	v->const_literal_flag = 1;

	dn = scf_dag_node_alloc(v->type, v, (scf_node_t*)f);
	if (!dn) {
		scf_variable_free(v);
		return NULL;
	}
	scf_variable_free(v);

	src = scf_3ac_operand_alloc();
	if (!src) {
		scf_dag_node_free(dn);
		return NULL;
	}

	src->node     = (scf_node_t*)f;
	src->dag_node = dn;
	return src;
}

static scf_3ac_code_t* _auto_gc_code_ref(scf_ast_t* ast, scf_dag_node_t* dn)
{
	scf_3ac_operand_t* src;
	scf_3ac_code_t*    c;
	scf_function_t*    f;
	scf_variable_t*    v;
	scf_type_t*        t;

	c = scf_3ac_code_alloc();
	if (!c)
		return NULL;
	c->op   = scf_3ac_find_operator(SCF_OP_CALL);

	c->srcs = scf_vector_alloc();
	if (!c->srcs) {
		scf_3ac_code_free(c);
		return NULL;
	}

	f = NULL;
	int ret = scf_ast_find_function(&f, ast, "scf__auto_ref");
	assert(0 == ret);
	assert(f);

#define AUTO_GC_CODE_ADD_FUNCPTR() \
	do { \
		src = _auto_gc_operand_alloc_pf(ast, f); \
		if (!src) { \
			scf_3ac_code_free(c); \
			return NULL; \
		} \
		\
		if (scf_vector_add(c->srcs, src) < 0) { \
			scf_3ac_operand_free(src); \
			scf_3ac_code_free(c); \
			return NULL; \
		} \
	} while (0)

	AUTO_GC_CODE_ADD_FUNCPTR();

#define AUTO_GC_CODE_ADD_VAR() \
	do { \
		src = scf_3ac_operand_alloc(); \
		if (!src) { \
			scf_3ac_code_free(c); \
			return NULL; \
		} \
		src->node     = dn->node; \
		src->dag_node = dn; \
		\
		if (scf_vector_add(c->srcs, src) < 0) { \
			scf_3ac_operand_free(src); \
			scf_3ac_code_free(c); \
			return NULL; \
		} \
	} while (0)

	AUTO_GC_CODE_ADD_VAR();

	return c;
}

static scf_3ac_code_t* _auto_gc_code_memset_array(scf_ast_t* ast, scf_dag_node_t* dn_array)
{
	scf_3ac_operand_t* src;
	scf_3ac_code_t*    c;
	scf_dag_node_t*    dn;
	scf_function_t*    f;
	scf_variable_t*    v;
	scf_type_t*        t;

	c = scf_3ac_code_alloc();
	if (!c)
		return NULL;
	c->op   = scf_3ac_find_operator(SCF_OP_3AC_MEMSET);

	c->srcs = scf_vector_alloc();
	if (!c->srcs) {
		scf_3ac_code_free(c);
		return NULL;
	}

	dn = dn_array;
	AUTO_GC_CODE_ADD_VAR();

	t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, SCF_VAR_INTPTR);
	assert(0 == ret);
	assert(t);

	v = SCF_VAR_ALLOC_BY_TYPE(dn_array->var->w, t, 1, 0, NULL);
	assert(v);
	v->data.i64 = 0;

	dn = scf_dag_node_alloc(v->type, v, NULL);
	assert(dn);
	AUTO_GC_CODE_ADD_VAR();


	v   = SCF_VAR_ALLOC_BY_TYPE(dn_array->var->w, t, 1, 0, NULL);
	assert(v);
	v->data.i64 = scf_variable_size(dn_array->var);

	dn = scf_dag_node_alloc(v->type, v, NULL);
	assert(dn);
	AUTO_GC_CODE_ADD_VAR();

	return c;
}

static scf_3ac_code_t* _auto_gc_code_free_array(scf_ast_t* ast, scf_dag_node_t* dn_array, int capacity, int nb_pointers)
{
	scf_3ac_operand_t* src;
	scf_3ac_code_t*    c;
	scf_dag_node_t*    dn;
	scf_function_t*    f;
	scf_variable_t*    v;
	scf_type_t*        t;

	c = scf_3ac_code_alloc();
	if (!c)
		return NULL;
	c->op   = scf_3ac_find_operator(SCF_OP_CALL);

	c->srcs = scf_vector_alloc();
	if (!c->srcs) {
		scf_3ac_code_free(c);
		return NULL;
	}

	f = NULL;
	int ret = scf_ast_find_function(&f, ast, "scf__auto_free_array");
	assert(0 == ret);
	assert(f);
	AUTO_GC_CODE_ADD_FUNCPTR();

	dn = dn_array;
	AUTO_GC_CODE_ADD_VAR();

	t   = scf_block_find_type_type(ast->current_block, SCF_VAR_INTPTR);
	v   = SCF_VAR_ALLOC_BY_TYPE(dn_array->var->w, t, 1, 0, NULL);
	assert(v);
	v->data.i64 = capacity;

	dn = scf_dag_node_alloc(v->type, v, NULL);
	assert(dn);
	AUTO_GC_CODE_ADD_VAR();


	v   = SCF_VAR_ALLOC_BY_TYPE(dn_array->var->w, t, 1, 0, NULL);
	assert(v);
	v->data.i64 = nb_pointers;

	dn = scf_dag_node_alloc(v->type, v, NULL);
	assert(dn);
	AUTO_GC_CODE_ADD_VAR();


	if (dn_array->var->type >= SCF_STRUCT) {

		t   = NULL;
		ret = scf_ast_find_type_type(&t, ast, dn_array->var->type);
		assert(0 == ret);
		assert(t);

		f   = scf_scope_find_function(t->scope, "__release");
	} else
		f = NULL;
	AUTO_GC_CODE_ADD_FUNCPTR();

	return c;
}

static scf_3ac_code_t* _auto_gc_code_freep_array(scf_ast_t* ast, scf_dag_node_t* dn_array, int nb_pointers)
{
	scf_3ac_operand_t* src;
	scf_3ac_code_t*    c;
	scf_dag_node_t*    dn;
	scf_function_t*    f;
	scf_variable_t*    v;
	scf_type_t*        t;

	c = scf_3ac_code_alloc();
	if (!c)
		return NULL;
	c->op   = scf_3ac_find_operator(SCF_OP_CALL);

	c->srcs = scf_vector_alloc();
	if (!c->srcs) {
		scf_3ac_code_free(c);
		return NULL;
	}

	f = NULL;
	int ret = scf_ast_find_function(&f, ast, "scf__auto_freep_array");
	assert(0 == ret);
	assert(f);
	AUTO_GC_CODE_ADD_FUNCPTR();

	dn = dn_array;
	AUTO_GC_CODE_ADD_VAR();

	t   = NULL;
	ret = scf_ast_find_type_type(&t, ast, SCF_VAR_INTPTR);
	assert(0 == ret);
	assert(t);
	v = SCF_VAR_ALLOC_BY_TYPE(dn_array->var->w, t, 1, 0, NULL);
	assert(v);
	v->data.i64 = nb_pointers;

	char buf[128];
	snprintf(buf, sizeof(buf) - 1, "%d", nb_pointers);
	scf_string_cat_cstr(v->w->text, buf);

	dn = scf_dag_node_alloc(v->type, v, NULL);
	assert(dn);
	AUTO_GC_CODE_ADD_VAR();

	if (dn_array->var->type >= SCF_STRUCT) {

		t = NULL;
		int ret = scf_ast_find_type_type(&t, ast, dn_array->var->type);
		assert(0 == ret);
		assert(t);

		f = scf_scope_find_function(t->scope, "__release");

		scf_logw("f: %p, t->name: %p\n", f, t->name->data);
	} else
		f = NULL;
	AUTO_GC_CODE_ADD_FUNCPTR();

	return c;
}

static scf_3ac_code_t* _code_alloc_address(scf_ast_t* ast, scf_dag_node_t* dn)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();
	scf_vector_t*   dsts = scf_vector_alloc();

	scf_3ac_operand_t*   src0  = scf_3ac_operand_alloc();
	scf_vector_add(srcs, src0);

	src0->node     = dn->node;
	src0->dag_node = dn;

	scf_3ac_operand_t*   dst0 = scf_3ac_operand_alloc();
	scf_vector_add(dsts, dst0);

	c->srcs = srcs;
	c->dsts = dsts;
	c->op   = scf_3ac_find_operator(SCF_OP_ADDRESS_OF);

	w = scf_lex_word_alloc(dn->var->w->file, 0, 0, SCF_LEX_WORD_ID);
	w->text = scf_string_cstr("&");

	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, dn->var->type);
	assert(0 == ret);
	assert(t);

	scf_variable_t* v   = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, dn->var->nb_pointers + 1, NULL);
	scf_dag_node_t* dn2 = scf_dag_node_alloc(v->type, v, NULL);

	dst0->dag_node = dn2;

	return c;
}

static scf_3ac_code_t* _code_alloc_dereference(scf_ast_t* ast, scf_dag_node_t* dn)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();
	scf_vector_t*   dsts = scf_vector_alloc();

	scf_3ac_operand_t*   src0  = scf_3ac_operand_alloc();
	scf_vector_add(srcs, src0);

	src0->node     = dn->node;
	src0->dag_node = dn;

	scf_3ac_operand_t*   dst0 = scf_3ac_operand_alloc();
	scf_vector_add(dsts, dst0);

	c->srcs = srcs;
	c->dsts = dsts;
	c->op   = scf_3ac_find_operator(SCF_OP_DEREFERENCE);

	w = scf_lex_word_alloc(dn->var->w->file, 0, 0, SCF_LEX_WORD_ID);
	w->text = scf_string_cstr("*");

	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, dn->var->type);
	assert(0 == ret);
	assert(t);

	scf_variable_t* v   = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, dn->var->nb_pointers - 1, NULL);
	scf_dag_node_t* dn2 = scf_dag_node_alloc(v->type, v, NULL);

	dst0->dag_node = dn2;

	return c;
}

static scf_3ac_code_t* _code_alloc_member(scf_ast_t* ast, scf_dag_node_t* dn_base, scf_dag_node_t* dn_member)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();
	scf_vector_t*   dsts = scf_vector_alloc();

	scf_3ac_operand_t*   base   = scf_3ac_operand_alloc();
	scf_3ac_operand_t*   member = scf_3ac_operand_alloc();
	scf_vector_add(srcs, base);
	scf_vector_add(srcs, member);

	base->node       = dn_base->node;
	base->dag_node   = dn_base;

	member->node     = dn_member->node;
	member->dag_node = dn_member;

	scf_3ac_operand_t*   dst0 = scf_3ac_operand_alloc();
	scf_vector_add(dsts, dst0);

	c->srcs = srcs;
	c->dsts = dsts;
	c->op   = scf_3ac_find_operator(SCF_OP_POINTER);

	w = scf_lex_word_alloc(dn_base->var->w->file, 0, 0, SCF_LEX_WORD_ID);
	w->text = scf_string_cstr("->");

	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, dn_member->var->type);
	assert(0 == ret);
	assert(t);

	scf_variable_t* v   = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, dn_member->var->nb_pointers, NULL);
	scf_dag_node_t* dn2 = scf_dag_node_alloc(v->type, v, NULL);

	dst0->dag_node = dn2;

	return c;
}

static scf_3ac_code_t* _code_alloc_member_address(scf_ast_t* ast, scf_dag_node_t* dn_base, scf_dag_node_t* dn_member)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();
	scf_vector_t*   dsts = scf_vector_alloc();

	scf_3ac_operand_t*   base   = scf_3ac_operand_alloc();
	scf_3ac_operand_t*   member = scf_3ac_operand_alloc();
	scf_vector_add(srcs, base);
	scf_vector_add(srcs, member);

	base->node       = dn_base->node;
	base->dag_node   = dn_base;

	member->node     = dn_member->node;
	member->dag_node = dn_member;

	scf_3ac_operand_t*   dst0 = scf_3ac_operand_alloc();
	scf_vector_add(dsts, dst0);

	c->srcs = srcs;
	c->dsts = dsts;
	c->op   = scf_3ac_find_operator(SCF_OP_3AC_ADDRESS_OF_POINTER);

	w = scf_lex_word_alloc(dn_base->var->w->file, 0, 0, SCF_LEX_WORD_ID);
	w->text = scf_string_cstr("&->");

	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, dn_member->var->type);
	assert(0 == ret);
	assert(t);

	scf_variable_t* v   = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, dn_member->var->nb_pointers + 1, NULL);
	scf_dag_node_t* dn2 = scf_dag_node_alloc(v->type, v, NULL);

	dn2->var->nb_dimentions = dn_base->var->nb_dimentions;

	dst0->dag_node = dn2;

	return c;
}

static scf_3ac_code_t* _code_alloc_array_member_address(scf_ast_t* ast, scf_dag_node_t* dn_base, scf_dag_node_t* dn_index, scf_dag_node_t* dn_scale)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();
	scf_vector_t*   dsts = scf_vector_alloc();

	scf_3ac_operand_t*   base   = scf_3ac_operand_alloc();
	scf_3ac_operand_t*   index  = scf_3ac_operand_alloc();
	scf_3ac_operand_t*   scale  = scf_3ac_operand_alloc();
	scf_vector_add(srcs, base);
	scf_vector_add(srcs, index);
	scf_vector_add(srcs, scale);

	base->node       = dn_base->node;
	base->dag_node   = dn_base;

	index->node      = dn_index->node;
	index->dag_node  = dn_index;

	scale->node      = dn_scale->node;
	scale->dag_node  = dn_scale;

	scf_3ac_operand_t*   dst0 = scf_3ac_operand_alloc();
	scf_vector_add(dsts, dst0);

	c->srcs = srcs;
	c->dsts = dsts;
	c->op   = scf_3ac_find_operator(SCF_OP_3AC_ADDRESS_OF_ARRAY_INDEX);

	w = scf_lex_word_alloc(dn_base->var->w->file, 0, 0, SCF_LEX_WORD_ID);
	w->text = scf_string_cstr("&[]");

	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, dn_base->var->type);
	assert(0 == ret);
	assert(t);

	scf_variable_t* v   = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, dn_base->var->nb_pointers, NULL);
	scf_dag_node_t* dn2 = scf_dag_node_alloc(v->type, v, NULL);

	dn2->var->nb_dimentions = dn_base->var->nb_dimentions;

	dst0->dag_node = dn2;

	return c;
}

static scf_3ac_code_t* _code_alloc_array_member(scf_ast_t* ast, scf_dag_node_t* dn_base, scf_dag_node_t* dn_index, scf_dag_node_t* dn_scale)
{
	scf_string_t*   s;
	scf_lex_word_t* w;
	scf_3ac_code_t* c    = scf_3ac_code_alloc();
	scf_vector_t*   srcs = scf_vector_alloc();
	scf_vector_t*   dsts = scf_vector_alloc();

	scf_3ac_operand_t*   base   = scf_3ac_operand_alloc();
	scf_3ac_operand_t*   index  = scf_3ac_operand_alloc();
	scf_3ac_operand_t*   scale  = scf_3ac_operand_alloc();
	scf_vector_add(srcs, base);
	scf_vector_add(srcs, index);
	scf_vector_add(srcs, scale);

	base->node       = dn_base->node;
	base->dag_node   = dn_base;

	index->node      = dn_index->node;
	index->dag_node  = dn_index;

	scale->node      = dn_scale->node;
	scale->dag_node  = dn_scale;

	scf_3ac_operand_t*   dst0 = scf_3ac_operand_alloc();
	scf_vector_add(dsts, dst0);

	c->srcs = srcs;
	c->dsts = dsts;
	c->op   = scf_3ac_find_operator(SCF_OP_ARRAY_INDEX);

	w = scf_lex_word_alloc(dn_base->var->w->file, 0, 0, SCF_LEX_WORD_ID);
	w->text = scf_string_cstr("[]");

	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, dn_base->var->type);
	assert(0 == ret);
	assert(t);

	scf_variable_t* v   = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, dn_base->var->nb_pointers, NULL);
	scf_dag_node_t* dn2 = scf_dag_node_alloc(v->type, v, NULL);

	dn2->var->nb_dimentions = dn_base->var->nb_dimentions;

	dst0->dag_node = dn2;

	return c;
}

static int _auto_gc_code_list_ref(scf_list_t* h, scf_ast_t* ast, scf_dn_status_t* ds)
{
	scf_dag_node_t*    dn = ds->dag_node;
	scf_3ac_code_t*    c;
	scf_3ac_operand_t* dst;

	if (ds->dn_indexes) {

		int i;
		for (i = ds->dn_indexes->size - 1; i >= 0; i--) {

			scf_dn_index_t* di = ds->dn_indexes->data[i];

			if (di->member) {
				assert(di->dn);

				c = _code_alloc_member(ast, dn, di->dn);

			} else {
				assert(di->index >= 0 || -1 == di->index);
				assert(di->dn_scale);

				c = _code_alloc_array_member(ast, dn, di->dn, di->dn_scale);
			}

			scf_list_add_tail(h, &c->list);

			dst = c->dsts->data[0];
			dn  = dst->dag_node;
		}
	}

	c = _auto_gc_code_ref(ast, dn);

	scf_list_add_tail(h, &c->list);
	return 0;
}

static int _auto_gc_code_list_freep(scf_list_t* h, scf_ast_t* ast, scf_dn_status_t* ds)
{
	scf_dag_node_t*    dn = ds->dag_node;
	scf_3ac_code_t*    c;
	scf_3ac_operand_t* dst;

	if (ds->dn_indexes) {

		scf_dn_index_t* di;
		int i;

		for (i = ds->dn_indexes->size - 1; i >= 1; i--) {
			di = ds->dn_indexes->data[i];

			if (di->member) {
				assert(di->dn);

				c = _code_alloc_member(ast, dn, di->dn);

			} else {
				assert(di->index >= 0);

				assert(0 == di->index);

				c = _code_alloc_dereference(ast, dn);
			}

			scf_list_add_tail(h, &c->list);

			dst = c->dsts->data[0];
			dn  = dst->dag_node;
		}

		di = ds->dn_indexes->data[0];

		if (di->member) {
			assert(di->dn);

			c = _code_alloc_member_address(ast, dn, di->dn);

		} else {
			assert(di->index >= 0 || -1 == di->index);
			assert(di->dn_scale);

			c = _code_alloc_array_member_address(ast, dn, di->dn, di->dn_scale);
		}

		scf_list_add_tail(h, &c->list);

		dst = c->dsts->data[0];
		dn  = dst->dag_node;

	} else {
		c = _code_alloc_address(ast, dn);

		scf_list_add_tail(h, &c->list);

		dst = c->dsts->data[0];
		dn  = dst->dag_node;
	}

	int nb_pointers = scf_variable_nb_pointers(dn->var);

	assert(nb_pointers >= 2);

	c = _auto_gc_code_freep_array(ast, dn, nb_pointers - 1);

	scf_list_add_tail(h, &c->list);
	return 0;
}

static int _auto_gc_code_list_free_array(scf_list_t* h, scf_ast_t* ast, scf_dag_node_t* dn_array)
{
	scf_3ac_code_t* c;

	assert(dn_array->var->nb_dimentions > 0);
	assert(dn_array->var->capacity      > 0);

	c = _auto_gc_code_free_array(ast, dn_array, dn_array->var->capacity, dn_array->var->nb_pointers);

	scf_list_add_tail(h, &c->list);
	return 0;
}

static int _auto_gc_code_list_memset_array(scf_list_t* h, scf_ast_t* ast, scf_dag_node_t* dn_array)
{
	scf_3ac_code_t*    c;

	assert(dn_array->var->capacity > 0);

	c = _auto_gc_code_memset_array(ast, dn_array);

	scf_list_add_tail(h, &c->list);
	return 0;
}

static int _find_ds_malloced(scf_basic_block_t* bb, void* data)
{
	scf_dn_status_t* ds = data;

	if (!scf_vector_find_cmp(bb->ds_malloced, ds, scf_dn_status_cmp_same_dn_indexes))
		return 0;

	if (scf_vector_find_cmp(bb->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes)) {
		scf_loge("error free dn: \n");
		return -1;
	}

	if (scf_vector_find(bb->dn_updateds, ds->dag_node))
		return 1;
	return 0;
}

static int _find_dn_active(scf_basic_block_t* bb, void* data)
{
	scf_dag_node_t* dn = data;

	if (scf_vector_find(bb->dn_loads, data))
		return 1;

	if (scf_vector_find(bb->dn_reloads, data))
		return 1;

	scf_logd("bb: %p, dn: %s, 0\n", bb, dn->var->w->text->data);
	return 0;
}

static int _bb_find_ds_malloced(scf_basic_block_t* root, scf_list_t* bb_list_head, scf_dn_status_t* ds, scf_vector_t* results)
{
	scf_list_t*        l;
	scf_basic_block_t* bb;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		bb->visited_flag = 0;
	}

	return scf_basic_block_search_dfs_prev(root, _find_ds_malloced, ds, results);
}

static int _bb_find_dn_active(scf_basic_block_t* root, scf_list_t* bb_list_head, scf_dag_node_t* dn, scf_vector_t* results)
{
	scf_list_t*        l;
	scf_basic_block_t* bb;

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		bb = scf_list_data(l, scf_basic_block_t, list);
		l  = scf_list_next(l);

		bb->visited_flag = 0;
	}

	return scf_basic_block_search_dfs_prev(root, _find_dn_active, dn, results);
}

static int _bb_prev_add_active(scf_basic_block_t* bb, void* data, scf_vector_t* queue)
{
	scf_basic_block_t* bb_prev;

	scf_dag_node_t* dn = data;

	int count = 0;
	int ret;
	int j;

	for (j = 0; j < bb->prevs->size; j++) {
		bb_prev   = bb->prevs->data[j];

		if (!scf_vector_find(bb_prev->exit_dn_aliases, dn)) {

			ret = scf_vector_add_unique(bb_prev->exit_dn_actives, dn);
			if (ret < 0)
				return ret;
		}

		if (scf_vector_find(bb_prev->dn_updateds, dn)) {

			if (scf_vector_find(bb_prev->exit_dn_aliases, dn)
					|| scf_type_is_operator(dn->type))

				ret = scf_vector_add_unique(bb_prev->dn_resaves, dn);
			else
				ret = scf_vector_add_unique(bb_prev->dn_saves, dn);

			if (ret < 0)
				return ret;
		}

		++count;

		ret = scf_vector_add(queue, bb_prev);
		if (ret < 0)
			return ret;
	}
	return count;
}

static int _bb_add_active(scf_basic_block_t* bb, scf_dag_node_t* dn)
{
	int ret = scf_vector_add_unique(bb->entry_dn_actives, dn);
	if (ret < 0)
		return ret;

	if (scf_type_is_operator(dn->type))
		ret = scf_vector_add(bb->dn_reloads, dn);
	else
		ret = scf_vector_add(bb->dn_loads, dn);

	return ret;
}

static int _bb_add_gc_code_ref(scf_ast_t* ast, scf_basic_block_t* bb, scf_dn_status_t* ds)
{
	scf_3ac_code_t* c;
	scf_list_t*     l;
	scf_list_t      h;
	scf_list_init(&h);

	if (scf_vector_add_unique(bb->dn_reloads, ds->dag_node) < 0)
		return -ENOMEM;

	int ret = _auto_gc_code_list_ref(&h, ast, ds);
	if (ret < 0)
		return ret;

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_list_del(&c->list);
		scf_list_add_tail(&bb->code_list_head, &c->list);

		c->basic_block = bb;
	}
	return 0;
}

static int _bb_add_gc_code_freep(scf_ast_t* ast, scf_basic_block_t* bb, scf_dn_status_t* ds)
{
	scf_3ac_code_t* c;
	scf_list_t*     l;
	scf_list_t      h;
	scf_list_init(&h);

	if (scf_vector_add_unique(bb->dn_reloads, ds->dag_node) < 0)
		return -ENOMEM;

	int ret = _auto_gc_code_list_freep(&h, ast, ds);
	if (ret < 0)
		return ret;

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_list_del(&c->list);
		scf_list_add_tail(&bb->code_list_head, &c->list);

		c->basic_block = bb;
	}
	return 0;
}

static int _bb_add_gc_code_memset_array(scf_ast_t* ast, scf_basic_block_t* bb, scf_dag_node_t* dn_array)
{
	scf_3ac_code_t* c;
	scf_list_t*     l;
	scf_list_t      h;
	scf_list_init(&h);

	if (scf_vector_add_unique(bb->dn_reloads, dn_array) < 0)
		return -ENOMEM;

	int ret = _auto_gc_code_list_memset_array(&h, ast, dn_array);
	if (ret < 0)
		return ret;

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_list_del(&c->list);
		scf_list_add_tail(&bb->code_list_head, &c->list);

		c->basic_block = bb;
	}
	return 0;
}

static int _bb_add_gc_code_free_array(scf_ast_t* ast, scf_basic_block_t* bb, scf_dag_node_t* dn_array)
{
	scf_3ac_code_t* c;
	scf_list_t*     l;
	scf_list_t      h;
	scf_list_init(&h);

	if (scf_vector_add_unique(bb->dn_reloads, dn_array) < 0)
		return -ENOMEM;

	int ret = _auto_gc_code_list_free_array(&h, ast, dn_array);
	if (ret < 0)
		return ret;

	for (l = scf_list_head(&h); l != scf_list_sentinel(&h); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_list_del(&c->list);
		scf_list_add_tail(&bb->code_list_head, &c->list);

		c->basic_block = bb;
	}
	return 0;
}

static int _bb_add_free_arry(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t* bb, scf_dag_node_t* dn_array)
{
	scf_basic_block_t* bb1     = NULL;
	scf_dag_node_t*    dn2;

	int ret = scf_basic_block_split(bb, &bb1);
	if (ret < 0)
		return ret;

	scf_list_add_front(&bb->list, &bb1->list);

	if (bb->end_flag) {

		scf_basic_block_mov_code(scf_list_head(&bb->code_list_head), bb1, bb);

		bb1->ret_flag  = bb->ret_flag;
		bb1->end_flag  = 1;
		bb ->end_flag  = 0;
		bb ->call_flag = 1;

		bb1 = bb;
	} else {
		bb1->call_flag = 1;
	}

	ret = _bb_add_gc_code_free_array(ast, bb1, dn_array);
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn_array);
	if (ret < 0)
		return ret;

	return scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn_array);
}

static int _bb_add_memset_array(scf_ast_t* ast, scf_function_t* f, scf_dag_node_t* dn_array)
{
	scf_basic_block_t* bb   = NULL;
	scf_basic_block_t* bb1  = NULL;
	scf_dag_node_t*    dn2;
	scf_list_t*        l;

	l  = scf_list_head(&f->basic_block_list_head);
	bb = scf_list_data(l, scf_basic_block_t, list);

	int ret = scf_basic_block_split(bb, &bb1);
	if (ret < 0)
		return ret;

	scf_list_add_front(&bb->list, &bb1->list);

	scf_basic_block_mov_code(scf_list_head(&bb->code_list_head), bb1, bb);

	bb1->call_flag = 1;
	bb1->ret_flag  = bb->ret_flag;
	bb1->end_flag  = bb->end_flag;

	bb1 = bb;

	ret = _bb_add_gc_code_memset_array(ast, bb1, dn_array);
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn_array);
	if (ret < 0)
		return ret;

	return scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn_array);
}

static int _bb_split_prev_add_free(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t* bb, scf_dn_status_t* ds, scf_vector_t* bb_split_prevs)
{
	scf_basic_block_t* bb1;
	scf_basic_block_t* bb2;
	scf_basic_block_t* bb3;
	scf_dag_node_t*    dn           = ds->dag_node;
	scf_list_t*        bb_list_head = &f->basic_block_list_head;

	bb1 = scf_basic_block_alloc();
	if (!bb1) {
		scf_vector_free(bb_split_prevs);
		return -ENOMEM;
	}

	bb1->ds_auto_gc = scf_dn_status_clone(ds);
	if (!bb1->ds_auto_gc)
		return -ENOMEM;

	bb1->call_flag      = 1;
	bb1->auto_free_flag = 1;

	scf_vector_add( bb1->nexts, bb);
	scf_vector_free(bb1->prevs);

	bb1->prevs     = bb_split_prevs;
	bb_split_prevs = NULL;

	scf_list_add_tail(&bb->list, &bb1->list);

	int j;
	for (j  = 0; j < bb1->prevs->size; j++) {
		bb2 =        bb1->prevs->data[j];

		assert(0 == scf_vector_del(bb->prevs, bb2));

		int k;
		for (k = 0; k < bb2->nexts->size; k++) {

			if (bb2->nexts->data[k] == bb)
				bb2->nexts->data[k] =  bb1;
		}
	}

	for (j  = 0; j < bb->prevs->size; j++) {
		bb2 =        bb->prevs->data[j];

		scf_list_t*        l;
		scf_3ac_code_t*    c;
		scf_3ac_operand_t* dst;

		for (l  = scf_list_next(&bb2->list); l != scf_list_sentinel(bb_list_head);
			 l  = scf_list_next(l)) {

			bb3 = scf_list_data(l, scf_basic_block_t, list);

			if (bb3->jcc_flag)
				continue;

			if (bb3->jmp_flag)
				break;

			if (bb3 == bb1) {
				bb3 = scf_basic_block_alloc();
				if (!bb3)
					return -ENOMEM;
				bb3->jmp_flag = 1;

				c       = scf_3ac_code_alloc();
				c->op   = scf_3ac_find_operator(SCF_OP_GOTO);
				c->dsts = scf_vector_alloc();

				dst     = scf_3ac_operand_alloc();
				dst->bb = bb;

				if (scf_vector_add(c->dsts, dst) < 0)
					return -ENOMEM;

				c->basic_block = bb3;

				assert(0 == scf_vector_add(f->jmps, c));

				scf_list_add_tail(&bb3->code_list_head, &c->list);

				scf_list_add_tail(&bb1->list, &bb3->list);
			}
			break;
		}
	}
	scf_vector_add(bb->prevs, bb1);

	for (j  = 0; j < bb1->prevs->size; j++) {
		bb2 =        bb1->prevs->data[j];

		scf_list_t*        l;
		scf_list_t*        l2;
		scf_3ac_code_t*    c;
		scf_3ac_operand_t* dst;

		for (l  = scf_list_next(&bb2->list); l != &bb1->list && l != scf_list_sentinel(bb_list_head);
			 l  = scf_list_next(l)) {

			bb3 = scf_list_data(l, scf_basic_block_t, list);

			if (!bb3->jmp_flag)
				break;

			for (l2 = scf_list_head(&bb3->code_list_head); l2 != scf_list_sentinel(&bb3->code_list_head);
				 l2 = scf_list_next(l2)) {

				c   = scf_list_data(l2, scf_3ac_code_t, list);
				dst = c->dsts->data[0];

				if (dst->bb == bb)
					dst->bb = bb1;
			}
		}

		scf_dn_status_t* ds2;
		int k;
		for (k  = 0; k < bb2->ds_malloced->size; k++) {
			ds2 =        bb2->ds_malloced->data[k];

			if (0 == scf_dn_status_cmp_same_dn_indexes(ds2, ds))
				continue;

			if (scf_vector_find_cmp(bb2->ds_freed, ds2, scf_dn_status_cmp_same_dn_indexes))
				continue;

			if (scf_vector_find_cmp(bb1->ds_malloced, ds2, scf_dn_status_cmp_same_dn_indexes))
				continue;

			scf_dn_status_t* ds3 = scf_dn_status_clone(ds2);
			if (!ds3)
				return -ENOMEM;

			int ret = scf_vector_add(bb1->ds_malloced, ds3);
			if (ret < 0)
				return ret;
		}
	}

	int ret = _bb_add_gc_code_freep(ast, bb1, ds);
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn);
	if (ret < 0)
		return ret;

	return scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn);
}

static int _bb_split_prevs(scf_basic_block_t* bb, scf_dn_status_t* ds, scf_vector_t* bb_split_prevs)
{
	scf_basic_block_t* bb_prev;
	int i;

	for (i = 0; i < bb->prevs->size; i++) {
		bb_prev   = bb->prevs->data[i];

		if (!scf_vector_find_cmp(bb_prev->ds_malloced, ds, scf_dn_status_cmp_like_dn_indexes))
			continue;

		if (scf_vector_find_cmp(bb_prev->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes))
			continue;

		int ret = scf_vector_add(bb_split_prevs, bb_prev);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int _auto_gc_last_free(scf_ast_t* ast, scf_function_t* f)
{
	scf_list_t*        bb_list_head = &f->basic_block_list_head;
	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_vector_t*      local_arrays;

	l  = scf_list_tail(bb_list_head);
	bb = scf_list_data(l, scf_basic_block_t, list);

	scf_logd("bb: %p, bb->ds_malloced->size: %d\n", bb, bb->ds_malloced->size);

	local_arrays = scf_vector_alloc();
	if (!local_arrays)
		return -ENOMEM;

	int ret;
	int i;
	for (i = 0; i < bb->ds_malloced->size; ) {

		scf_dn_status_t* ds = bb->ds_malloced->data[i];
		scf_dag_node_t*  dn = ds->dag_node;
		scf_variable_t*  v  = dn->var;

		scf_loge("f: %s, last free: v_%d_%d/%s, ds->ret: %u\n",
				f->node.w->text->data, v->w->line, v->w->pos, v->w->text->data, ds->ret);
		scf_dn_status_print(ds);

		if (ds->ret) {
			i++;
			continue;
		}

		if (ds->dn_indexes) {

			scf_dn_index_t*  di;

			int var_index = 0;
			int j;

			for (j = ds->dn_indexes->size - 1; j >= 0; j--) {
				di = ds->dn_indexes->data[j];

				if (di->member) {
					assert(di->member->member_flag && v->type >= SCF_STRUCT);
					break;
				}

				if (-1 == di->index)
					var_index++;
			}

			if (var_index > 0) {
				if (v->nb_dimentions > 0 && v->local_flag) {

					if (scf_vector_add_unique(local_arrays, dn) < 0)
						return -ENOMEM;
				}

				i++;
				continue;
			}

			if (j >= 0) {
				i++;
				continue;
			}
		}

		scf_vector_t* vec;

		vec = scf_vector_alloc();
		if (!vec)
			return -ENOMEM;

		scf_basic_block_t* bb1;
		scf_basic_block_t* bb2;
		scf_basic_block_t* bb3;
		scf_basic_block_t* bb_dominator;

		int dfo = 0;
		int j;

		ret = _bb_find_ds_malloced(bb, bb_list_head, ds, vec);
		if (ret < 0) {
			scf_vector_free(vec);
			return ret;
		}
#define AUTO_GC_FIND_MAX_DFO() \
		do { \
			for (j  = 0; j < vec->size; j++) { \
				bb1 =        vec->data[j]; \
				\
				if (bb1->dfo_normal > dfo) \
					dfo = bb1->dfo_normal; \
			} \
		} while (0)
		AUTO_GC_FIND_MAX_DFO();

		vec->size = 0;

		ret = _bb_find_dn_active(bb, bb_list_head, dn, vec);
		if (ret < 0) {
			scf_vector_free(vec);
			return ret;
		}
		AUTO_GC_FIND_MAX_DFO();

		for (j = 0; j    < bb->dominators_normal->size; j++) {
			bb_dominator = bb->dominators_normal->data[j];

			if (bb_dominator->dfo_normal > dfo)
				break;
		}

		vec->size = 0;

		ret = _bb_split_prevs(bb_dominator, ds, vec);
		if (ret < 0) {
			scf_vector_free(vec);
			return ret;
		}

		ret = _bb_split_prev_add_free(ast, f, bb_dominator, ds, vec);
		if (ret < 0)
			return ret;
		i++;
	}

	scf_dag_node_t*    dn;
	scf_variable_t*    v;

	for (i = 0; i < local_arrays->size; i++) {
		dn =        local_arrays->data[i];

		ret = _bb_add_memset_array(ast, f, dn);
		if (ret < 0)
			return ret;

		ret = _bb_add_free_arry(ast, f, bb, dn);
		if (ret < 0)
			return ret;
	}

	return 0;
}

#define AUTO_GC_BB_SPLIT(parent, child) \
	do { \
		int ret = scf_basic_block_split(parent, &child); \
		if (ret < 0) \
			return ret; \
		\
		child->call_flag        = parent->call_flag; \
		child->dereference_flag = parent->dereference_flag; \
		\
		SCF_XCHG(parent->ds_malloced, child->ds_malloced); \
		\
		scf_vector_free(child->exit_dn_actives); \
		scf_vector_free(child->exit_dn_aliases); \
		\
		child->exit_dn_actives = scf_vector_clone(parent->exit_dn_actives); \
		child->exit_dn_aliases = scf_vector_clone(parent->exit_dn_aliases); \
		\
		scf_list_add_front(&parent->list, &child->list); \
	} while (0)

static int _optimize_auto_gc_bb_ref(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t** pbb, scf_list_t* bb_list_head, scf_dn_status_t* ds)
{
	scf_basic_block_t* cur_bb = *pbb;
	scf_basic_block_t* bb1    = NULL;
	scf_dag_node_t*    dn     = ds->dag_node;

	AUTO_GC_BB_SPLIT(cur_bb, bb1);

	bb1->ds_auto_gc = scf_dn_status_clone(ds);
	if (!bb1->ds_auto_gc)
		return -ENOMEM;

	int ret = _bb_add_gc_code_ref(ast, bb1, ds);
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn);
	if (ret < 0)
		return ret;

	ret = scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn);
	if (ret < 0)
		return ret;

	bb1->call_flag  = 1;
	bb1->dereference_flag = 0;
	bb1->auto_ref_flag    = 1;

	*pbb = bb1;
	return 0;
}

static int _optimize_auto_gc_bb_free(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t** pbb, scf_dn_status_t* ds)
{
	scf_basic_block_t* cur_bb = *pbb;
	scf_basic_block_t* bb1    = NULL;
	scf_basic_block_t* bb2    = NULL;
	scf_dag_node_t*    dn           = ds->dag_node;
	scf_list_t*        bb_list_head = &f->basic_block_list_head;

	AUTO_GC_BB_SPLIT(cur_bb, bb1);
	AUTO_GC_BB_SPLIT(bb1,    bb2);

	bb1->ds_auto_gc = scf_dn_status_clone(ds);
	if (!bb1->ds_auto_gc)
		return -ENOMEM;

	bb1->call_flag  = 1;
	bb1->dereference_flag = 0;
	bb1->auto_free_flag   = 1;

	int ret = _bb_add_gc_code_freep(ast, bb1, ds);
	if (ret < 0)
		return ret;

	ret = _bb_add_active(bb1, dn);
	if (ret < 0)
		return ret;

	ret = scf_basic_block_search_bfs(bb1, _bb_prev_add_active, dn);
	if (ret < 0)
		return ret;

	*pbb = bb2;
	return 0;
}

static int _bb_prevs_malloced(scf_basic_block_t* bb, scf_vector_t* ds_malloced)
{
	scf_basic_block_t* bb_prev;
	int i;
	int j;

	for (i = 0; i < bb->prevs->size; i++) {

		bb_prev   = bb->prevs->data[i];

		for (j = 0; j < bb_prev->ds_malloced->size; j++) {

			scf_dn_status_t* ds = bb_prev->ds_malloced->data[j];

			if (scf_vector_find_cmp(bb_prev->ds_freed, ds, scf_dn_status_cmp_same_dn_indexes))
				continue;

			if (scf_vector_find_cmp(ds_malloced, ds, scf_dn_status_cmp_like_dn_indexes))
				continue;

			scf_dn_status_t* ds2 = scf_dn_status_clone(ds);
			if (!ds2)
				return -ENOMEM;

			int ret = scf_vector_add(ds_malloced, ds2);
			if (ret < 0) {
				scf_dn_status_free(ds2);
				return ret;
			}
		}
	}
	return 0;
}

int __bb_find_ds_alias(scf_vector_t* aliases, scf_dn_status_t* ds_obj, scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head);

static int _bb_need_ref(scf_dn_status_t* ds_obj, scf_vector_t* ds_malloced,
		scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds_alias;
	scf_vector_t*    aliases;

	int ret;
	int i;

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

	ret = __bb_find_ds_alias(aliases, ds_obj, c, bb, bb_list_head);
	if (ret < 0)
		return ret;

	scf_logd("aliases->size: %d\n", aliases->size);

	int need = 0;
	for (i = 0; i < aliases->size; i++) {
		ds_alias  = aliases->data[i];

		if (!ds_alias->dag_node)
			continue;

		if (scf_vector_find_cmp(ds_malloced, ds_alias, scf_dn_status_cmp_like_dn_indexes)) {
			need = 1;
			break;
		}
	}

	if (scf_vector_find_cmp(ds_malloced, ds_obj, scf_dn_status_cmp_like_dn_indexes)) {
		need = 1;
	}

	scf_vector_free (aliases);
	return need;
}

static int _bb_split_prevs_need_free(scf_dn_status_t* ds_obj, scf_vector_t* ds_malloced, scf_vector_t* bb_split_prevs,
		scf_3ac_code_t* c, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_dn_status_t* ds_obj2;
	scf_dn_status_t* ds_alias;
	scf_dn_index_t*  di;
	scf_vector_t*    aliases;

	int ret;
	int i;
	int need = 0;

	aliases = scf_vector_alloc();
	if (!aliases)
		return -ENOMEM;

#define SPLIT_PREVS_NEED_FREE(malloced, obj, split_prevs) \
		do { \
			scf_dn_status_t* ds; \
			\
			if (scf_vector_find_cmp(malloced, obj, scf_dn_status_cmp_like_dn_indexes)) { \
				\
				if (_bb_split_prevs(bb, obj, split_prevs) < 0) { \
					ret = -ENOMEM; \
					goto error; \
				} \
				\
				need = 1; \
			} \
		} while (0)

#if 0
	ret = __bb_find_ds_alias(aliases, ds_obj, c, bb, bb_list_head);
	if (ret < 0)
		return ret;

#if 0
	printf("\n");
	int k;
	for (k = 0; k < ds_malloced->size; k++) {
		ds_alias  = ds_malloced->data[k];

		scf_loge("## k: %d, ds_malloced: %p, size: %d\n", k, ds_malloced, ds_malloced->size);
		scf_dn_status_print(ds_alias);
	}
#endif
	for (i = 0; i < aliases->size; i++) {
		ds_alias  = aliases->data[i];

		if (!ds_alias->dag_node)
			continue;


		SPLIT_PREVS_NEED_FREE(ds_malloced, ds_alias, bb_split_prevs);
	}
		//		ds = scf_vector_find_cmp(malloced, obj, scf_dn_status_cmp_same_dn_indexes); \
				if (ds) {\
				    assert(0 == scf_vector_del(malloced, ds)); \
				    scf_dn_status_free(ds); \
				} \
#endif
#endif
	SPLIT_PREVS_NEED_FREE(ds_malloced, ds_obj, bb_split_prevs);
	ret = need;

error:
//	scf_vector_clear(aliases, ( void (*)(void*) ) scf_dn_status_free);
	scf_vector_free (aliases);
	return ret;
}

static int _optimize_auto_gc_bb(scf_ast_t* ast, scf_function_t* f, scf_basic_block_t* bb, scf_list_t* bb_list_head)
{
	scf_list_t*     l;
	scf_3ac_code_t* c;
	scf_vector_t*   ds_malloced;
	scf_vector_t*   ds_assigned;

	scf_basic_block_t* bb_prev = NULL;
	scf_basic_block_t* cur_bb  = bb;

	ds_malloced = scf_vector_alloc();
	if (!ds_malloced)
		return -ENOMEM;

	ds_assigned = scf_vector_alloc();
	if (!ds_assigned)
		return -ENOMEM;

	// at first, the malloced vars, are ones malloced in previous blocks

	int ret = _bb_prevs_malloced(bb, ds_malloced);
	if (ret < 0) {
		scf_vector_clear(ds_malloced, ( void (*)(void*) )scf_dn_status_free);
		scf_vector_free(ds_malloced);
		return ret;
	}

	for (l = scf_list_head(&bb->code_list_head); l != scf_list_sentinel(&bb->code_list_head); ) {

		c  = scf_list_data(l, scf_3ac_code_t, list);
		l  = scf_list_next(l);

		scf_3ac_operand_t* base;
		scf_3ac_operand_t* member;
		scf_3ac_operand_t* index;
		scf_3ac_operand_t* scale;

		scf_3ac_operand_t* dst;
		scf_3ac_operand_t* src;
		scf_dag_node_t*    dn;
		scf_dn_status_t*   ds_obj;
		scf_variable_t*    v0;

		if (SCF_OP_ASSIGN == c->op->type) {

			dst = c->dsts->data[0];
			v0  = dst->dag_node->var;

			if (!scf_variable_may_malloced(v0))
				goto _end;

			ds_obj = NULL;

			ret = scf_ds_for_dn(&ds_obj, dst->dag_node);
			if (!ds_obj)
				return -ENOMEM;

			src = c->srcs->data[0];
			dn  = src->dag_node;

		} else if (SCF_OP_3AC_ASSIGN_ARRAY_INDEX == c->op->type) {

			assert(4 == c->srcs->size);

			base  = c->srcs->data[0];
			index = c->srcs->data[1];
			scale = c->srcs->data[2];
			src   = c->srcs->data[3];
			dn    = src->dag_node;
			v0    = _scf_operand_get(base->node->parent);

			if (!scf_variable_may_malloced(v0))
				goto _end;

			ds_obj = NULL;

			int ret = scf_ds_for_assign_array_member(&ds_obj, base->dag_node, index->dag_node, scale->dag_node);
			if (ret < 0)
				return ret;

			if (ds_obj->dag_node->var->arg_flag)
				ds_obj->ret = 1;

		} else if (SCF_OP_3AC_ASSIGN_POINTER == c->op->type) {

			assert(3 == c->srcs->size);

			base   = c->srcs->data[0];
			member = c->srcs->data[1];
			src    = c->srcs->data[2];
			dn     = src->dag_node;
			v0     = member->dag_node->var;

			if (!scf_variable_may_malloced(v0))
				goto _end;

			ds_obj = NULL;

			int ret = scf_ds_for_assign_member(&ds_obj, base->dag_node, member->dag_node);
			if (ret < 0)
				return ret;

			if (ds_obj->dag_node->var->arg_flag)
				ds_obj->ret = 1;

		} else if (SCF_OP_3AC_ASSIGN_DEREFERENCE == c->op->type) {

			assert(2 == c->srcs->size);

			src = c->srcs->data[1];
			dn  = src->dag_node;
			v0  = dn->var;

			if (!scf_variable_may_malloced(v0))
				goto _end;

			src     = c->srcs->data[0];
			ds_obj  = NULL;

			int ret = scf_ds_for_assign_dereference(&ds_obj, src->dag_node);
			if (ret < 0)
				return ret;

		} else
			goto _end;

		scf_vector_t* bb_split_prevs;

		bb_split_prevs = scf_vector_alloc();
		if (!bb_split_prevs)
			return -ENOMEM;

		ret = _bb_split_prevs_need_free(ds_obj, ds_malloced, bb_split_prevs, c, bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		if (ret) {
			if (!scf_vector_find_cmp(ds_assigned, ds_obj, scf_dn_status_cmp_like_dn_indexes)
					&& bb_split_prevs->size > 0
					&& bb_split_prevs->size < bb->prevs->size)

				ret = _bb_split_prev_add_free(ast, f, bb, ds_obj, bb_split_prevs);
			else {
				scf_vector_free(bb_split_prevs);
				bb_split_prevs = NULL;

				ret = _optimize_auto_gc_bb_free(ast, f, &cur_bb, ds_obj);
			}

			if (ret < 0)
				return ret;
		} else {
			scf_vector_free(bb_split_prevs);
			bb_split_prevs = NULL;
		}

		if (scf_vector_add(ds_assigned, ds_obj) < 0) {
			scf_dn_status_free(ds_obj);
			return -ENOMEM;
		}

_ref:
		if (!dn->var->local_flag && cur_bb != bb)
			dn ->var->tmp_flag = 1;

		while (dn) {
			if (SCF_OP_TYPE_CAST == dn->type)
				dn = dn->childs->data[0];

			else if (SCF_OP_EXPR == dn->type)
				dn = dn->childs->data[0];
			else
				break;
		}

		if (SCF_OP_CALL == dn->type || dn->node->split_flag) {

			if (dn->node->split_flag) {
				assert(SCF_OP_CALL   == dn->node->split_parent->type
					|| SCF_OP_CREATE == dn->node->split_parent->type);
			}

			scf_dag_node_t* dn_pf;
			scf_function_t* f2;
			scf_variable_t* ret;

			dn_pf = dn->childs->data[0];
			f2    = dn_pf->var->func_ptr;
			ret   = f2->rets->data[0];

			if (!strcmp(f2->node.w->text->data, "scf__auto_malloc") || ret->auto_gc_flag) {

				if (SCF_OP_RETURN == c->op->type) {
					ret = f->rets->data[0];
					ret->auto_gc_flag = 1;
				}

				if (!scf_vector_find_cmp(ds_malloced, ds_obj, scf_dn_status_cmp_like_dn_indexes)) {

					assert(0 == scf_vector_add(ds_malloced, ds_obj));
				} else {
					scf_dn_status_free(ds_obj);
					ds_obj = NULL;
				}

				if (cur_bb != bb) {
					scf_list_del(&c->list);
					scf_list_add_tail(&cur_bb->code_list_head, &c->list);
				}
				continue;
			}
		} else {
			scf_dn_status_t* ds = NULL;

			ret = scf_ds_for_dn(&ds, dn);
			if (ret < 0)
				return ret;

			int ret = _bb_need_ref(ds, ds_malloced, c, bb, bb_list_head);

			scf_dn_status_free(ds);
			ds = NULL;

			if (ret < 0)
				return ret;

			if (ret > 0) {
				scf_basic_block_t* bb2 = NULL;

				if (SCF_OP_RETURN == c->op->type) {
					scf_variable_t* ret = f->rets->data[0];
					ret->auto_gc_flag = 1;
				}

				if (cur_bb != bb) {
					scf_list_del(&c->list);
					scf_list_add_tail(&cur_bb->code_list_head, &c->list);
				}

				int ret = _optimize_auto_gc_bb_ref(ast, f, &cur_bb, bb_list_head, ds_obj);
				if (ret < 0)
					return ret;

				if (!scf_vector_find_cmp(ds_malloced, ds_obj, scf_dn_status_cmp_like_dn_indexes)) {

					assert(0 == scf_vector_add(ds_malloced, ds_obj));
				} else {
					scf_dn_status_free(ds_obj);
					ds_obj = NULL;
				}

				if (l != scf_list_sentinel(&bb->code_list_head)) {

					AUTO_GC_BB_SPLIT(cur_bb, bb2);
					cur_bb = bb2;
				}

				continue;
			}
		}

		scf_dn_status_free(ds_obj);
		ds_obj = NULL;
_end:
		if (cur_bb != bb) {
			scf_list_del(&c->list);
			scf_list_add_tail(&cur_bb->code_list_head, &c->list);
		}
	}

	return 0;
}

static int _optimize_auto_gc(scf_ast_t* ast, scf_function_t* f, scf_list_t* bb_list_head, scf_vector_t* functions)
{
	if (!ast || !f || !bb_list_head)
		return -EINVAL;

	if (!strcmp(f->node.w->text->data, "scf__auto_malloc"))
		return 0;

	scf_list_t*        l;
	scf_basic_block_t* bb;
	scf_basic_block_t* bb2;

	int ret;

	ret = _auto_gc_last_free(ast, f);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	for (l = scf_list_head(bb_list_head); l != scf_list_sentinel(bb_list_head); ) {

		scf_dag_node_t* dn;
		scf_vector_t*   dn_actives;

		scf_list_t*     start = l;
		scf_list_t*     l2;

		bb = scf_list_data(l, scf_basic_block_t, list);

		for (l  = scf_list_next(l); l != scf_list_sentinel(bb_list_head); l = scf_list_next(l)) {

			bb2 = scf_list_data(l, scf_basic_block_t, list);

			if (!bb2->auto_ref_flag && !bb2->auto_free_flag)
				break;
		}

		ret = _optimize_auto_gc_bb(ast, f, bb, bb_list_head);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		dn_actives = scf_vector_alloc();
		if (!dn_actives)
			return -ENOMEM;

		for (l2 = scf_list_prev(l); l2 != scf_list_prev(start); l2 = scf_list_prev(l2)) {

			bb  = scf_list_data(l2, scf_basic_block_t, list);

			ret = scf_basic_block_active_vars(bb);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			int i;
			for (i = 0; i < dn_actives->size; i++) {
				dn =        dn_actives->data[i];

				ret = scf_vector_add_unique(bb->exit_dn_actives, dn);
				if (ret < 0) {
					scf_vector_free(dn_actives);
					return ret;
				}
			}

			for (i = 0; i < bb->entry_dn_actives->size; i++) {
				dn =        bb->entry_dn_actives->data[i];

				ret = scf_vector_add_unique(dn_actives, dn);
				if (ret < 0) {
					scf_vector_free(dn_actives);
					return ret;
				}
			}
		}

		scf_vector_free(dn_actives);
		dn_actives = NULL;

		for (l2 = scf_list_prev(l); l2 != scf_list_prev(start); ) {

			bb  = scf_list_data(l2, scf_basic_block_t, list);
			l2  = scf_list_prev(l2);
#if 1
			if (l2 != start && scf_list_prev(l) != &bb->list) {

				bb2 = scf_list_data(l2, scf_basic_block_t, list);

				if (bb->auto_free_flag
						&& bb2->auto_ref_flag
						&& 0 == scf_dn_status_cmp_same_dn_indexes(bb->ds_auto_gc, bb2->ds_auto_gc)) {

					scf_basic_block_t* bb3;
					scf_basic_block_t* bb4;

					assert(1 == bb2->prevs->size);
					assert(1 == bb ->nexts->size);

					bb3 = bb2->prevs->data[0];
					bb4 = bb ->nexts->data[0];

					assert(1 == bb3->nexts->size);
					assert(1 == bb4->prevs->size);

					bb3->nexts->data[0] = bb4;
					bb4->prevs->data[0] = bb3;

					l2 = scf_list_prev(l2);

					scf_list_del(&bb->list);
					scf_list_del(&bb2->list);

					scf_basic_block_free(bb);
					scf_basic_block_free(bb2);
					bb  = NULL;
					bb2 = NULL;
					continue;
				}
			}
#endif
			ret = scf_basic_block_loads_saves(bb, bb_list_head);
			if (ret < 0)
				return ret;
		}
	}

//	scf_basic_block_print_list(bb_list_head);
	return 0;
}

scf_optimizer_t  scf_optimizer_auto_gc =
{
	.name     =  "auto_gc",

	.optimize =  _optimize_auto_gc,

	.flags    = SCF_OPTIMIZER_LOCAL,
};

