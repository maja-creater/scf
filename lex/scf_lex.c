#include"scf_lex.h"

static scf_lex_key_word_t	key_words[] = {
	{"if",        SCF_LEX_WORD_KEY_IF},
	{"else",      SCF_LEX_WORD_KEY_ELSE},

	{"for",       SCF_LEX_WORD_KEY_FOR},
	{"while",     SCF_LEX_WORD_KEY_WHILE},

	{"break",     SCF_LEX_WORD_KEY_BREAK},
	{"continue",  SCF_LEX_WORD_KEY_CONTINUE},

	{"switch",    SCF_LEX_WORD_KEY_SWITCH},
	{"case",      SCF_LEX_WORD_KEY_CASE},
	{"default",   SCF_LEX_WORD_KEY_DEFAULT},

	{"return",    SCF_LEX_WORD_KEY_RETURN},

	{"goto",      SCF_LEX_WORD_KEY_GOTO},
	{"error",     SCF_LEX_WORD_KEY_ERROR},

	{"sizeof",    SCF_LEX_WORD_KEY_SIZEOF},

	{"create",    SCF_LEX_WORD_KEY_CREATE},

	{"operator",  SCF_LEX_WORD_KEY_OPERATOR},

	{"_",         SCF_LEX_WORD_KEY_UNDERLINE},

	{"char",      SCF_LEX_WORD_KEY_CHAR},

	{"int",       SCF_LEX_WORD_KEY_INT},
	{"float",     SCF_LEX_WORD_KEY_FLOAT},
	{"double",    SCF_LEX_WORD_KEY_DOUBLE},

	{"int8_t",    SCF_LEX_WORD_KEY_INT8},
	{"int16_t",   SCF_LEX_WORD_KEY_INT16},
	{"int32_t",   SCF_LEX_WORD_KEY_INT32},
	{"int64_t",   SCF_LEX_WORD_KEY_INT64},

	{"uint8_t",   SCF_LEX_WORD_KEY_UINT8},
	{"uint16_t",  SCF_LEX_WORD_KEY_UINT16},
	{"uint32_t",  SCF_LEX_WORD_KEY_UINT32},
	{"uint64_t",  SCF_LEX_WORD_KEY_UINT64},

	{"intptr_t",  SCF_LEX_WORD_KEY_INTPTR},
	{"uintptr_t", SCF_LEX_WORD_KEY_UINTPTR},

	{"void",      SCF_LEX_WORD_KEY_VOID},

	{"va_start",  SCF_LEX_WORD_KEY_VA_START},
	{"va_arg",    SCF_LEX_WORD_KEY_VA_ARG},
	{"va_end",    SCF_LEX_WORD_KEY_VA_END},

	{"container", SCF_LEX_WORD_KEY_CONTAINER},

	{"class",     SCF_LEX_WORD_KEY_CLASS},

	{"const",     SCF_LEX_WORD_KEY_CONST},
	{"static",    SCF_LEX_WORD_KEY_STATIC},
	{"extern",    SCF_LEX_WORD_KEY_EXTERN},
	{"inline",    SCF_LEX_WORD_KEY_INLINE},

	{"async",     SCF_LEX_WORD_KEY_ASYNC},

	{"include",   SCF_LEX_WORD_KEY_INCLUDE},

	{"union",     SCF_LEX_WORD_KEY_UNION},
	{"struct",    SCF_LEX_WORD_KEY_STRUCT},
};

static scf_lex_escape_char_t	escape_chars[] = {
	{'n', '\n'},
	{'r', '\r'},
	{'t', '\t'},
	{'0', '\0'},
};

static int _find_key_word(const char* text)
{
	int i;
	for (i = 0; i < sizeof(key_words) / sizeof(key_words[0]); i++) {
		if (!strcmp(key_words[i].text, text)) {
			return key_words[i].type;
		}
	}
	return -1;
}

