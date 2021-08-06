#include"scf_ast.h"
#include"scf_operator_handler_semantic.h"
#include"scf_type_cast.h"

typedef struct {
	scf_variable_t**	pret;

} scf_handler_data_t;

static int __scf_op_semantic_call(scf_ast_t* ast, scf_function_t* f, void* data);

static int _scf_expr_calculate_internal(scf_ast_t* ast, scf_node_t* node, void* data);

static int _semantic_add_address_of(scf_ast_t* ast, scf_node_t** pp, scf_node_t* src)
{
	scf_node_t* parent = src->parent;

	scf_operator_t* op = scf_find_base_operator_by_type(SCF_OP_ADDRESS_OF);
	if (!op)
		return -EINVAL;

	scf_variable_t* v_src = _scf_operand_get(src);

	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, v_src->type);
	if (ret < 0)
		return ret;
	assert(t);

	scf_variable_t* v = SCF_VAR_ALLOC_BY_TYPE(v_src->w, t, v_src->const_flag, v_src->nb_pointers + 1, v_src->func_ptr);
	if (!v)
		return -ENOMEM;

	scf_node_t* address_of = scf_node_alloc(NULL, SCF_OP_ADDRESS_OF, NULL);
	if (!address_of) {
		scf_variable_free(v);
		return -ENOMEM;
	}

	ret = scf_node_add_child(address_of, src);
	if (ret < 0) {
		scf_variable_free(v);
		scf_node_free(address_of);
		return ret;
	}

	address_of->op     = op;
	address_of->result = v;
	address_of->parent = parent;
	*pp = address_of;
	return 0;
}

static int _semantic_add_type_cast(scf_ast_t* ast, scf_node_t** pp, scf_variable_t* v_dst, scf_node_t* src)
{
	scf_node_t* parent = src->parent;

	scf_operator_t* op = scf_find_base_operator_by_type(SCF_OP_TYPE_CAST);
	if (!op)
		return -EINVAL;

	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, v_dst->type);
	if (ret < 0)
		return ret;
	assert(t);

	scf_variable_t* v_src = _scf_operand_get(src);
	scf_variable_t* v     = SCF_VAR_ALLOC_BY_TYPE(NULL, t, v_src->const_flag, v_dst->nb_pointers, v_dst->func_ptr);
	if (!v)
		return -ENOMEM;

	scf_node_t* dst = scf_node_alloc(NULL, v->type, v);
	scf_variable_free(v);
	v = NULL;
	if (!dst)
		return -ENOMEM;

	scf_node_t* cast = scf_node_alloc(NULL, SCF_OP_TYPE_CAST, NULL);
	if (!cast) {
		scf_node_free(dst);
		return -ENOMEM;
	}

	ret = scf_node_add_child(cast, dst);
	if (ret < 0) {
		scf_node_free(dst);
		scf_node_free(cast);
		return ret;
	}

	ret = scf_node_add_child(cast, src);
	if (ret < 0) {
		scf_node_free(cast); // dst is cast's child, will be recursive freed
		return ret;
	}

	cast->op     = op;
	cast->result = scf_variable_ref(dst->var);
	cast->parent = parent;
	*pp = cast;
	return 0;
}

static int _semantic_do_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	int ret = scf_find_updated_type(ast, v0, v1);
	if (ret < 0) {
		scf_loge("var type update failed, type: %d, %d\n", v0->type, v1->type);
		return -EINVAL;
	}

	scf_type_t* t = NULL;
	ret = scf_ast_find_type_type(&t, ast, ret);
	if (ret < 0)
		return ret;
	assert(t);

	scf_variable_t* v_std = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 0, 0, NULL);
	if (!v_std)
		return -ENOMEM;

	if (t->type != v0->type) {
		ret = _semantic_add_type_cast(ast, &nodes[0], v_std, nodes[0]);
		if (ret < 0) {
			scf_loge("add type cast failed\n");
			goto end;
		}
	}

	if (t->type != v1->type) {
		ret = _semantic_add_type_cast(ast, &nodes[1], v_std, nodes[1]);
		if (ret < 0) {
			scf_loge("add type cast failed\n");
			goto end;
		}
	}

	ret = 0;
end:
	scf_variable_free(v_std);
	return ret;
}

static void _semantic_check_var_size(scf_ast_t* ast, scf_node_t* node)
{
	scf_type_t* t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, node->type);
	assert(0 == ret);
	assert(t);

	if (0 == node->var->size) {
		node->var->size = t->size;
		scf_logd("node: %p var: %p, var->size: %d\n", node, node->var, node->var->size);
	}

	if (0 == node->var->data_size)
		node->var->data_size = t->size;
}

static int _semantic_add_call_rets(scf_ast_t* ast, scf_node_t* parent, scf_handler_data_t* d, scf_function_t* f)
{
	scf_variable_t* fret;
	scf_variable_t* r;
	scf_type_t*	    t;
	scf_node_t*     node;


	if (f->rets->size > 0) {
		if (!parent->result_nodes) {

			parent->result_nodes = scf_vector_alloc();
			if (!parent->result_nodes)
				return -ENOMEM;
		} else
			scf_vector_clear(parent->result_nodes, ( void(*)(void*) ) scf_node_free);
	}

	int i;
	for (i   = 0; i < f->rets->size; i++) {
		fret =        f->rets->data[i];

		t = NULL;
		int ret = scf_ast_find_type_type(&t, ast, fret->type);
		if (ret < 0)
			return ret;
		assert(t);

		r    = SCF_VAR_ALLOC_BY_TYPE(parent->w, t, fret->const_flag, fret->nb_pointers, fret->func_ptr);
		node = scf_node_alloc(r->w, parent->type, NULL);
//		node = scf_node_alloc(NULL, r->type, r);
		if (!node) {
			scf_loge("\n");
			return -ENOMEM;
		}

		node->result       = r;
		node->op           = parent->op;
		node->split_parent = parent;
		node->split_flag   = 1;

		if (scf_vector_add(parent->result_nodes, node) < 0) {
			scf_loge("\n");
			scf_node_free(node);
			return -ENOMEM;
		}
	}

	if (d->pret && parent->result_nodes->size > 0) {

		r = _scf_operand_get(parent->result_nodes->data[0]);

		*d->pret = scf_variable_ref(r);
	}

	return 0;
}

static int _semantic_add_call(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, scf_handler_data_t* d, scf_function_t* f)
{
	assert(nb_nodes >= 1);

	scf_variable_t* var_pf  = NULL;
	scf_node_t*     node_pf = NULL;
	scf_node_t*     node    = NULL;
	scf_node_t*     parent  = nodes[0]->parent;
	scf_type_t*     pt      = scf_block_find_type_type(ast->current_block, SCF_FUNCTION_PTR);


	var_pf = SCF_VAR_ALLOC_BY_TYPE(f->node.w, pt, 1, 1, f);
	if (!var_pf) {
		scf_loge("var alloc error\n");
		return -ENOMEM;
	}
	var_pf->const_flag = 1;
	var_pf->const_literal_flag = 1;

	node_pf = scf_node_alloc(NULL, var_pf->type, var_pf);
	if (!node_pf) {
		scf_loge("node alloc failed\n");
		return -ENOMEM;
	}

	parent->type = SCF_OP_CALL;
	parent->op   = scf_find_base_operator_by_type(SCF_OP_CALL);

	scf_node_add_child(parent, node_pf);

	int i;
	for (i = parent->nb_nodes - 2; i >= 0; i--)
		parent->nodes[i + 1] = parent->nodes[i];
	parent->nodes[0] = node_pf;

	return _semantic_add_call_rets(ast, parent, d, f);
}

static int _semantic_find_proper_function2(scf_ast_t* ast, scf_vector_t* fvec, scf_vector_t* argv, scf_function_t** pf)
{
	scf_function_t* f;
	scf_variable_t* v0;
	scf_variable_t* v1;

	int i;
	int j;

	for (i = 0; i < fvec->size; i++) {
		f  =        fvec->data[i];

		if (scf_function_same_argv(f->argv, argv)) {
			*pf = f;
			return 0;
		}
	}

	for (i = 0; i < fvec->size; i++) {
		f  =        fvec->data[i];

		for (j = 0; j < argv->size; j++) {

			v0 =     f->argv->data[j];
			v1 =        argv->data[j];

			if (scf_variable_is_struct_pointer(v0))
				continue;

			if (scf_type_cast_check(ast, v0, v1) < 0)
				break;

			*pf = f;
			return 0;
		}
	}

	return -404;
}

static int _semantic_find_proper_function(scf_ast_t* ast, scf_type_t* t, const char* fname, scf_vector_t* argv, scf_function_t** pf)
{
	scf_vector_t* fvec = NULL;

	int ret  = scf_scope_find_like_functions(&fvec, t->scope, fname, argv);
	if (ret < 0)
		return ret;

	ret = _semantic_find_proper_function2(ast, fvec, argv, pf);

	scf_vector_free(fvec);
	return ret;
}

