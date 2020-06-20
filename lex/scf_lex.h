#ifndef SCF_LEX_H
#define SCF_LEX_H

#include"scf_lex_word.h"

typedef struct {
	char*	text;
	int		type;
} scf_lex_key_word_t;

typedef struct {
	int		origin;
	int		escape;
} scf_lex_escape_char_t;

typedef struct {
	scf_list_t		list;	// manage list, all errors here

	scf_string_t*	message;	// error message for user

	scf_string_t*	file;	// original code file name
	int				line;	// line in the code file above	
	int				pos;	// position in the line above

} scf_lex_error_t;

typedef struct {
	scf_list_t		list;
	int				c;
} scf_lex_char_t;

typedef struct {
	scf_list_t		word_list_head; // word list head
	scf_list_t		error_list_head; // error list head

	scf_list_t		char_list_head; // temp char list head
	FILE*			fp;	// file pointer to the code

	int				nb_identities;

	scf_string_t*	file;	// original code file name
	int				nb_lines;
	int				pos;

} scf_lex_t;

scf_lex_error_t*	scf_lex_error_alloc(scf_string_t* file, int line, int pos);
void 				scf_lex_error_free(scf_lex_error_t* e);

int	scf_lex_open(scf_lex_t** plex, const char* path);
int scf_lex_close(scf_lex_t* lex);

int scf_lex_push_word(scf_lex_t* lex, scf_lex_word_t* word);
int scf_lex_pop_word(scf_lex_t* lex, scf_lex_word_t** pword);

#endif