static int _find_escape_char(const int c)
{
	int i;
	for (i = 0; i < sizeof(escape_chars) / sizeof(escape_chars[0]); i++) {
		if (escape_chars[i].origin == c) {
			return escape_chars[i].escape; // return the escape char
		}
	}

	// if it isn't in the escape array, return the original char
	return c;
}

scf_lex_error_t* scf_lex_error_alloc(scf_string_t* file, int line, int pos)
{
	assert(file);

	scf_lex_error_t* e = calloc(1, sizeof(scf_lex_error_t));
	assert(e);

	e->file = scf_string_clone(file);
	assert(e->file);

	e->line = line;
	e->pos = pos;
	return e;
}

void scf_lex_error_free(scf_lex_error_t* e)
{
	assert(e);

	if (e->message)
		scf_string_free(e->message);
	if (e->file)
		scf_string_free(e->file);

	free(e);
	e = NULL;
}

int	scf_lex_open(scf_lex_t** plex, const char* path)
{
	printf("%s(),%d, keywords: %ld\n", __func__, __LINE__,
			sizeof(key_words) / sizeof(key_words[0]));

	assert(plex);
	assert(path);

	scf_lex_t* lex = calloc(1, sizeof(scf_lex_t));
	assert(lex);

	scf_list_init(&lex->word_list_head);
	scf_list_init(&lex->error_list_head);
	scf_list_init(&lex->char_list_head);

	lex->fp = fopen(path, "r");
	if (!lex->fp) {
		char cwd[4096];
		getcwd(cwd, 4095);
		scf_loge("path: %s, errno: %d, pwd: %s\n", path, errno, cwd);

		free(lex);
		lex = NULL;
		return -1;
	}

	lex->file = scf_string_cstr(path);
	assert(lex->file);

	lex->nb_lines = 1;

	*plex = lex;
	return 0;
}

int scf_lex_close(scf_lex_t* lex)
{
	assert(lex);

	scf_list_clear(&lex->word_list_head, scf_lex_word_t, list, scf_lex_word_free);
	scf_list_clear(&lex->error_list_head, scf_lex_error_t, list, scf_lex_error_free);

	free(lex);
	lex = NULL;
	return 0;
}

int scf_lex_push_word(scf_lex_t* lex, scf_lex_word_t* word)
{
	assert(lex);
	assert(word);

	scf_list_add_front(&lex->word_list_head, &word->list);
	return 0;
}

static scf_lex_char_t* _lex_pop_char(scf_lex_t* lex)
{
	assert(lex);
	assert(lex->fp);

	if (!scf_list_empty(&lex->char_list_head)) {
		scf_list_t* l = scf_list_head(&lex->char_list_head);
		scf_lex_char_t* c = scf_list_data(l, scf_lex_char_t, list);
		scf_list_del(&c->list);
		return c;
	}

	scf_lex_char_t* c = malloc(sizeof(scf_lex_char_t));
	assert(c);

	c->c = fgetc(lex->fp);
	return c;
}

static void _lex_push_char(scf_lex_t* lex, scf_lex_char_t* c)
{
	assert(lex);
	assert(c);

	scf_list_add_front(&lex->char_list_head, &c->list);
}

static void _lex_space(scf_lex_t* lex)
{
	scf_lex_char_t* c = _lex_pop_char(lex);

	if ('\n' == c->c || '\r' == c->c || '\t' == c->c || ' ' == c->c) {

		if ('\n' == c->c) {
			lex->nb_lines++;
			lex->pos = 0;
		} else {
			lex->pos++;
		}

		free(c);
		c = NULL;

		_lex_space(lex); // recursive call until others, to delete the unexpected space
	} else {
		_lex_push_char(lex, c); // save to the front of temp char list, used it later
	}
}

