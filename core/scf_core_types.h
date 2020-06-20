#ifndef SCF_CORE_TYPES_H
#define SCF_CORE_TYPES_H

typedef struct scf_type_s       scf_type_t;
typedef struct scf_variable_s   scf_variable_t;

typedef struct scf_label_s		scf_label_t;

typedef struct scf_node_s       scf_node_t;
typedef struct scf_operator_s	scf_operator_t;

typedef struct scf_block_s      scf_block_t;
typedef struct scf_function_s   scf_function_t;

typedef struct scf_scope_s      scf_scope_t;


enum scf_core_types {
	SCF_OP_ADD	= 0,	// +
	SCF_OP_SUB,			// -
	SCF_OP_MUL,			// *
	SCF_OP_DIV,			// / division

	SCF_OP_INC,			// ++
	SCF_OP_DEC,			// --

	SCF_OP_NEG,			// -
	SCF_OP_POSITIVE,	// +

	SCF_OP_BIT_AND,		// &
	SCF_OP_BIT_OR,		// |
	SCF_OP_BIT_NOT,		// ~

	SCF_OP_LOGIC_AND,		// &&
	SCF_OP_LOGIC_OR,		// ||
	SCF_OP_LOGIC_NOT,		// !

	SCF_OP_ASSIGN,		// = assign

	SCF_OP_LT,			// < less than
	SCF_OP_GT,			// > greater than

	SCF_OP_EQ,			// == equal
	SCF_OP_NE,			// != not equal
	SCF_OP_LE,			// <= less equal 
	SCF_OP_GE,			// >= greater equal

	// 21
	SCF_OP_EXPR,		// () expr
	SCF_OP_CALL,		// () function call
	SCF_OP_TYPE_CAST,	// (char*) type cast

	SCF_OP_CREATE,	    // create
	SCF_OP_SIZEOF,	    // sizeof

	SCF_OP_ADDRESS_OF,	// & get variable address
	SCF_OP_DEREFERENCE,	// * pointer dereference
	SCF_OP_ARRAY_INDEX,	// [] array index

	SCF_OP_POINTER,     // -> struct member
	SCF_OP_DOT,			// . dot

	SCF_OP_BLOCK,		// statement block, first in fisr run
	SCF_OP_IF,			// if statement
	SCF_OP_FOR,			// for statement
	SCF_OP_WHILE,		// while statement
	SCF_OP_RETURN,		// return statement
	SCF_OP_BREAK,		// break statement
	SCF_OP_CONTINUE,	// continue statement
	SCF_OP_ERROR,       // error statement

	SCF_OP_3AC_TEQ,		// test if 0
	SCF_OP_3AC_JE,		// je, jmp if 0
	SCF_OP_3AC_JNE,		// jne, jmp if not 0
	SCF_OP_3AC_CALL_EXTERN, // call extern function
	SCF_OP_3AC_PUSH,    // push a var to stack, only for 3ac & native 
	SCF_OP_3AC_POP,     // pop a var from stack, only for 3ac & native
	SCF_OP_GOTO,		// goto statement

	SCF_VAR_CHAR,		// char variable

	SCF_VAR_INT,		// int variable
	SCF_VAR_FLOAT,      // float variable
	SCF_VAR_DOUBLE,		// double variable
	SCF_VAR_COMPLEX,    // complex variable

	SCF_VAR_I8,
	SCF_VAR_I16,
	SCF_VAR_I32,
	SCF_VAR_I64,

	SCF_VAR_U8,
	SCF_VAR_U16,
	SCF_VAR_U32,
	SCF_VAR_U64,

	SCF_VAR_INTPTR,
	SCF_VAR_UINTPTR,

	SCF_FUNCTION_PTR, // function pointer

	SCF_VAR_VEC2,
	SCF_VAR_VEC3,
	SCF_VAR_VEC4,

	SCF_VAR_MAT2x2,
	SCF_VAR_MAT3x3,
	SCF_VAR_MAT4x4,

	SCF_VAR_STRING,     // string variable

	SCF_LABEL,			// label 

	SCF_FUNCTION,		// function

	SCF_STRUCT,			// struct type defined by user
};

static int scf_type_is_integer(int type)
{
	return SCF_VAR_INT == type
		|| (type >= SCF_VAR_I8 && type <= SCF_FUNCTION_PTR);
}

static int scf_type_is_number(int type)
{
	return type >= SCF_VAR_CHAR && type <= SCF_VAR_MAT4x4;
}

static int scf_type_is_var(int type)
{
	return (type >= SCF_VAR_CHAR && type <= SCF_VAR_STRING) || type >= SCF_STRUCT;
}

static int scf_type_is_operator(int type)
{
	return type >= SCF_OP_ADD && type <= SCF_OP_GOTO;
}

#endif

