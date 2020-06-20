#ifndef SCF_OPERATOR_H
#define SCF_OPERATOR_H

#include"scf_def.h"
#include"scf_core_types.h"

#define SCF_OP_ASSOCIATIVITY_LEFT	0
#define SCF_OP_ASSOCIATIVITY_RIGHT	1

struct scf_operator_s
{
	int						type;
	char*					name;

	int						priority;
	int						nb_operands;
	int						associativity;
};

scf_operator_t* scf_find_base_operator(const char* name, const int nb_operands);

scf_operator_t* scf_find_base_operator_by_type(const int type);

#endif