static int _lex_plus(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0)
{
	scf_lex_char_t* c1 = _lex_pop_char(lex);
	if ('+' == c1->c) {
		scf_lex_char_t* c2 = _lex_pop_char(lex);
		if ('+' == c2->c) {
			scf_lex_error_t* e = scf_lex_error_alloc(lex->file, lex->nb_lines, lex->pos);
			e->message = scf_string_cstr("error: +++ is not supported");
			scf_list_add_tail(&lex->error_list_head, &e->list);

			free(c2);
			free(c1);
			free(c0);
			c2 = NULL;
			c1 = NULL;
			c0 = NULL;

			// can add error correction to continue the lexer
			scf_loge("\n");
			return -1;
		} else {
			_lex_push_char(lex, c2);
			c2 = NULL;

			scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_INC);
			w->text = scf_string_cstr("++");
			lex->pos += 2;

			*pword = w;

			free(c1);
			c1 = NULL;
		}
	} else if ('=' == c1->c) {
		scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_ADD_ASSIGN);
		w->text = scf_string_cstr("+=");
		lex->pos += 2;

		*pword = w;
		free(c1);
		c1 = NULL;
	} else {
		_lex_push_char(lex, c1);
		c1 = NULL;

		scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_PLUS);
		w->text = scf_string_cstr("+");
		lex->pos++;

		*pword = w;
	}

	free(c0);
	c0 = NULL;
	return 0;
}

static int _lex_minus(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0)
{
	scf_lex_char_t* c1 = _lex_pop_char(lex);
	if ('-' == c1->c) {
		scf_lex_char_t* c2 = _lex_pop_char(lex);
		if ('-' == c2->c) {
			scf_lex_error_t* e = scf_lex_error_alloc(lex->file, lex->nb_lines, lex->pos);
			e->message = scf_string_cstr("error: --- is not supported");
			scf_list_add_tail(&lex->error_list_head, &e->list);

			free(c2);
			free(c1);
			free(c0);
			c2 = NULL;
			c1 = NULL;
			c0 = NULL;

			// can add error correction to continue the lexer
			scf_loge("\n");
			return -1;
		} else {
			_lex_push_char(lex, c2);
			c2 = NULL;

			scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_DEC);
			w->text = scf_string_cstr("--");
			lex->pos += 2;

			*pword = w;
			free(c1);
			c1 = NULL;
		}
	} else if ('>' == c1->c) {
		scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_ARROW);
		w->text = scf_string_cstr("->");
		lex->pos += 2;

		*pword = w;
		free(c1);
		c1 = NULL;

	} else if ('=' == c1->c) {
		scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_SUB_ASSIGN);
		w->text = scf_string_cstr("-=");
		lex->pos += 2;

		*pword = w;
		free(c1);
		c1 = NULL;
	} else {
		_lex_push_char(lex, c1);
		c1 = NULL;

		scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_MINUS);
		w->text = scf_string_cstr("-");
		lex->pos++;

		*pword = w;
	}

	free(c0);
	c0 = NULL;
	return 0;
}

static int _lex_number_base_16(scf_lex_t* lex, scf_lex_word_t** pword, scf_string_t* s)
{
	uint64_t value = 0;

	while (1) {
		uint64_t value2;

		scf_lex_char_t* c2 = _lex_pop_char(lex);

		if (c2->c >= '0' && c2->c <= '9')
			value2 =        c2->c -  '0';

		else if ('a' <= c2->c && 'f' >= c2->c)
			value2    = c2->c  - 'a' + 10;

		else if ('A' <= c2->c && 'F' >= c2->c)
			value2    = c2->c  - 'A' + 10;

		else {
			_lex_push_char(lex, c2);
			c2 = NULL;

			scf_lex_word_t* w;

			if (value & ~0xffffffffULL)
				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_U64);
			else
				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_U32);

			w->data.u64 = value;

			w->text = s;
			s = NULL;

			*pword = w;
			return 0;
		}

		value <<= 4;
		value  += value2;

		scf_string_cat_cstr_len(s, (char*)&c2->c, 1);
		lex->pos++;

		free(c2);
		c2 = NULL;
	}
}

