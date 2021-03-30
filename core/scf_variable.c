#include"scf_variable.h"
#include"scf_type.h"
#include"scf_function.h"

scf_variable_t*	scf_variable_alloc(scf_lex_word_t* w, scf_type_t* t)
{
	scf_variable_t* var = calloc(1, sizeof(scf_variable_t));
	assert(var);

	var->refs = 1;
	var->type = t->type;

	var->const_flag  = t->node.const_flag;
	var->nb_pointers = t->nb_pointers;
	var->func_ptr    = t->func_ptr;

	if (var->nb_pointers > 0)
		var->size = sizeof(void*);
	else
		var->size = t->size;

	if (var->nb_pointers > 1)
		var->data_size = sizeof(void*);
	else
		var->data_size = t->size;

	var->offset = t->offset;

	if (w) {
		var->w = scf_lex_word_clone(w);

		if (scf_lex_is_const(w)) {
			var->const_flag         = 1;
			var->const_literal_flag = 1;

			switch (w->type) {
				case SCF_LEX_WORD_CONST_CHAR:
					var->data.u32 = w->data.u32;
					break;
				case SCF_LEX_WORD_CONST_STRING:
					var->data.s = scf_string_clone(w->data.s);
					break;

				case SCF_LEX_WORD_CONST_INT:
					var->data.i = w->data.i;
					break;
				case SCF_LEX_WORD_CONST_FLOAT:
					var->data.f = w->data.f;
					break;
				case SCF_LEX_WORD_CONST_DOUBLE:
					var->data.d = w->data.d;
					break;
				case SCF_LEX_WORD_CONST_COMPLEX:
					var->data.z = w->data.z;
					break;

				case SCF_LEX_WORD_CONST_I64:
					var->data.i64 = w->data.i64;
					break;
				case SCF_LEX_WORD_CONST_U64:
					var->data.u64 = w->data.u64;
					break;
				default:
					break;
			};
		}
	} else {
		var->w = NULL;
	}

	return var;
}

scf_variable_t*	scf_variable_clone(scf_variable_t* var)
{
	var->refs++;
	return var;
}

scf_variable_t*	scf_variable_ref(scf_variable_t* var)
{
	var->refs++;
	return var;
}

void scf_variable_free(scf_variable_t* var)
{
	assert(var);

	var->refs--;
	if (var->refs > 0) {
		return;
	} else if (var->refs < 0) {
		assert(0); // BUGS
	}

	if (var->w) {
		scf_lex_word_free(var->w);
		var->w = NULL;
	}

	if (SCF_VAR_STRING == var->type) {
		scf_string_free(var->data.s);
		var->data.s = NULL;
	}

	if (var->signature) {
		scf_string_free(var->signature);
		var->signature = NULL;
	}

	free(var);
	var = NULL;
}

void scf_variable_add_array_dimention(scf_variable_t* var, int dimention_size)
{
	assert(var);

	void* p = realloc(var->dimentions, sizeof(int) * (var->nb_dimentions + 1));
	assert(p);
	var->dimentions = p;
	var->dimentions[var->nb_dimentions++] = dimention_size;
}

void scf_variable_alloc_space(scf_variable_t* var)
{
	assert(var);
	if (var->alloc_flag)
		return;

	if (var->nb_dimentions > 0) {
		int i;
		int n = 1;
		for (i = 0; i < var->nb_dimentions; i++) {
			assert(var->dimentions[i] > 0);
			n *= var->dimentions[i];
		}
		var->capacity = n;

		var->data.p = calloc(n, var->size);
		assert(var->data.p);
		var->alloc_flag = 1;

	} else if (var->type >= SCF_STRUCT) {
		var->data.p = calloc(1, var->size);
		assert(var->data.p);
		var->alloc_flag = 1;
	}
}

void scf_variable_get_array_member(scf_variable_t* array, int index, scf_variable_t* member)
{
	assert(array);
	assert(member);
	assert(array->type == member->type);
	assert(array->size == member->size);
	assert(index >= 0 && index < array->capacity);

	memcpy(&(member->data.i), array->data.p + index * member->size, member->size);
}

void scf_variable_set_array_member(scf_variable_t* array, int index, scf_variable_t* member)
{
	assert(array);
	assert(member);
	assert(array->type == member->type);
	assert(array->size == member->size);
	assert(index >= 0 && index < array->capacity);

	memcpy(array->data.p + index * member->size, &(member->data.i), member->size);
}

