#ifndef SCF_EXPR_H
#define SCF_EXPR_H

#include"scf_node.h"

typedef scf_node_t	scf_expr_t; // expr is a node

scf_expr_t*			scf_expr_alloc();
void				scf_expr_free(scf_expr_t* expr);
int					scf_expr_add_node(scf_expr_t* expr, scf_node_t* node);

#endif