static int _lex_number_base_8(scf_lex_t* lex, scf_lex_word_t** pword, scf_string_t* s)
{
	uint64_t value = 0;

	while (1) {
		scf_lex_char_t* c2 = _lex_pop_char(lex);

		if (c2->c >= '0' && c2->c <= '7') {
			scf_string_cat_cstr_len(s, (char*)&c2->c, 1);
			lex->pos++;

			value  = (value << 3) + c2->c - '0';

			free(c2);
			c2 = NULL;

		} else if ('8' == c2->c || '9' == c2->c) {
			scf_loge("number must be 0-7 when base 8");
			free(c2);
			c2 = NULL;
			return -1;
		} else {
			_lex_push_char(lex, c2);
			c2 = NULL;

			scf_lex_word_t* w;

			if (value & ~0xffffffffULL)
				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_U64);
			else
				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_U32);
			w->data.u64 = value;

			w->text = s;
			s = NULL;

			*pword = w;
			return 0;
		}
	}
}

static int _lex_number(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0)
{
	lex->pos++;

	if ('0' == c0->c) {
		scf_lex_char_t* c1 = _lex_pop_char(lex);
		if ('x' == c1->c || 'X' == c1->c) {
			// base 16
			scf_string_t* s = scf_string_cstr_len((char*)&c0->c, 1);
			scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
			lex->pos += 2;
			free(c1);
			c1 = NULL;
			free(c0);
			c0 = NULL;

			return _lex_number_base_16(lex, pword, s);

		} else if ('.' == c1->c) {
			// double
			scf_string_t* s = scf_string_cstr_len((char*)&c0->c, 1);
			scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
			lex->pos += 2;
			free(c1);
			c1 = NULL;
			free(c0);
			c0 = NULL;

			while (1) {
				scf_lex_char_t* c2 = _lex_pop_char(lex);
				if (c2->c >= '0' && c2->c <= '9') {
					scf_string_cat_cstr_len(s, (char*)&c2->c, 1);
					lex->pos++;
					free(c2);
					c2 = NULL;
				} else if ('.' == c2->c) {
					// error
					printf("%s(),%d, error: \n", __func__, __LINE__);
					return -1;
				} else {
					_lex_push_char(lex, c2);
					c2 = NULL;

					scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_DOUBLE);
					w->data.d = atof(s->data);

					w->text = s;
					s = NULL;

					*pword = w;
					return 0;
				}
			}
		} else {
			scf_string_t* s = scf_string_cstr_len((char*)&c0->c, 1);

			if (c1->c < '0' || c1->c > '9') {
				// is 0
				_lex_push_char(lex, c1);
				c1 = NULL;

				scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_INT);
				w->data.i = atoi(s->data);

				w->text = s;
				s = NULL;
				lex->pos++;

				*pword = w;
				return 0;
			}

			// base 8
			_lex_push_char(lex, c1);
			c1 = NULL;

			lex->pos++;

			return _lex_number_base_8(lex, pword, s);
		}
	} else {
		// base 10
		scf_string_t* s  = scf_string_cstr_len((char*)&c0->c, 1);

		uint64_t value   = c0->c - '0';
		int      nb_dots = 0;

		while (1) {
			scf_lex_char_t* c1 = _lex_pop_char(lex);

			if ((c1->c >= '0' && c1->c <= '9') || '.' == c1->c) {
				if ('.' == c1->c) {
					nb_dots++;
					if (nb_dots > 1) {
						// error
						scf_lex_error_t* e = scf_lex_error_alloc(lex->file, lex->nb_lines, lex->pos);
						e->message = scf_string_cstr("error: 2 '.' found, number is wrong");
						scf_list_add_tail(&lex->error_list_head, &e->list);
						printf("%s(),%d, error: \n", __func__, __LINE__);
						return -1;
					}
				} else
					value = value * 10 + c1->c - '0';

				scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
				lex->pos++;
				free(c1);
				c1 = NULL;
			} else {
				_lex_push_char(lex, c1);
				c1 = NULL;

				scf_lex_word_t* w = NULL;
				if (nb_dots > 0) {
					w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_DOUBLE);
					w->data.d = atof(s->data);
				} else {
					if (value & ~0xffffffffULL)
						w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_I64);
					else
						w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_INT);
					w->data.i64 = value;
				}

				w->text = s;
				s = NULL;

				*pword = w;

				free(c0);
				c0 = NULL;
				return 0;
			}
		}
	}
}