static int _semantic_do_overloaded2(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, scf_handler_data_t* d, scf_vector_t* argv, scf_function_t* f)
{
	scf_variable_t* v0;
	scf_variable_t* v1;

	int i;
	for (i = 0; i < argv->size; i++) {

		v0 =     f->argv->data[i];
		v1 =        argv->data[i];

		if (scf_variable_is_struct_pointer(v0))
			continue;

		if (scf_variable_same_type(v0, v1))
			continue;

		int ret = _semantic_add_type_cast(ast, &nodes[i], v0, nodes[i]);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	return _semantic_add_call(ast, nodes, nb_nodes, d, f);
}

static int _semantic_do_overloaded(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, scf_handler_data_t* d)
{
	scf_function_t* f;
	scf_variable_t* v0;
	scf_variable_t* v1;
	scf_vector_t*   argv;
	scf_vector_t*   fvec   = NULL;
	scf_node_t*     parent = nodes[0]->parent;
	scf_type_t*	    t      = NULL;

	argv = scf_vector_alloc();
	if (!argv)
		return -ENOMEM;

	int ret;
	int i;

	for (i = 0; i < nb_nodes; i++) {

		v0 = _scf_operand_get(nodes[i]);

		if (!t && scf_variable_is_struct_pointer(v0)) {

			t = NULL;
			ret = scf_ast_find_type_type(&t, ast, v0->type);
			if (ret < 0)
				return ret;
			assert(t->scope);
		}

		ret = scf_vector_add(argv, v0);
		if (ret < 0) {
			scf_vector_free(argv);
			return ret;
		}
	}

	ret  = scf_scope_find_overloaded_functions(&fvec, t->scope, parent->type, argv);
	if (ret < 0) {
		scf_vector_free(argv);
		return ret;
	}

	ret = _semantic_find_proper_function2(ast, fvec, argv, &f);
	if (ret < 0)
		scf_loge("\n");
	else
		ret = _semantic_do_overloaded2(ast, nodes, nb_nodes, d, argv, f);

	scf_vector_free(fvec);
	scf_vector_free(argv);
	return ret;
}

static int _semantic_do_overloaded_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, scf_handler_data_t* d)
{
	scf_function_t* f;
	scf_variable_t* v;
	scf_vector_t*   argv;
	scf_vector_t*   fvec   = NULL;
	scf_node_t*     parent = nodes[0]->parent;
	scf_type_t*	    t      = NULL;

	argv = scf_vector_alloc();
	if (!argv)
		return -ENOMEM;

	int ret;
	int i;
	for (i = 0; i < nb_nodes; i++) {

		v  = _scf_operand_get(nodes[i]);

		if (scf_variable_is_struct(v)) {
			if (!t) {
				t = NULL;
				ret = scf_ast_find_type_type(&t, ast, v->type);
				if (ret < 0)
					return ret;
				assert(t->scope);
			}

			ret = _semantic_add_address_of(ast, &nodes[i], nodes[i]);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}

			v  = _scf_operand_get(nodes[i]);
		}

		ret = scf_vector_add(argv, v);
		if (ret < 0) {
			scf_vector_free(argv);
			return ret;
		}
	}

	ret  = scf_scope_find_overloaded_functions(&fvec, t->scope, parent->type, argv);
	if (ret < 0) {
		scf_vector_free(argv);
		return ret;
	}

	ret = _semantic_find_proper_function2(ast, fvec, argv, &f);
	if (ret < 0)
		scf_loge("\n");
	else
		ret = _semantic_do_overloaded2(ast, nodes, nb_nodes, d, argv, f);

	scf_vector_free(fvec);
	scf_vector_free(argv);
	return ret;
}

static int _semantic_do_create(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, scf_handler_data_t* d)
{
	scf_variable_t* v0;
	scf_variable_t* v_pf;
	scf_block_t*    b;
	scf_type_t*	    t;
	scf_type_t*	    pt;
	scf_node_t*     parent  = nodes[0]->parent;
	scf_node_t*     node0   = nodes[0];
	scf_node_t*     node1   = nodes[1];
	scf_node_t*     create  = NULL;
	scf_node_t*     node_pf = NULL;

	v0 = _scf_operand_get(nodes[0]);

	t  = NULL;
	int ret = scf_ast_find_type_type(&t, ast, v0->type);
	if (ret < 0)
		return ret;

	pt = scf_block_find_type_type(ast->current_block, SCF_FUNCTION_PTR);
	assert(t);
	assert(pt);

	create = scf_node_alloc(parent->w, SCF_OP_CREATE, NULL);
	if (!create)
		return -ENOMEM;

	v_pf = SCF_VAR_ALLOC_BY_TYPE(t->w, pt, 1, 1, NULL);
	if (!v_pf)
		return -ENOMEM;
	v_pf->const_literal_flag = 1;

	node_pf = scf_node_alloc(t->w, v_pf->type, v_pf);
	if (!node_pf)
		return -ENOMEM;

	ret = scf_node_add_child(create, node_pf);
	if (ret < 0)
		return ret;

	ret = scf_node_add_child(create, node1);
	if (ret < 0)
		return ret;
	create->parent   = parent;
	parent->nodes[1] = create;

	b = scf_block_alloc_cstr("multi_rets");
	if (!b)
		return -ENOMEM;

	ret = scf_node_add_child((scf_node_t*)b, node0);
	if (ret < 0)
		return ret;
	parent->nodes[0] = (scf_node_t*)b;
	b->node.parent   = parent;

	ret = _scf_expr_calculate_internal(ast, parent, d);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	return 0;
}

static int _scf_expr_calculate_internal(scf_ast_t* ast, scf_node_t* node, void* data)
{
	if (!node)
		return 0;

	if (SCF_FUNCTION == node->type)
		return __scf_op_semantic_call(ast, (scf_function_t*)node, data);

	if (0 == node->nb_nodes) {

		if (scf_type_is_var(node->type))
			_semantic_check_var_size(ast, node);

		scf_logd("node->type: %d, %p, %p\n", node->type, _scf_operand_get(node), node);
		assert(scf_type_is_var(node->type) || SCF_LABEL == node->type);
		return 0;
	}

	assert(scf_type_is_operator(node->type));
	assert(node->nb_nodes > 0);

	if (!node->op) {
		node->op = scf_find_base_operator_by_type(node->type);
		if (!node->op) {
			scf_loge("node %p, type: %d, w: %p\n", node, node->type, node->w);
			return -1;
		}
	}

	if (node->result) {
		scf_variable_free(node->result);
		node->result = NULL;
	}

	if (node->result_nodes) {
		scf_vector_clear(node->result_nodes, ( void (*)(void*) ) scf_node_free);
		scf_vector_free(node->result_nodes);
		node->result_nodes = NULL;
	}

	scf_operator_handler_t* h;
	scf_handler_data_t*     d    = data;
	scf_variable_t**	    pret = d->pret; // save d->pret, and reload it before return

	int i;

	if (SCF_OP_ASSOCIATIVITY_LEFT == node->op->associativity) {
		// left associativity, from 0 to N -1

		for (i = 0; i < node->nb_nodes; i++) {

			d->pret = &(node->nodes[i]->result);

			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
				scf_loge("\n");
				goto _error;
			}
		}

		h = scf_find_semantic_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			goto _error;
		}

		d->pret = &node->result;

		if (h->func(ast, node->nodes, node->nb_nodes, d) < 0)
			goto _error;

	} else {
		// right associativity, from N - 1 to 0

		for (i = node->nb_nodes - 1; i >= 0; i--) {

			d->pret = &(node->nodes[i]->result);

			if (_scf_expr_calculate_internal(ast, node->nodes[i], d) < 0) {
				scf_loge("\n");
				goto _error;
			}
		}

		h = scf_find_semantic_operator_handler(node->op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			goto _error;
		}

		d->pret = &node->result;

		if (h->func(ast, node->nodes, node->nb_nodes, d) < 0)
			goto _error;
	}

	d->pret = pret;
	return 0;

_error:
	d->pret = pret;
	return -1;
}

static int _scf_expr_calculate(scf_ast_t* ast, scf_expr_t* e, scf_variable_t** pret)
{
	assert(e);
	assert(e->nodes);

	scf_node_t* root = e->nodes[0];

	if (scf_type_is_var(root->type)) {

		scf_logd("root: %p var: %p\n", root, root->var);

		_semantic_check_var_size(ast, root);

		root->result = scf_variable_ref(root->var);

		if (pret)
			*pret = scf_variable_ref(root->var);
		return 0;
	}

	scf_handler_data_t d = {0};
	d.pret = &root->result;

	if (_scf_expr_calculate_internal(ast, root, &d) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (pret) {
		*pret = scf_variable_ref(root->result);
	}
	return 0;
}

static int _semantic_add_var(scf_node_t** pp, scf_ast_t* ast, scf_node_t* parent,
		scf_lex_word_t* w, int type, int const_, int nb_pointers_, scf_function_t* func_ptr_)
{
	scf_node_t*     node;
	scf_type_t*     t;
	scf_variable_t* v;

	t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, type);
	if (ret < 0)
		return ret;
	if (!t)
		return -ENOMEM;

	v = SCF_VAR_ALLOC_BY_TYPE(w, t, const_, nb_pointers_, func_ptr_);
	if (!v)
		return -ENOMEM;

	node = scf_node_alloc(v->w, v->type, v);
	if (!node) {
		scf_variable_free(v);
		return -ENOMEM;
	}

	if (parent) {
		int ret = scf_node_add_child(parent, node);
		if (ret < 0) {
			scf_node_free(node);
			scf_variable_free(v);
			return ret;
		}
	}

	*pp = node;
	return 0;
}

