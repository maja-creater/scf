#include"scf_lex.h"

int main(int argc, char* argv[])
{
	scf_lex_t*	lex = NULL;

	if (scf_lex_open(&lex, argv[1]) < 0) {
		printf("%s(),%d, error: \n", __func__, __LINE__);
		return -1;
	}

	scf_lex_word_t* w = NULL;
	while (1) {
		if (scf_lex_pop_word(lex, &w) < 0) {
			printf("%s(),%d, error: \n", __func__, __LINE__);
			return -1;
		}

		printf("word: type: %d, line: %d, pos: %d, text: %s",
				w->type, w->line, w->pos, w->text->data);

		if (SCF_LEX_WORD_CONST_STRING == w->type) {
			printf(", data: %s", w->data.s->data);

		} else if (SCF_LEX_WORD_CONST_CHAR == w->type || SCF_LEX_WORD_CONST_INT == w->type) {
			printf(", data: %d", w->data.i);

		} else if (SCF_LEX_WORD_CONST_DOUBLE == w->type) {
			printf(", data: %lg", w->data.d);
		}
		printf("\n");

		if (SCF_LEX_WORD_EOF == w->type) {
			printf("%s(),%d, eof\n", __func__, __LINE__);
			break;
		}

		scf_lex_word_free(w);
		w = NULL;
	}

	printf("%s(),%d, main ok\n", __func__, __LINE__);
	return 0;
}