static int _lex_identity(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0)
{
	scf_string_t* s = scf_string_cstr_len((char*)&c0->c, 1);
	lex->pos++;
	free(c0);
	c0 = NULL;

	while (1) {
		scf_lex_char_t* c1 = _lex_pop_char(lex);
		if (('a' <= c1->c && 'z' >= c1->c)
				|| ('A' <= c1->c && 'Z' >= c1->c)
				|| ('0' <= c1->c && '9' >= c1->c)
				|| '_' == c1->c
				) {
			scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
			lex->pos++;
			free(c1);
			c1 = NULL;
		} else {
			_lex_push_char(lex, c1);
			c1 = NULL;

			scf_lex_word_t* w = NULL;

			if (!strcmp(s->data, "NULL")) {

				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_U64);
				if (w)
					w->data.u64 = 0;

			} else if (!strcmp(s->data, "__LINE__")) {

				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_U64);
				if (w)
					w->data.u64 = lex->nb_lines;

			} else if (!strcmp(s->data, "__func__")) {

				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_STRING);

			} else {
				int type = _find_key_word(s->data);

				if (-1 == type) {
					type = SCF_LEX_WORD_ID + lex->nb_identities++;
					w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, type);
				} else
					w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, type);
			}

			if (w)
				w->text = s;
			else
				scf_string_free(s);
			s = NULL;

			*pword = w;
			return 0;
		}
	}

	return -1;
}

static int _lex_dot(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0)
{
	scf_string_t* s = scf_string_cstr_len((char*)&c0->c, 1);
	lex->pos++;
	free(c0);
	c0 = NULL;

	int nb_numbers = 0;
	int nb_dots    = 1;

	while (1) {
		scf_lex_char_t* c1 = _lex_pop_char(lex);

		if ('0' <= c1->c && '9' >= c1->c) {
			nb_numbers++;
		} else if ('.' == c1->c) {
			nb_dots++;
		} else {
			_lex_push_char(lex, c1);
			c1 = NULL;

			scf_lex_word_t* w = NULL;

			if (nb_numbers  > 0) {
				if (nb_dots > 1) {
					scf_loge("\n");
					return -1;
				}

				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_DOUBLE);
				w->data.d = atof(s->data);

			} else if (1 == nb_dots) { // dot .
				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_DOT);

			} else if (3 == nb_dots) { // variable args ...
				w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_VAR_ARGS);

			} else if (nb_dots > 1) { // dot must be 1 or 3, others error
				scf_loge("\n");
				return -1;
			}
			w->text = s;
			s = NULL;

			*pword = w;
			return 0;
		}

		scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
		lex->pos++;
		free(c1);
		c1 = NULL;
	}
}

static int _lex_op1_ll1(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0, int type0)
{
	scf_string_t*	s = scf_string_cstr_len((char*)&c0->c, 1);
	scf_lex_word_t*	w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, type0);

	lex->pos++;
	w->text = s;
	s = NULL;
	*pword = w;

	free(c0);
	c0 = NULL;
	return 0;
}

static int _lex_op2_ll1(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0,
		int type0, char* chs, int* types, int n)
{
	scf_string_t* s = scf_string_cstr_len((char*)&c0->c, 1);

	scf_lex_word_t* w = NULL;
	scf_lex_char_t* c1 = _lex_pop_char(lex);

	int i;
	for (i = 0; i < n; i++) {
		if (chs[i] == c1->c)
			break;
	}

	if (i < n) {
		scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
		w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, types[i]);
		lex->pos += 2;

		free(c1);
		c1 = NULL;
	} else {
		_lex_push_char(lex, c1);
		c1 = NULL;
		w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, type0);
		lex->pos++;
	}
	w->text = s;
	s       = NULL;
	*pword  = w;

	free(c0);
	c0 = NULL;
	return 0;
}