void scf_variable_print(scf_variable_t* var)
{
	assert(var);

	if (var->nb_pointers > 0) {
		printf("%s(),%d, print var: name: %s, type: %d, value: %p\n",
				__func__, __LINE__, var->w->text->data, var->type, var->data.p);
		return;	
	}

	if (SCF_VAR_CHAR == var->type || SCF_VAR_INT == var->type) {
		printf("%s(),%d, print var: name: %s, type: %d, value: %d\n",
				__func__, __LINE__, var->w->text->data, var->type, var->data.i);

		//assert(0);

	} else if (SCF_VAR_DOUBLE == var->type) {
		printf("%s(),%d, print var: name: %s, type: %d, value: %lg\n",
				__func__, __LINE__, var->w->text->data, var->type, var->data.d);

	} else if (SCF_VAR_STRING == var->type) {
		printf("%s(),%d, print var: name: %s, type: %d, value: %s\n",
				__func__, __LINE__, var->w->text->data, var->type, var->data.s->data);
	} else {
		printf("%s(),%d, print var: name: %s, type: %d\n",
				__func__, __LINE__, var->w->text->data, var->type);
	}
}

int scf_variable_same_type(scf_variable_t* v0, scf_variable_t* v1)
{
	if (v0) {
		if (!v1)
			return 0;

		if (v0->type != v1->type)
			return 0;

		if (v0->nb_pointers != v1->nb_pointers)
			return 0;

		if (v0->nb_dimentions != v1->nb_dimentions)
			return 0;

		int i;
		for (i = 0; i < v0->nb_dimentions; i++) {
			if (v0->dimentions[i] != v1->dimentions[i])
				return 0;
		}

		if (SCF_FUNCTION_PTR == v0->type) {
			assert(v0->func_ptr);
			assert(v1->func_ptr);

			if (!scf_function_same_type(v0->func_ptr, v1->func_ptr))
				return 0;
		}
	} else {
		if (v1)
			return 0;
	}

	return 1;
}

int scf_variable_type_like(scf_variable_t* v0, scf_variable_t* v1)
{
	if (v0) {
		if (!v1)
			return 0;

		if (v0->type != v1->type)
			return 0;

		if (scf_variable_nb_pointers(v0) != scf_variable_nb_pointers(v1))
			return 0;

		if (SCF_FUNCTION_PTR == v0->type) {
			assert(v0->func_ptr);
			assert(v1->func_ptr);

			if (!scf_function_same_type(v0->func_ptr, v1->func_ptr))
				return 0;
		}
	} else {
		if (v1)
			return 0;
	}

	return 1;
}

void scf_variable_sign_extend(scf_variable_t* v, int bytes)
{
	if (bytes <= v->size)
		return;

	bytes = bytes > 8 ? 8 : bytes;

	v->data.u64 = scf_sign_extend(v->data.u64, v->size << 3);

	v->size = bytes;
}

void scf_variable_zero_extend(scf_variable_t* v, int bytes)
{
	if (bytes <= v->size)
		return;

	bytes = bytes > 8 ? 8 : bytes;

	v->data.u64 = scf_zero_extend(v->data.u64, v->size << 3);

	v->size = bytes;
}

void scf_variable_extend_bytes(scf_variable_t* v, int bytes)
{
	if (scf_type_is_signed(v->type))
		scf_variable_sign_extend(v, bytes);
	else
		scf_variable_zero_extend(v, bytes);
}

void scf_variable_extend_std(scf_variable_t* v, scf_variable_t* std)
{
	if (scf_type_is_signed(v->type))
		scf_variable_sign_extend(v, std->size);
	else
		scf_variable_zero_extend(v, std->size);

	v->type = std->type;
}

int scf_variable_size(scf_variable_t* v)
{
	if (0 == v->nb_dimentions)
		return v->size;

	assert(v->nb_dimentions > 0);

	int capacity = 1;
	int j;

	for (j = 0; j < v->nb_dimentions; j++) {
		if (v->dimentions[j] < 0) {
			scf_loge("\n");
			return -EINVAL;
		}

		capacity *= v->dimentions[j];
	}
	v->capacity = capacity;

	return capacity * v->size;
}