static int _scf_op_semantic_create(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(nb_nodes >= 1);

	scf_handler_data_t* d    = data;
	scf_variable_t**    pret = NULL;

	int ret;
	int i;

	scf_variable_t* v0;
	scf_variable_t* v1;
	scf_variable_t* v2;
	scf_vector_t*   argv;
	scf_type_t*     class;
	scf_type_t*     t;
	scf_node_t*     parent = nodes[0]->parent;
	scf_node_t*     ninit  = nodes[0];

	scf_function_t* fmalloc;
	scf_function_t* finit;
	scf_node_t*     nmalloc;
	scf_node_t*     nsize;
	scf_node_t*     nthis;
	scf_node_t*     nerr;

	v0 = _scf_operand_get(nodes[0]);
	assert(v0 && SCF_FUNCTION_PTR == v0->type);

	class = NULL;
	ret = scf_ast_find_type(&class, ast, v0->w->text->data);
	if (ret < 0)
		return ret;
	assert(class);

	fmalloc = NULL;
	ret = scf_ast_find_function(&fmalloc, ast, "scf__auto_malloc");
	if (ret < 0)
		return ret;
	if (!fmalloc) {
		scf_loge("\n");
		return -EINVAL;
	}

	argv = scf_vector_alloc();
	if (!argv)
		return -ENOMEM;

	ret = _semantic_add_var(&nthis, ast, NULL, v0->w, class->type, 0, 1, NULL);
	if (ret < 0) {
		scf_vector_free(argv);
		return ret;
	}

	ret = scf_vector_add(argv, nthis->var);
	if (ret < 0) {
		scf_vector_free(argv);
		scf_node_free  (nthis);
		return ret;
	}

	for (i = 1; i < nb_nodes; i++) {

		pret    = d->pret;
		d->pret = &(nodes[i]->result);
		ret     = _scf_expr_calculate_internal(ast, nodes[i], d);
		d->pret = pret;

		if (ret < 0) {
			scf_vector_free(argv);
			scf_node_free  (nthis);
			return ret;
		}

		ret = scf_vector_add(argv, _scf_operand_get(nodes[i]));
		if (ret < 0) {
			scf_vector_free(argv);
			scf_node_free  (nthis);
			return ret;
		}
	}

	ret = _semantic_find_proper_function(ast, class, "__init", argv, &finit);
	scf_vector_free(argv);

	if (ret < 0) {
		scf_loge("init function of class '%s' not found\n", v0->w->text->data);
		scf_node_free(nthis);
		return -1;
	}
	v0->func_ptr = finit;

	ret = _semantic_add_var(&nsize, ast, parent, v0->w, SCF_VAR_INT, 1, 0, NULL);
	if (ret < 0) {
		scf_node_free(nthis);
		return ret;
	}
	nsize->var->const_literal_flag = 1;
	nsize->var->data.i64           = class->size;

	ret = _semantic_add_var(&nmalloc, ast, parent, fmalloc->node.w, SCF_FUNCTION_PTR, 1, 1, fmalloc);
	if (ret < 0) {
		scf_node_free(nthis);
		return ret;
	}
	nmalloc->var->const_literal_flag = 1;

	ret = scf_node_add_child(parent, nthis);
	if (ret < 0) {
		scf_node_free(nthis);
		return ret;
	}

	for (i = parent->nb_nodes - 4; i >= 0; i--)
		parent->nodes[i + 3] = parent->nodes[i];
	parent->nodes[0] = nmalloc;
	parent->nodes[1] = nsize;
	parent->nodes[2] = ninit;
	parent->nodes[3] = nthis;

	assert(parent->nb_nodes - 3 == finit->argv->size);

	for (i = 0; i < finit->argv->size; i++) {
		v1 =        finit->argv->data[i];

		v2 = _scf_operand_get(parent->nodes[i + 3]);

		if (scf_variable_is_struct_pointer(v1))
			continue;

		if (scf_variable_same_type(v1, v2))
			continue;

		ret = _semantic_add_type_cast(ast, &parent->nodes[i + 3], v1, parent->nodes[i + 3]);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}


	if (v0->w)
		scf_lex_word_free(v0->w);
	v0->w = scf_lex_word_clone(v0->func_ptr->node.w);

	if (!parent->result_nodes) {

		parent->result_nodes = scf_vector_alloc();
		if (!parent->result_nodes) {
			scf_node_free(nthis);
			return -ENOMEM;
		}
	} else
		scf_vector_clear(parent->result_nodes, ( void (*)(void*) ) scf_node_free);

	if (scf_vector_add(parent->result_nodes, nthis) < 0) {
		scf_node_free(nthis);
		return ret;
	}

	ret = _semantic_add_var(&nerr, ast, NULL, parent->w, SCF_VAR_INT, 0, 0, NULL);
	if (ret < 0)
		return ret;

	if (scf_vector_add(parent->result_nodes, nerr) < 0) {
		scf_node_free(nerr);
		return ret;
	}

	nthis->op           = parent->op;
	nthis->split_parent = parent;
	nthis->split_flag   = 1;

	nerr->op           = parent->op;
	nerr->split_parent = parent;
	nerr->split_flag   = 1;

	*d->pret = scf_variable_ref(nthis->var);
	return 0;
}