static int _lex_char(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0)
{
	scf_lex_word_t* w = NULL;
	scf_string_t* s = scf_string_cstr_len((char*)&c0->c, 1);

	scf_lex_char_t* c1 = _lex_pop_char(lex);

	if ('\\' == c1->c) {
		scf_lex_char_t* c2 = _lex_pop_char(lex);
		scf_lex_char_t* c3 = _lex_pop_char(lex);

		if ('\'' == c3->c) {
			scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
			scf_string_cat_cstr_len(s, (char*)&c2->c, 1);
			scf_string_cat_cstr_len(s, (char*)&c3->c, 1);

			w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_CHAR);
			w->data.i = _find_escape_char(c2->c);
			lex->pos += 4;

			free(c3);
			c3 = NULL;
			free(c2);
			c2 = NULL;
		} else {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	} else {
		scf_lex_char_t* c2 = _lex_pop_char(lex);
		if ('\'' == c2->c) {
			scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
			scf_string_cat_cstr_len(s, (char*)&c2->c, 1);

			w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_CHAR);
			w->data.i = c1->c;
			lex->pos += 3;

			free(c2);
			c2 = NULL;
		} else {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}
	}

	w->text = s;
	s = NULL;
	*pword = w;

	free(c1);
	c1 = NULL;
	free(c0);
	c0 = NULL;
	return 0;
}

static int _lex_string(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0)
{
	scf_lex_word_t* w = NULL;
	scf_string_t* s = scf_string_cstr_len((char*)&c0->c, 1);
	scf_string_t* data = scf_string_alloc();

	while (1) {
		scf_lex_char_t* c1 = _lex_pop_char(lex);
		if ('\"' == c1->c) {
			scf_string_cat_cstr_len(s, (char*)&c1->c, 1);

			w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_CONST_STRING);
			w->data.s = data;
			data = NULL;
			w->text = s;
			s = NULL;
			lex->pos++;
			*pword = w;

			free(c1);
			c1 = NULL;
			return 0;
		} else if ('\\' == c1->c) {
			scf_lex_char_t* c2 = _lex_pop_char(lex);
			int ch2 = _find_escape_char(c2->c);

			scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
			scf_string_cat_cstr_len(s, (char*)&c2->c, 1);

			scf_string_cat_cstr_len(data, (char*)&ch2, 1);
			lex->pos += 2;

			free(c2);
			c2 = NULL;
			free(c1);
			c1 = NULL;
		} else if (EOF == c1->c) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		} else {
			scf_string_cat_cstr_len(s, (char*)&c1->c, 1);
			scf_string_cat_cstr_len(data, (char*)&c1->c, 1);
			lex->pos++;
			free(c1);
			c1 = NULL;
		}
	}
}

static int _lex_op3_ll1(scf_lex_t* lex, scf_lex_word_t** pword, scf_lex_char_t* c0,
		char ch1_0, char ch1_1, char ch2, int type0, int type1, int type2, int type3)
{
	scf_lex_char_t* c1 = _lex_pop_char(lex);
	scf_lex_word_t* w  = NULL;
	scf_string_t*   s  = scf_string_cstr_len((char*)&c0->c, 1);

	if (ch1_0 == c1->c) {
		scf_string_cat_cstr_len(s, (char*)&c1->c, 1);

		scf_lex_char_t* c2 = _lex_pop_char(lex);

		if (ch2 == c2->c) {
			scf_string_cat_cstr_len(s, (char*)&c2->c, 1);

			w         = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, type0);
			w->text   = s;
			s         = NULL;
			lex->pos += 3;

			free(c2);
			c2 = NULL;
		} else {
			_lex_push_char(lex, c2);
			c2 = NULL;

			w         = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, type1);
			w->text   = s;
			s         = NULL;
			lex->pos += 2;
		}

		free(c1);
		c1 = NULL;

	} else if (ch1_1 == c1->c) {
		scf_string_cat_cstr_len(s, (char*)&c1->c, 1);

		w         = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, type2);
		w->text   = s;
		s         = NULL;
		lex->pos += 2;

		free(c1);
		c1 = NULL;
	} else {
		_lex_push_char(lex, c1);
		c1 = NULL;

		w       = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, type3);
		w->text = s;
		s       = NULL;
		lex->pos++;
	}

	*pword = w;
	free(c0);
	c0 = NULL;
	return 0;
}