static int _scf_op_semantic_pointer(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d  = data;
	scf_variable_t*     v0 = _scf_operand_get(nodes[0]);
	scf_variable_t*     v1 = _scf_operand_get(nodes[1]);

	assert(v0);
	assert(v1);
	assert(v0->type >= SCF_STRUCT);

	scf_type_t*	t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, v1->type);
	if (ret < 0)
		return ret;

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v1->const_flag, v1->nb_pointers, v1->func_ptr);
	if (!r)
		return -ENOMEM;

	int i;
	for (i = 0; i < v1->nb_dimentions; i++)
		scf_variable_add_array_dimention(r, v1->dimentions[i]);

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_array_index(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	scf_handler_data_t* d    = data;
	scf_variable_t**	pret = d->pret;

	d->pret = &(nodes[1]->result);
	int ret = _scf_expr_calculate_internal(ast, nodes[1], d);
	d->pret = pret;

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	if (scf_variable_is_struct_pointer(v0)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (!scf_variable_interger(v1)) {
		scf_loge("array index should be an interger\n");
		return -1;
	}

	int nb_pointers = 0;

	if (v0->nb_dimentions > 0) {
		if (v0->dimentions[0] < 0) {
			scf_loge("\n");
			return -1;
		}

		nb_pointers = v0->nb_pointers;

		if (scf_variable_const(v1)) {
			if (v1->data.i < 0) {
				scf_loge("error: index %d < 0\n", v1->data.i);
				return -1;
			}

			if (v1->data.i >= v0->dimentions[0]) {
				scf_loge("index %d >= size %d\n", v1->data.i, v0->dimentions[0]);
				return -1;
			}
		}
	} else if (0 == v0->nb_dimentions && v0->nb_pointers > 0) {
		nb_pointers = v0->nb_pointers - 1;
	} else {
		scf_loge("index out, v0: %s, v0->nb_dimentions: %d, v0->nb_pointers: %d\n",
				v0->w->text->data, v0->nb_dimentions, v0->nb_pointers);
		return -1;
	}

	scf_type_t*	t = NULL;
	ret = scf_ast_find_type_type(&t, ast, v0->type);
	if (ret < 0)
		return ret;

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, nb_pointers, v0->func_ptr);
	if (!r)
		return -ENOMEM;

	int i;
	for (i = 1; i < v0->nb_dimentions; i++)
		scf_variable_add_array_dimention(r, v0->dimentions[i]);

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_block(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (0 == nb_nodes)
		return 0;

	scf_handler_data_t* d = data;

	scf_block_t* b = (scf_block_t*)(nodes[0]->parent);

	scf_block_t* prev_block = ast->current_block;
	ast->current_block = b;

	int i = 0;
	while (i < nb_nodes) {
		scf_node_t*		node = nodes[i];
		scf_operator_t* op   = node->op;

		if (scf_type_is_var(node->type)) {
			i++;
			continue;
		}

		scf_variable_t** pret = d->pret;

		int ret;

		if (SCF_FUNCTION == node->type)
			ret = __scf_op_semantic_call(ast, (scf_function_t*)node, data);
		else {
			if (!op) {
				op = scf_find_base_operator_by_type(node->type);
				if (!op) {
					scf_loge("node->type: %d\n", node->type);
					return -1;
				}
			}

			scf_operator_handler_t* h = scf_find_semantic_operator_handler(op->type, -1, -1, -1);
			if (!h) {
				scf_loge("op->type: %d, va: %d, %d, %d\n", op->type, SCF_OP_VA_START, SCF_OP_VA_ARG, SCF_OP_VA_END);
				return -1;
			}

			d->pret = &node->result;
			ret = h->func(ast, node->nodes, node->nb_nodes, d);
		}
		d->pret = pret;

		if (ret < 0) {
			scf_loge("\n");
			ast->current_block = prev_block;
			return -1;
		}

		i++;
	}

	ast->current_block = prev_block;
	return 0;
}

static int _scf_op_semantic_error(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;
	scf_variable_t**    pret = NULL;

	scf_block_t* b = ast->current_block;

	while (b && SCF_FUNCTION  != b->node.type) {
		b = (scf_block_t*)b->node.parent;
	}

	SCF_CHECK_ERROR(!b, -1, "error statement must in a function\n");
	assert(SCF_FUNCTION == b->node.type);

	assert(nb_nodes >= 2);
	assert(nodes);

	int i;
	for (i = 0; i < nb_nodes; i++) {

		pret    = d->pret;
		d->pret = &(nodes[i]->result);
		int ret = _scf_expr_calculate_internal(ast, nodes[i], d);
		d->pret = pret;

		SCF_CHECK_ERROR(ret < 0, -1, "expr calculate error\n");
	}

	if (!scf_type_is_integer(nodes[0]->result->type)) {
		scf_loge("1st expr in error statement should got an interger for condition\n");
		return -1;
	}

	if (0 == nodes[1]->result->nb_pointers
			|| SCF_FUNCTION_PTR == nodes[1]->result->type) {
		scf_loge("2nd expr in error statement should got a pointer to be freed, but not a function pointer\n");
		return -1;
	}

	if (nb_nodes >= 3) {
		if (!scf_type_is_integer(nodes[2]->result->type)) {
			scf_loge("3rd expr in error statement should got an interger for return value\n");
			return -1;
		}
	} else if (nb_nodes < 3) {
		scf_logw("error in line: %d not set return value\n", nodes[0]->parent->w->line);
	}

	if (nb_nodes >= 4) {
		if (SCF_VAR_CHAR != nodes[3]->result->type
				|| 1 != nodes[3]->result->nb_pointers) {
			scf_loge("4th expr in error statement should got an format string\n");
			return -1;
		}
	}

	return 0;
}

static int _scf_op_semantic_return(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;

	scf_function_t* f = (scf_function_t*) ast->current_block;

	while (f && SCF_FUNCTION  != f->node.type)
		f = (scf_function_t*) f->node.parent;

	if (!f) {
		scf_loge("\n");
		return -1;
	}

	if (nb_nodes > f->rets->size) {
		scf_loge("\n");
		return -1;
	}

	int i;
	for (i = 0; i < nb_nodes; i++) {
		assert(nodes);

		scf_variable_t* fret = f->rets->data[i];
		scf_expr_t*     e    = nodes[i];
		scf_variable_t* r    = NULL;

		if (SCF_VAR_VOID == fret->type && 0 == fret->nb_pointers) {
			scf_loge("void function needs no return value\n");
			return -1;
		}

		if (_scf_expr_calculate(ast, e, &r) < 0) {
			scf_loge("\n");
			return -1;
		}

		int same = scf_variable_same_type(r, fret);

		scf_variable_free(r);
		r = NULL;

		if (!same) {
			int ret = _semantic_add_type_cast(ast, &(e->nodes[0]), fret, e->nodes[0]);
			if (ret < 0) {
				scf_loge("\n");
				return ret;
			}
		}
	}

	return 0;
}

static int _scf_op_semantic_break(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;

	scf_node_t* n = (scf_node_t*)ast->current_block;

	while (n && (SCF_OP_WHILE != n->type && SCF_OP_FOR != n->type)) {
		n = n->parent;
	}

	if (!n) {
		scf_loge("\n");
		return -1;
	}
	assert(SCF_OP_WHILE == n->type || SCF_OP_FOR == n->type);

	if (!n->parent) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

static int _scf_op_semantic_continue(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	scf_handler_data_t* d = data;

	scf_node_t* n = (scf_node_t*)ast->current_block;

	while (n && (SCF_OP_WHILE != n->type && SCF_OP_FOR != n->type)) {
		n = n->parent;
	}

	if (!n) {
		scf_loge("\n");
		return -1;
	}
	assert(SCF_OP_WHILE == n->type || SCF_OP_FOR == n->type);
	return 0;
}

static int _scf_op_semantic_label(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return 0;
}

static int _scf_op_semantic_goto(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* nl = nodes[0];
	assert(SCF_LABEL == nl->type);

	scf_label_t* l = nl->label;
	assert(l->w);

	scf_label_t* l2 = scf_block_find_label(ast->current_block, l->w->text->data);
	if (!l2) {
		scf_loge("label '%s' not found\n", l->w->text->data);
		return -1;
	}

	return 0;
}

static int _scf_op_semantic_node(scf_ast_t* ast, scf_node_t* node, scf_handler_data_t* d)
{
	scf_operator_t* op = node->op;

	if (!op) {
		op = scf_find_base_operator_by_type(node->type);
		if (!op) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_operator_handler_t*	h = scf_find_semantic_operator_handler(op->type, -1, -1, -1);
	if (!h) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t** pret = d->pret;

	d->pret = &node->result;
	int ret = h->func(ast, node->nodes, node->nb_nodes, d);
	d->pret = pret;

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

static int _scf_op_semantic_if(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (nb_nodes < 2) {
		scf_loge("\n");
		return -1;
	}

	scf_handler_data_t* d = data;

	scf_expr_t* e = nodes[0];
	assert(SCF_OP_EXPR == e->type);

	scf_block_t* b = (scf_block_t*)(e->parent);

	scf_variable_t* r = NULL;
	if (_scf_expr_calculate(ast, e, &r) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (!r || !scf_variable_interger(r)) {
		scf_loge("\n");
		return -1;
	}
	scf_variable_free(r);
	r = NULL;

	int i;
	for (i = 1; i < nb_nodes; i++) {
		scf_node_t*		node = nodes[i];
		scf_operator_t* op   = node->op;

		if (node->w) {
			scf_logd("node: %p, w: %s, line:%d\n", node, node->w->text->data, node->w->line);
		}
		if (!op) {
			op = scf_find_base_operator_by_type(node->type);
			if (!op) {
				scf_loge("\n");
				return -1;
			}
		}

		scf_operator_handler_t* h = scf_find_semantic_operator_handler(op->type, -1, -1, -1);
		if (!h) {
			scf_loge("\n");
			return -1;
		}

		scf_variable_t** pret = d->pret;

		d->pret = &node->result;
		int ret = h->func(ast, node->nodes, node->nb_nodes, d);
		d->pret = pret;

		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	return 0;
}

static int _scf_op_semantic_while(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_expr_t* e = nodes[0];
	assert(SCF_OP_EXPR == e->type);

	scf_block_t* b = (scf_block_t*)(e->parent);

	scf_variable_t* r = NULL;
	if (_scf_expr_calculate(ast, e, &r) < 0) {
		scf_loge("\n");
		return -1;
	}

	if (!r || !scf_variable_interger(r)) {
		scf_loge("\n");
		return -1;
	}
	scf_variable_free(r);
	r = NULL;

	// while body
	scf_node_t*     node = nodes[1];
	scf_operator_t* op   = node->op;

	if (!op) {
		op = scf_find_base_operator_by_type(node->type);
		if (!op) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_operator_handler_t*	h = scf_find_semantic_operator_handler(op->type, -1, -1, -1);
	if (!h) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t** pret = d->pret;

	d->pret = &node->result;
	int ret = h->func(ast, node->nodes, node->nb_nodes, d);
	d->pret = pret;

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}


static int _scf_op_semantic_for(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(4 == nb_nodes);

	scf_handler_data_t* d = data;
	int ret = 0;

	if (nodes[0]) {
		ret = _scf_op_semantic_node(ast, nodes[0], d);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	scf_expr_t* e = nodes[1];
	if (e) {
		assert(SCF_OP_EXPR == e->type);

		scf_variable_t* r = NULL;

		if (_scf_expr_calculate(ast, e, &r) < 0) {
			scf_loge("\n");
			return -1;
		}

		if (!r || !scf_variable_interger(r)) {
			scf_loge("\n");
			return -1;
		}
		scf_variable_free(r);
		r = NULL;
	}

	int i;
	for (i = 2; i < nb_nodes; i++) {
		if (!nodes[i])
			continue;

		ret = _scf_op_semantic_node(ast, nodes[i], d);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}
	}

	return 0;
}

static int __scf_op_semantic_call(scf_ast_t* ast, scf_function_t* f, void* data)
{
	scf_logd("f: %p, f->node->w: %s\n", f, f->node.w->text->data);

	scf_handler_data_t* d = data;

	// save & change the current block
	scf_block_t* prev_block = ast->current_block;
	ast->current_block = (scf_block_t*)f;

	if (_scf_op_semantic_block(ast, f->node.nodes, f->node.nb_nodes, d) < 0) {
		scf_loge("\n");
		return -1;
	}

	ast->current_block = prev_block;
	return 0;
}

static int _scf_op_semantic_call(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(nb_nodes > 0);

	scf_handler_data_t* d      = data;
	scf_variable_t**    pret   = d->pret;
	scf_node_t*         parent = nodes[0]->parent;

	d->pret = &nodes[0]->result;
	int ret = _scf_expr_calculate_internal(ast, nodes[0], d);
	d->pret = pret;

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	if (SCF_FUNCTION_PTR != v0->type || !v0->func_ptr) {
		scf_loge("\n");
		return -1;
	}

	scf_function_t* f = v0->func_ptr;

	if (f->vargs_flag) {
		if (f->argv->size > nb_nodes - 1) {
			scf_loge("\n");
			return -1;
		}
	} else if (f->argv->size != nb_nodes - 1) {
		scf_loge("\n");
		return -1;
	}

	int i;
	for (i = 0; i < f->argv->size; i++) {
		scf_variable_t* v0 = f->argv->data[i];
		scf_variable_t* v1 = _scf_operand_get(nodes[i + 1]);

		if (SCF_VAR_VOID == v1->type && 0 == v1->nb_pointers) {
			scf_loge("void var should be a pointer\n");
			return -1;
		}

		if (scf_variable_type_like(v0, v1))
			continue;

		if (scf_type_cast_check(ast, v0, v1) < 0) {
			scf_loge("f: %s, arg var not same type, i: %d, line: %d\n",
					f->node.w->text->data, i, parent->debug_w->line);
			return -1;
		}

		ret = _semantic_add_type_cast(ast, &nodes[i + 1], v0, nodes[i + 1]);
		if (ret < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	return _semantic_add_call_rets(ast, parent, d, f);
}

static int _scf_op_semantic_expr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* n = nodes[0];
	if (n->result) {
		scf_variable_free(n->result);
		n->result = 0;
	}

	scf_variable_t** pret = d->pret;

	d->pret = &n->result;
	int ret = _scf_expr_calculate_internal(ast, n, d);
	d->pret = pret;

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	if (scf_type_is_var(n->type)) {
		assert(n->var);
		if (d->pret)
			*d->pret = scf_variable_ref(n->var);

	} else {
		if (n->result && d->pret)
			*d->pret = scf_variable_ref(n->result);
	}

	return 0;
}

static int _scf_op_semantic_neg(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	assert(v0);

	if (scf_variable_is_struct_pointer(v0)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (scf_variable_interger(v0) || scf_variable_float(v0)) {

		scf_type_t*	t = NULL;
		int ret = scf_ast_find_type_type(&t, ast, v0->type);
		if (ret < 0)
			return ret;

		scf_lex_word_t* w = nodes[0]->parent->w;
		scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers, v0->func_ptr);
		if (!r)
			return -ENOMEM;

		*d->pret = r;
		return 0;
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_semantic_inc(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);

	if (scf_variable_const(v0) || scf_variable_const_string(v0)) {
		scf_loge("line: %d\n", parent->w->line);
		return -1;
	}

	if (scf_variable_is_struct_pointer(v0)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (scf_variable_interger(v0)) {

		scf_type_t*	t = NULL;
		int ret = scf_ast_find_type_type(&t, ast, v0->type);
		if (ret < 0)
			return ret;

		scf_lex_word_t* w = nodes[0]->parent->w;
		scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers, v0->func_ptr);
		if (!r)
			return -ENOMEM;

		*d->pret = r;
		return 0;
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_semantic_inc_post(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_inc(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_dec(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_inc(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_dec_post(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_inc(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_positive(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_node_t* child  = nodes[0];
	scf_node_t* parent = nodes[0]->parent;

	nodes[0] = NULL;

	scf_node_free_data(parent);
	scf_node_move_data(parent, child);
	scf_node_free(child);

	return 0;
}

static int _scf_op_semantic_dereference(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	if (v0->nb_pointers <= 0) {
		scf_loge("var is not a pointer\n");
		return -EINVAL;
	}

	scf_type_t*	t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, v0->type);
	if (ret < 0)
		return ret;

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, 0, v0->nb_pointers - 1, v0->func_ptr);
	if (!r)
		return -ENOMEM;

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_address_of(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	if (v0->const_literal_flag) {
		scf_loge("\n");
		return -EINVAL;
	}

	scf_type_t*	t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, v0->type);
	if (ret < 0)
		return ret;

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers + 1, v0->func_ptr);
	if (!r)
		return -ENOMEM;

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_type_cast(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* dst = _scf_operand_get(nodes[0]);
	scf_variable_t* src = _scf_operand_get(nodes[1]);

	assert(dst);
	assert(src);

	if (d->pret) {
		*d->pret = scf_variable_ref(dst);
	}

	return 0;
}

static int _scf_op_semantic_container(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(3 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_node_t*     e      = nodes[0];
	scf_node_t*     node1  = nodes[1];
	scf_node_t*     node2  = nodes[2];
	scf_node_t*     parent = e->parent;

	scf_variable_t* v0     = _scf_operand_get(e);
	scf_variable_t* v1     = _scf_operand_get(node1);
	scf_variable_t* v2     = _scf_operand_get(node2);
	scf_variable_t* u8;
	scf_variable_t* offset;
	scf_type_t*     t;

	assert(v0);
	assert(SCF_OP_EXPR == e->type);
	assert(d->pret == &parent->result);

	assert(v0->nb_pointers > 0);
	assert(v1->nb_pointers > 0);
	assert(v1->type >= SCF_STRUCT);
	assert(v2->member_flag);

	parent->type     = SCF_OP_TYPE_CAST;
	parent->result   = scf_variable_ref(v1);
	parent->nodes[0] = node1;
	parent->nodes[1] = e;
	parent->nodes[2] = NULL;
	parent->nb_nodes = 2;
	parent->op       = scf_find_base_operator_by_type(SCF_OP_TYPE_CAST);

	if (0 == v2->offset) {
		scf_node_free(node2);
		return 0;
	}

	t  = scf_block_find_type_type(ast->current_block, SCF_VAR_U8);
	u8 = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 0, 1, NULL);

	int ret = _semantic_add_type_cast(ast, &e->nodes[0], u8, e->nodes[0]);
	if (ret < 0)
		return ret;

	t      = scf_block_find_type_type(ast->current_block, SCF_VAR_UINTPTR);
	offset = SCF_VAR_ALLOC_BY_TYPE(v2->w, t, 1, 0, NULL);

	offset->data.u64 = v2->offset;

	assert(!node2->result);

	scf_variable_free(node2->var);
	scf_variable_free(e->result);

	node2->type = offset->type;
	node2->var  = offset;

	scf_node_add_child(e, node2);

	e->type     = SCF_OP_SUB;
	e->result   = u8;
	e->op       = scf_find_base_operator_by_type(SCF_OP_SUB);

	return 0;
}

static int _scf_op_semantic_sizeof(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);
	assert(SCF_OP_EXPR == nodes[0]->type);
	assert(d->pret == &parent->result);

	int size = scf_variable_size(v0);
	if (size < 0)
		return size;

	scf_type_t*     t = scf_block_find_type_type(ast->current_block, SCF_VAR_INTPTR);

	scf_lex_word_t* w = parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, 1, 0, NULL);
	if (!r)
		return -ENOMEM;

	r->data.i64   = size;
	r->const_flag = 1;

	SCF_XCHG(r->w, parent->w);

	scf_node_free_data(parent);
	parent->type = r->type;
	parent->var  = r;

	return 0;
}

static int _scf_op_semantic_logic_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);

	assert(v0);

	if (scf_variable_is_struct_pointer(v0)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (scf_variable_interger(v0)) {

		int const_flag    = v0->const_flag && 0 == v0->nb_pointers && 0 == v0->nb_dimentions;

		scf_type_t*	    t = scf_block_find_type_type(ast->current_block, SCF_VAR_INT);

		scf_lex_word_t* w = nodes[0]->parent->w;
		scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag, 0, NULL);
		if (!r)
			return -ENOMEM;

		*d->pret = r;
		return 0;
	}

	scf_loge("v0: %d/%s\n", v0->w->line, v0->w->text->data);
	return -1;
}

static int _scf_op_semantic_bit_not(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(1 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	assert(v0);

	if (scf_variable_is_struct_pointer(v0)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	scf_type_t* t;

	if (v0->nb_pointers + v0->nb_dimentions > 0) {
		t = scf_block_find_type_type(ast->current_block, SCF_VAR_UINTPTR);

	} else if (scf_type_is_integer(v0->type)) {
		t = NULL;
		int ret = scf_ast_find_type_type(&t, ast, v0->type);
		if (ret < 0)
			return ret;
	} else {
		scf_loge("\n");
		return -1;
	}

	scf_lex_word_t* w = nodes[0]->parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, 0, NULL);
	if (!r)
		return -ENOMEM;

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_binary(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);
	assert(v1);

	if (scf_variable_is_struct_pointer(v0) || scf_variable_is_struct_pointer(v1)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (scf_variable_interger(v0) || scf_variable_float(v0)) {

		if (scf_variable_interger(v1) || scf_variable_float(v1)) {

			scf_function_t* func_ptr     = NULL;
			scf_variable_t* v2           = NULL;
			scf_type_t*     t            = NULL;

			int             const_flag   = 0;
			int             nb_pointers  = 0;
			int             nb_pointers0 = scf_variable_nb_pointers(v0);
			int             nb_pointers1 = scf_variable_nb_pointers(v1);

			if (nb_pointers0 > 0) {

				if (nb_pointers1 > 0) {

					if (!scf_variable_same_type(v0, v1)) {
						scf_loge("different type pointer, type: %d,%d, nb_pointers: %d,%d\n",
								v0->type, v1->type, nb_pointers0, nb_pointers1);
						return -EINVAL;
					}

				} else if (!scf_variable_interger(v1)) {
					scf_loge("var calculated with a pointer should be a interger\n");
					return -EINVAL;
				} else {
					t  = scf_block_find_type_type(ast->current_block, SCF_VAR_UINTPTR);

					v2 = SCF_VAR_ALLOC_BY_TYPE(v1->w, t, v1->const_flag, 0, NULL);

					int ret = _semantic_add_type_cast(ast, &nodes[1], v2, nodes[1]);

					scf_variable_free(v2);
					v2  = NULL;
					if (ret < 0) {
						scf_loge("add type cast failed\n");
						return ret;
					}
				}

				t = NULL;
				int ret = scf_ast_find_type_type(&t, ast, v0->type);
				if (ret < 0)
					return ret;

				const_flag  = v0->const_flag;
				nb_pointers = nb_pointers0;
				func_ptr    = v0->func_ptr;

			} else if (nb_pointers1 > 0) {

				if (!scf_variable_interger(v0)) {
					scf_loge("var calculated with a pointer should be a interger\n");
					return -EINVAL;
				} else {
					t  = scf_block_find_type_type(ast->current_block, SCF_VAR_UINTPTR);

					v2 = SCF_VAR_ALLOC_BY_TYPE(v0->w, t, v0->const_flag, 0, NULL);

					int ret = _semantic_add_type_cast(ast, &nodes[0], v2, nodes[0]);

					scf_variable_free(v2);
					v2  = NULL;
					if (ret < 0) {
						scf_loge("add type cast failed\n");
						return ret;
					}
				}

				t = NULL;
				int ret = scf_ast_find_type_type(&t, ast, v1->type);
				if (ret < 0)
					return ret;

				const_flag  = v1->const_flag;
				nb_pointers = nb_pointers1;
				func_ptr    = v1->func_ptr;

			} else if (v0->type == v1->type) { // from here v0 & v1 are normal var
				t = NULL;
				int ret = scf_ast_find_type_type(&t, ast, v0->type);
				if (ret < 0)
					return ret;

				const_flag  = v0->const_flag && v1->const_flag;
				nb_pointers = 0;
				func_ptr    = NULL;

			} else {
				int ret = scf_find_updated_type(ast, v0, v1);
				if (ret < 0) {
					scf_loge("var type update failed, type: %d, %d\n", v0->type, v1->type);
					return -EINVAL;
				}

				t = NULL;
				ret = scf_ast_find_type_type(&t, ast, ret);
				if (ret < 0)
					return ret;

				if (t->type != v0->type)
					ret = _semantic_add_type_cast(ast, &nodes[0], v1, nodes[0]);
				else
					ret = _semantic_add_type_cast(ast, &nodes[1], v0, nodes[1]);

				if (ret < 0) {
					scf_loge("add type cast failed\n");
					return ret;
				}

				const_flag  = v0->const_flag && v1->const_flag;
				nb_pointers = 0;
				func_ptr    = NULL;
			}

			scf_lex_word_t* w = nodes[0]->parent->w;
			scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag, nb_pointers, func_ptr);
			if (!r) {
				scf_loge("var alloc failed\n");
				return -ENOMEM;
			}

			*d->pret = r;
			return 0;
		}
	}

	scf_loge("type %d, %d not support\n", v0->type, v1->type);
	return -1;
}

static int _scf_op_semantic_add(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_sub(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_mul(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_div(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_binary_interger(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data, int result_type)
{
	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;
	scf_lex_word_t* w      = parent->w;

	scf_type_t*     t;
	scf_variable_t* r;

	assert(v0);
	assert(v1);

	if (scf_variable_is_struct_pointer(v0) || scf_variable_is_struct_pointer(v1)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (scf_variable_interger(v0) && scf_variable_interger(v1)) {

		int const_flag = v0->const_flag && v1->const_flag;

		if (!scf_variable_same_type(v0, v1)) {

			int ret = _semantic_do_type_cast(ast, nodes, nb_nodes, data);
			if (ret < 0) {
				scf_loge("semantic do type cast failed, line: %d\n", parent->w->line);
				return ret;
			}
		}

		v0 = _scf_operand_get(nodes[0]);

		t = NULL;
		int ret = scf_ast_find_type_type(&t, ast, v0->type);
		if (ret < 0)
			return ret;

		r = SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag, v0->nb_pointers, v0->func_ptr);
		if (!r) {
			scf_loge("var alloc failed\n");
			return -ENOMEM;
		}

		*d->pret = r;
		return 0;
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_semantic_mod(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_shl(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_shr(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_bit_and(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_bit_or(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger(ast, nodes, nb_nodes, data, -1);
}

static int _semantic_multi_rets_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t*     parent = nodes[0]->parent;
	scf_node_t*     gp     = parent->parent;

	assert(SCF_OP_BLOCK == nodes[0]->type);

	while (SCF_OP_EXPR == gp->type)
		gp = gp->parent;
#if 1
	if (gp->type != SCF_OP_BLOCK && gp->type != SCF_FUNCTION) {
		scf_loge("a call to multi-return-values function MUST be in block, line: %d, gp->type: %d, b: %d, f: %d\n",
				parent->w->line, gp->type, SCF_OP_BLOCK, SCF_FUNCTION);
		return -1;
	}
#endif

	scf_node_t* rets = nodes[0];
	scf_node_t* call = nodes[1];
	scf_node_t* ret;

	while (call) {
		if (SCF_OP_EXPR == call->type)
			call = call->nodes[0];
		else
			break;
	}

	if (SCF_OP_CALL != call->type && SCF_OP_CREATE != call->type) {
		scf_loge("\n");
		return -1;
	}

	assert(call->nb_nodes > 0);

	scf_loge("rets->nb_nodes: %d, call->result_nodes: %p\n", rets->nb_nodes, call->result_nodes);
	scf_loge("rets->nb_nodes: %d, call->result_nodes->size: %d\n", rets->nb_nodes, call->result_nodes->size);

	assert(rets->nb_nodes <= call->result_nodes->size);

	int i;
	for (i  = 0; i < rets->nb_nodes; i++) {

		scf_variable_t* v0 = _scf_operand_get(rets->nodes[i]);
		scf_variable_t* v1 = _scf_operand_get(call->result_nodes->data[i]);

		if (!scf_variable_same_type(v0, v1)) {
			scf_loge("\n");
			return -1;
		}

		if (v0->const_flag) {
			scf_loge("\n");
			return -1;
		}

		scf_node_t* assign = scf_node_alloc(parent->w, SCF_OP_ASSIGN, NULL);
		if (!assign)
			return -ENOMEM;

		scf_node_add_child(assign, rets->nodes[i]);
		scf_node_add_child(assign, call->result_nodes->data[i]);

		rets->nodes[i] = assign;
	}

	scf_node_add_child(rets, nodes[1]);

	for (i = rets->nb_nodes - 2; i >= 0; i--)
		rets->nodes[i + 1] = rets->nodes[i];
	rets->nodes[0] =  nodes[1];

	parent->type     = SCF_OP_EXPR;
	parent->nb_nodes = 1;
	parent->nodes[0] = rets;

	return 0;
}

static int _scf_op_semantic_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_node_t* parent = nodes[0]->parent;
	scf_node_t* call   = nodes[1];

	while (call) {
		if (SCF_OP_EXPR == call->type)
			call = call->nodes[0];
		else
			break;
	}

	if (SCF_OP_CALL == call->type
			&& call->result_nodes->size > 1) {

		if (SCF_OP_BLOCK != nodes[0]->type) {

			scf_block_t* b = scf_block_alloc_cstr("multi_rets");
			if (!b)
				return -ENOMEM;

			int ret = scf_node_add_child((scf_node_t*)b, nodes[0]);
			if (ret < 0) {
				scf_block_free(b);
				return ret;
			}
			parent->nodes[0] = (scf_node_t*)b;
			b->node.parent   = parent;
		}
	}

	if (SCF_OP_BLOCK == nodes[0]->type) {

		return _semantic_multi_rets_assign(ast, nodes, nb_nodes, data);
	}

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);

	assert(v0);
	assert(v1);

	if (SCF_VAR_VOID == v0->type && 0 == v0->nb_pointers) {
		scf_loge("var '%s' can't be 'void' type\n", v0->w->text->data);
		return -1;
	}

	if (SCF_VAR_VOID == v1->type && 0 == v1->nb_pointers) {
		scf_loge("var '%s' can't be 'void' type\n", v1->w->text->data);
		return -1;
	}

	if (v0->const_literal_flag || v0->nb_dimentions > 0) {

		scf_loge("const var '%s' can't be assigned\n", v0->w->text->data);
		return -1;

	} else if (v0->const_flag) {
		scf_logw("const var '%s' can't be assigned\n", v0->w->text->data);
	}

	if (scf_variable_is_struct(v0) || scf_variable_is_struct(v1)) {

		int size = scf_variable_size(v0);

		int ret = _semantic_do_overloaded_assign(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error, ret: %d\n", ret);
			return -1;
		}

		if (scf_variable_same_type(v0, v1)) {

			scf_function_t* f = NULL;
			ret = scf_ast_find_function(&f, ast, "scf__memcpy");
			if (ret < 0)
				return ret;

			if (!f) {
				scf_loge("semantic do overloaded error: default 'scf__memcpy' NOT found\n");
				return -1;
			}

			scf_type_t*     t = scf_block_find_type_type(ast->current_block, SCF_VAR_INTPTR);
			scf_variable_t* v = SCF_VAR_ALLOC_BY_TYPE(NULL, t, 1, 0, NULL);
			if (!v) {
				scf_loge("var alloc failed\n");
				return -ENOMEM;
			}
			v->data.i64 = size;

			scf_node_t* node_size = scf_node_alloc(NULL, v->type, v);
			if (!node_size) {
				scf_loge("node alloc failed\n");
				return -ENOMEM;
			}

			scf_node_add_child(parent, node_size);

			return _semantic_add_call(ast, parent->nodes, parent->nb_nodes, d, f);
		}

		scf_loge("semantic do overloaded error\n");
		return -1;
	}

	if (!scf_variable_same_type(v0, v1)) {

		if (scf_variable_is_struct_pointer(v0)) {

			scf_type_t*	t = NULL;
			int ret = scf_ast_find_type_type(&t, ast, v0->type);
			if (ret < 0)
				return ret;
			assert(t);

			if (scf_scope_find_function(t->scope, "__init")) {

				int ret = _semantic_do_create(ast, nodes, nb_nodes, d);
				if (0 == ret)
					return 0;

				if (-404 != ret) {
					scf_loge("semantic do overloaded error, ret: %d\n", ret);
					return -1;
				}
			}
		}

		scf_logd("v0: v_%d_%d/%s\n", v0->w->line, v0->w->pos, v0->w->text->data);

		if (scf_type_cast_check(ast, v0, v1) < 0) {
			scf_loge("\n");
			return -1;
		}

		int ret = _semantic_add_type_cast(ast, &nodes[1], v0, nodes[1]);
		if (ret < 0) {
			scf_loge("add type cast failed\n");
			return ret;
		}
	}

	scf_type_t*	t = NULL;
	int ret = scf_ast_find_type_type(&t, ast, v0->type);
	if (ret < 0)
		return ret;

	scf_lex_word_t* w = parent->w;
	scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers, v0->func_ptr);
	if (!r) {
		scf_loge("var alloc failed\n");
		return -1;
	}

	*d->pret = r;
	return 0;
}

static int _scf_op_semantic_binary_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;

	assert(v0);
	assert(v1);

	if (v0->const_flag || v0->nb_dimentions > 0) {
		scf_loge("const var '%s' can't be assigned\n", v0->w->text->data);
		return -1;
	}

	if (scf_variable_is_struct_pointer(v0) || scf_variable_is_struct_pointer(v1)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (scf_variable_interger(v0) || scf_variable_float(v0)) {

		if (scf_variable_interger(v1) || scf_variable_float(v1)) {

			if (!scf_variable_same_type(v0, v1)) {

				if (scf_type_cast_check(ast, v0, v1) < 0) {
					scf_loge("\n");
					return -1;
				}

				int ret = _semantic_add_type_cast(ast, &nodes[1], v0, nodes[1]);
				if (ret < 0) {
					scf_loge("add type cast failed\n");
					return ret;
				}
			}

			scf_type_t*	t = NULL;
			int ret = scf_ast_find_type_type(&t, ast, v0->type);
			if (ret < 0)
				return ret;
			assert(t);

			scf_lex_word_t* w = nodes[0]->parent->w;
			scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers, v0->func_ptr);
			if (!r) {
				scf_loge("var alloc failed\n");
				return -ENOMEM;
			}

			*d->pret = r;
		}
	} else {
		scf_loge("type %d, %d not support\n", v0->type, v1->type);
		return -1;
	}

	return 0;
}

static int _scf_op_semantic_binary_interger_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data, int result_type)
{
	scf_handler_data_t* d  = data;

	scf_variable_t* v0     = _scf_operand_get(nodes[0]);
	scf_variable_t* v1     = _scf_operand_get(nodes[1]);
	scf_node_t*     parent = nodes[0]->parent;
	scf_lex_word_t* w      = parent->w;

	scf_type_t*     t;
	scf_variable_t* r;

	assert(v0);
	assert(v1);

	if (v0->const_flag || v0->nb_dimentions > 0) {
		scf_loge("const var '%s' can't be assigned\n", v0->w->text->data);
		return -1;
	}

	if (scf_variable_is_struct_pointer(v0) || scf_variable_is_struct_pointer(v1)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (scf_variable_interger(v0) && scf_variable_interger(v1)) {

		if (!scf_variable_same_type(v0, v1)) {

			if (scf_type_cast_check(ast, v0, v1) < 0) {
				scf_loge("\n");
				return -1;
			}

			int ret = _semantic_add_type_cast(ast, &nodes[1], v0, nodes[1]);
			if (ret < 0) {
				scf_loge("add type cast failed\n");
				return ret;
			}
		}

		t = NULL;
		int ret = scf_ast_find_type_type(&t, ast, v0->type);
		if (ret < 0)
			return ret;
		assert(t);

		r = SCF_VAR_ALLOC_BY_TYPE(w, t, v0->const_flag, v0->nb_pointers, v0->func_ptr);
		if (!r) {
			scf_loge("var alloc failed\n");
			return -ENOMEM;
		}

		*d->pret = r;
		return 0;
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_semantic_add_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_assign(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_sub_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_assign(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_mul_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_assign(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_div_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_assign(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_mod_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger_assign(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_shl_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger_assign(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_shr_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger_assign(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_and_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger_assign(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_or_assign(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger_assign(ast, nodes, nb_nodes, data, -1);
}

static int _scf_op_semantic_cmp(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	assert(2 == nb_nodes);

	scf_handler_data_t* d = data;

	scf_variable_t* v0 = _scf_operand_get(nodes[0]);
	scf_variable_t* v1 = _scf_operand_get(nodes[1]);

	assert(v0);
	assert(v1);

	if (scf_variable_is_struct_pointer(v0) || scf_variable_is_struct_pointer(v1)) {

		int ret = _semantic_do_overloaded(ast, nodes, nb_nodes, d);
		if (0 == ret)
			return 0;

		if (-404 != ret) {
			scf_loge("semantic do overloaded error\n");
			return -1;
		}
	}

	if (scf_variable_interger(v0) || scf_variable_float(v0)) {

		if (scf_variable_interger(v1) || scf_variable_float(v1)) {

			int const_flag = v0->const_flag && v1->const_flag; 

			if (!scf_variable_same_type(v0, v1)) {

				int ret = _semantic_do_type_cast(ast, nodes, nb_nodes, data);
				if (ret < 0) {
					scf_loge("semantic do type cast failed\n");
					return ret;
				}
			}
	
			v0 = _scf_operand_get(nodes[0]);

			scf_type_t*     t = scf_block_find_type_type(ast->current_block, SCF_VAR_INT);

			scf_lex_word_t* w = nodes[0]->parent->w;
			scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(w, t, const_flag, 0, NULL);
			if (!r) {
				scf_loge("var alloc failed\n");
				return -ENOMEM;
			}

			*d->pret = r;
			return 0;
		}
	}

	scf_loge("\n");
	return -1;
}

static int _scf_op_semantic_eq(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
#define CMPEQ_CHECK_FLOAT() \
	do { \
		assert(2 == nb_nodes); \
		scf_variable_t* v0 = _scf_operand_get(nodes[0]); \
		scf_variable_t* v1 = _scf_operand_get(nodes[1]); \
		\
		if (scf_variable_float(v0) || scf_variable_float(v1)) { \
			scf_loge("float type can't cmp equal\n"); \
			return -EINVAL; \
		} \
	} while (0)

	CMPEQ_CHECK_FLOAT();

	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_ne(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	CMPEQ_CHECK_FLOAT();

	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_gt(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_ge(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	CMPEQ_CHECK_FLOAT();

	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_lt(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_le(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	CMPEQ_CHECK_FLOAT();

	return _scf_op_semantic_cmp(ast, nodes, nb_nodes, data);
}

static int _scf_op_semantic_logic_and(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger(ast, nodes, nb_nodes, data, SCF_VAR_INT);
}

static int _scf_op_semantic_logic_or(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	return _scf_op_semantic_binary_interger(ast, nodes, nb_nodes, data, SCF_VAR_INT);
}

static int _scf_op_semantic_va_start(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (2 != nb_nodes) {
		scf_loge("\n");
		return -1;
	}
	return 0;
}

static int _scf_op_semantic_va_arg(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (2 != nb_nodes) {
		scf_loge("\n");
		return -1;
	}

	scf_handler_data_t* d = data;

	if (d->pret) {
		scf_variable_t* v = _scf_operand_get(nodes[1]);

		scf_type_t* t = NULL;
		int ret = scf_ast_find_type_type(&t, ast, v->type);
		if (ret < 0)
			return ret;
		assert(t);

		scf_variable_t* r = SCF_VAR_ALLOC_BY_TYPE(nodes[0]->parent->w, t, 0, v->nb_pointers, v->func_ptr);

		if (!r)
			return -ENOMEM;

		*d->pret = r;
	}
	return 0;
}

static int _scf_op_semantic_va_end(scf_ast_t* ast, scf_node_t** nodes, int nb_nodes, void* data)
{
	if (1 != nb_nodes) {
		scf_loge("\n");
		return -1;
	}
	return 0;
}

scf_operator_handler_t semantic_operator_handlers[] = {
	{{NULL, NULL}, SCF_OP_EXPR,           -1,   -1, -1, _scf_op_semantic_expr},
	{{NULL, NULL}, SCF_OP_CALL,           -1,   -1, -1, _scf_op_semantic_call},

	{{NULL, NULL}, SCF_OP_ARRAY_INDEX,    -1,   -1, -1, _scf_op_semantic_array_index},
	{{NULL, NULL}, SCF_OP_POINTER,        -1,   -1, -1, _scf_op_semantic_pointer},
	{{NULL, NULL}, SCF_OP_CREATE,         -1,   -1, -1, _scf_op_semantic_create},

	{{NULL, NULL}, SCF_OP_VA_START,       -1,   -1, -1, _scf_op_semantic_va_start},
	{{NULL, NULL}, SCF_OP_VA_ARG,         -1,   -1, -1, _scf_op_semantic_va_arg},
	{{NULL, NULL}, SCF_OP_VA_END,         -1,   -1, -1, _scf_op_semantic_va_end},

	{{NULL, NULL}, SCF_OP_CONTAINER,      -1,   -1, -1, _scf_op_semantic_container},

	{{NULL, NULL}, SCF_OP_SIZEOF,         -1,   -1, -1, _scf_op_semantic_sizeof},
	{{NULL, NULL}, SCF_OP_TYPE_CAST,      -1,   -1, -1, _scf_op_semantic_type_cast},
	{{NULL, NULL}, SCF_OP_LOGIC_NOT,      -1,   -1, -1, _scf_op_semantic_logic_not},
	{{NULL, NULL}, SCF_OP_BIT_NOT,        -1,   -1, -1, _scf_op_semantic_bit_not},
	{{NULL, NULL}, SCF_OP_NEG,            -1,   -1, -1, _scf_op_semantic_neg},
	{{NULL, NULL}, SCF_OP_POSITIVE,       -1,   -1, -1, _scf_op_semantic_positive},

	{{NULL, NULL}, SCF_OP_INC,            -1,   -1, -1, _scf_op_semantic_inc},
	{{NULL, NULL}, SCF_OP_DEC,            -1,   -1, -1, _scf_op_semantic_dec},

	{{NULL, NULL}, SCF_OP_INC_POST,       -1,   -1, -1, _scf_op_semantic_inc_post},
	{{NULL, NULL}, SCF_OP_DEC_POST,       -1,   -1, -1, _scf_op_semantic_dec_post},

	{{NULL, NULL}, SCF_OP_DEREFERENCE,    -1,   -1, -1, _scf_op_semantic_dereference},
	{{NULL, NULL}, SCF_OP_ADDRESS_OF,     -1,   -1, -1, _scf_op_semantic_address_of},

	{{NULL, NULL}, SCF_OP_MUL,            -1,   -1, -1, _scf_op_semantic_mul},
	{{NULL, NULL}, SCF_OP_DIV,            -1,   -1, -1, _scf_op_semantic_div},
	{{NULL, NULL}, SCF_OP_MOD,            -1,   -1, -1, _scf_op_semantic_mod},

	{{NULL, NULL}, SCF_OP_ADD,            -1,   -1, -1, _scf_op_semantic_add},
	{{NULL, NULL}, SCF_OP_SUB,            -1,   -1, -1, _scf_op_semantic_sub},

	{{NULL, NULL}, SCF_OP_SHL,            -1,   -1, -1, _scf_op_semantic_shl},
	{{NULL, NULL}, SCF_OP_SHR,            -1,   -1, -1, _scf_op_semantic_shr},

	{{NULL, NULL}, SCF_OP_BIT_AND,        -1,   -1, -1, _scf_op_semantic_bit_and},
	{{NULL, NULL}, SCF_OP_BIT_OR,         -1,   -1, -1, _scf_op_semantic_bit_or},

	{{NULL, NULL}, SCF_OP_EQ,             -1,   -1, -1, _scf_op_semantic_eq},
	{{NULL, NULL}, SCF_OP_NE,             -1,   -1, -1, _scf_op_semantic_ne},
	{{NULL, NULL}, SCF_OP_GT,             -1,   -1, -1, _scf_op_semantic_gt},
	{{NULL, NULL}, SCF_OP_LT,             -1,   -1, -1, _scf_op_semantic_lt},
	{{NULL, NULL}, SCF_OP_GE,             -1,   -1, -1, _scf_op_semantic_ge},
	{{NULL, NULL}, SCF_OP_LE,             -1,   -1, -1, _scf_op_semantic_le},

	{{NULL, NULL}, SCF_OP_LOGIC_AND,      -1,   -1, -1, _scf_op_semantic_logic_and},
	{{NULL, NULL}, SCF_OP_LOGIC_OR,       -1,   -1, -1, _scf_op_semantic_logic_or},

	{{NULL, NULL}, SCF_OP_ASSIGN,		  -1,   -1, -1, _scf_op_semantic_assign},
	{{NULL, NULL}, SCF_OP_ADD_ASSIGN,     -1,   -1, -1, _scf_op_semantic_add_assign},
	{{NULL, NULL}, SCF_OP_SUB_ASSIGN,     -1,   -1, -1, _scf_op_semantic_sub_assign},
	{{NULL, NULL}, SCF_OP_MUL_ASSIGN,     -1,   -1, -1, _scf_op_semantic_mul_assign},
	{{NULL, NULL}, SCF_OP_DIV_ASSIGN,     -1,   -1, -1, _scf_op_semantic_div_assign},
	{{NULL, NULL}, SCF_OP_MOD_ASSIGN,     -1,   -1, -1, _scf_op_semantic_mod_assign},
	{{NULL, NULL}, SCF_OP_SHL_ASSIGN,     -1,   -1, -1, _scf_op_semantic_shl_assign},
	{{NULL, NULL}, SCF_OP_SHR_ASSIGN,     -1,   -1, -1, _scf_op_semantic_shr_assign},
	{{NULL, NULL}, SCF_OP_AND_ASSIGN,     -1,   -1, -1, _scf_op_semantic_and_assign},
	{{NULL, NULL}, SCF_OP_OR_ASSIGN,      -1,   -1, -1, _scf_op_semantic_or_assign},

	{{NULL, NULL}, SCF_OP_BLOCK,		  -1,   -1, -1, _scf_op_semantic_block},
	{{NULL, NULL}, SCF_OP_RETURN,		  -1,   -1, -1, _scf_op_semantic_return},
	{{NULL, NULL}, SCF_OP_BREAK,		  -1,   -1, -1, _scf_op_semantic_break},
	{{NULL, NULL}, SCF_OP_CONTINUE,		  -1,   -1, -1, _scf_op_semantic_continue},
	{{NULL, NULL}, SCF_OP_GOTO,			  -1,   -1, -1, _scf_op_semantic_goto},
	{{NULL, NULL}, SCF_LABEL,			  -1,   -1, -1, _scf_op_semantic_label},
	{{NULL, NULL}, SCF_OP_ERROR,          -1,   -1, -1, _scf_op_semantic_error},

	{{NULL, NULL}, SCF_OP_IF,			  -1,   -1, -1, _scf_op_semantic_if},
	{{NULL, NULL}, SCF_OP_WHILE,		  -1,   -1, -1, _scf_op_semantic_while},
	{{NULL, NULL}, SCF_OP_FOR,            -1,   -1, -1, _scf_op_semantic_for},
};

scf_operator_handler_t* scf_find_semantic_operator_handler(const int type, const int src0_type, const int src1_type, const int ret_type)
{
	int i;
	for (i = 0; i < sizeof(semantic_operator_handlers) / sizeof(semantic_operator_handlers[0]); i++) {

		scf_operator_handler_t* h = &semantic_operator_handlers[i];

		if (type == h->type)
			return h;
	}

	return NULL;
}

int scf_function_semantic_analysis(scf_ast_t* ast, scf_function_t* f)
{
	scf_handler_data_t d = {0};

	int ret = __scf_op_semantic_call(ast, f, &d);

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

int scf_expr_semantic_analysis(scf_ast_t* ast, scf_expr_t* e)
{
	scf_handler_data_t d = {0};

	if (!e->nodes || e->nb_nodes != 1) {
		scf_loge("\n");
		return -1;
	}

	int ret = _scf_expr_calculate_internal(ast, e->nodes[0], &d);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

int scf_semantic_analysis(scf_ast_t* ast)
{
	scf_handler_data_t d = {0};

	int ret = _scf_expr_calculate_internal(ast, (scf_node_t*)ast->root_block, &d);

	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}