int scf_lex_pop_word(scf_lex_t* lex, scf_lex_word_t** pword)
{
	assert(lex);
	assert(pword);
	assert(lex->fp);

	if (!scf_list_empty(&lex->word_list_head)) {
		scf_list_t*		l = scf_list_head(&lex->word_list_head);
		scf_lex_word_t*	w = scf_list_data(l, scf_lex_word_t, list);

		scf_list_del(&w->list);
		*pword = w;
		return 0;
	}

	scf_lex_char_t* c = _lex_pop_char(lex);
	if (EOF == c->c) {
		scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_EOF);
		w->text = scf_string_cstr("eof");
		*pword = w;
		free(c);
		c = NULL;
		return 0;
	}

	if ('\n' == c->c || '\r' == c->c || '\t' == c->c || ' ' == c->c) {
#if 0
		scf_lex_word_t* w = scf_lex_word_alloc(lex->file, lex->nb_lines, lex->pos, SCF_LEX_WORD_SPACE);
		w->text = scf_string_cstr(" ");
		*pword = w;
#endif
		if ('\n' == c->c) {
			lex->nb_lines++;
			lex->pos = 0;
		} else {
			lex->pos++;
		}

		free(c);
		c = NULL;
		_lex_space(lex);
		return scf_lex_pop_word(lex, pword); // recursive call to drop the SPACE WORD
	}

	if ('+' == c->c)
		return _lex_plus(lex, pword, c);

	if ('-' == c->c)
		return _lex_minus(lex, pword, c);

	if ('*' == c->c) {
		char c1 = '=';
		int  t1 = SCF_LEX_WORD_MUL_ASSIGN;

		return _lex_op2_ll1(lex, pword, c, SCF_LEX_WORD_STAR, &c1, &t1, 1);
	}

	if ('/' == c->c) {

		scf_lex_char_t* c2 = _lex_pop_char(lex);

		if ('/' == c2->c) {
			free(c);
			free(c2);
			c  = NULL;
			c2 = NULL;

			while (1) {
				c2 = _lex_pop_char(lex);

				if (EOF == c2->c) {
					_lex_push_char(lex, c2);
					break;
				}

				int tmp = c2->c;
				free(c2);
				c2 = NULL;

				if ('\n' == tmp) {
					lex->nb_lines++;
					lex->pos = 0;
					break;
				}
			}

			return scf_lex_pop_word(lex, pword);

		} else if ('*' == c2->c) {
			free(c);
			free(c2);
			c  = NULL;
			c2 = NULL;

			while (1) {
				c2 = _lex_pop_char(lex);

				if (EOF == c2->c) {
					_lex_push_char(lex, c2);
					break;
				}

				int tmp = c2->c;
				free(c2);
				c2 = NULL;

				if ('\n' == tmp) {
					lex->nb_lines++;
					lex->pos = 0;
					continue;
				}

				if ('*' == tmp) {
					c2  = _lex_pop_char(lex);

					if ('/' == c2->c) {
						free(c2);
						c2 = NULL;
						break;
					}

					_lex_push_char(lex, c2);
					c2 = NULL;
				}
			}

			return scf_lex_pop_word(lex, pword);

		} else {
			_lex_push_char(lex, c2);
			c2 = NULL;
		}

		char c1 = '=';
		int  t1 = SCF_LEX_WORD_DIV_ASSIGN;

		return _lex_op2_ll1(lex, pword, c, SCF_LEX_WORD_DIV, &c1, &t1, 1);
	}

	if ('%' == c->c) {
		char c1 = '=';
		int  t1 = SCF_LEX_WORD_MOD_ASSIGN;

		return _lex_op2_ll1(lex, pword, c, SCF_LEX_WORD_MOD, &c1, &t1, 1);
	}

	switch (c->c) {
		case '~':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_BIT_NOT);
			break;

		case '(':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_LP);
			break;
		case ')':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_RP);
			break;
		case '[':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_LS);
			break;
		case ']':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_RS);
			break;
		case '{':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_LB);
			break;
		case '}':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_RB);
			break;

		case ',':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_COMMA);
			break;
		case ';':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_SEMICOLON);
			break;
		case ':':
			return _lex_op1_ll1(lex, pword, c, SCF_LEX_WORD_COLON);
			break;

		default:
			break;
	};

	if ('&' == c->c) {
		char chs[2]   = {'&', '='};
		int  types[2] = {SCF_LEX_WORD_LOGIC_AND, SCF_LEX_WORD_BIT_AND_ASSIGN};

		return _lex_op2_ll1(lex, pword, c, SCF_LEX_WORD_BIT_AND, chs, types, 2);
	}

	if ('|' == c->c) {
		char chs[2]   = {'|', '='};
		int  types[2] = {SCF_LEX_WORD_LOGIC_OR, SCF_LEX_WORD_BIT_OR_ASSIGN};

		return _lex_op2_ll1(lex, pword, c, SCF_LEX_WORD_BIT_OR, chs, types, 2);
	}

	if ('!' == c->c) {
		char c1 = '=';
		int  t1 = SCF_LEX_WORD_NE;

		return _lex_op2_ll1(lex, pword, c, SCF_LEX_WORD_LOGIC_NOT, &c1, &t1, 1);
	}

	if ('=' == c->c) {
		char c1 = '=';
		int  t1 = SCF_LEX_WORD_EQ;

		return _lex_op2_ll1(lex, pword, c, SCF_LEX_WORD_ASSIGN, &c1, &t1, 1);
	}

	if ('>' == c->c)
		return _lex_op3_ll1(lex, pword, c, '>', '=', '=',
				SCF_LEX_WORD_SHR_ASSIGN, SCF_LEX_WORD_SHR, SCF_LEX_WORD_GE, SCF_LEX_WORD_GT);

	if ('<' == c->c)
		return _lex_op3_ll1(lex, pword, c, '<', '=', '=',
				SCF_LEX_WORD_SHL_ASSIGN, SCF_LEX_WORD_SHL, SCF_LEX_WORD_LE, SCF_LEX_WORD_LT);

	if ('.' == c->c)
		return _lex_dot(lex, pword, c);

	if ('\'' == c->c)
		return _lex_char(lex, pword, c);

	if ('\"' == c->c) {
		scf_lex_word_t* w0 = NULL;

		int ret = _lex_string(lex, &w0, c);
		if (ret < 0) {
			*pword = NULL;
			return ret;
		}

		while (1) {
			scf_lex_word_t* w1 = NULL;

			ret = scf_lex_pop_word(lex, &w1);
			if (ret < 0) {
				scf_lex_word_free(w0);
				*pword = NULL;
				return ret;
			}

			if (SCF_LEX_WORD_CONST_STRING != w1->type) {
				scf_lex_push_word(lex, w1);
				break;
			}

			if (scf_string_cat(w0->text, w1->text) < 0
					|| scf_string_cat(w0->data.s, w1->data.s) < 0) {

				scf_lex_word_free(w1);
				scf_lex_word_free(w0);
				*pword = NULL;
				return -1;
			}

			scf_lex_word_free(w1);
		}

		scf_logd("w0: %s\n", w0->data.s->data);
		*pword = w0;
		return 0;
	}

	if ('0' <= c->c && '9' >= c->c)
		return _lex_number(lex, pword, c);

	if ('_' == c->c || ('a' <= c->c && 'z' >= c->c) || ('A' <= c->c && 'Z' >= c->c)) {
		return _lex_identity(lex, pword, c);
	}

	scf_loge("c: %c\n", c->c);
	return -1;
}

