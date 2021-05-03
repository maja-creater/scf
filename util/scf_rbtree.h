#ifndef SCF_RBTREE_H
#define SCF_RBTREE_H

#include"scf_def.h"

typedef struct scf_rbtree_s            scf_rbtree_t;
typedef struct scf_rbtree_node_s       scf_rbtree_node_t;

typedef int  (*scf_rbtree_node_do_pt )(scf_rbtree_node_t* node0, void* data);

struct scf_rbtree_node_s
{
	scf_rbtree_node_t*  parent;

	scf_rbtree_node_t*  left;
	scf_rbtree_node_t*  right;

	uint16_t            depth;
	uint16_t            bdepth;
	uint8_t             color;
};

struct scf_rbtree_s
{
	scf_rbtree_node_t*  root;
	scf_rbtree_node_t   sentinel;
};

#define SCF_RBTREE_BLACK 0
#define SCF_RBTREE_RED   1

#define scf_rbtree_sentinel(tree) (&tree->sentinel)

static inline void scf_rbtree_init(scf_rbtree_t* tree)
{
#if 0
	tree->sentinel.parent = NULL;
	tree->sentinel.left   = NULL;
	tree->sentinel.right  = NULL;
#else
	tree->sentinel.parent = &tree->sentinel;
	tree->sentinel.left   = &tree->sentinel;
	tree->sentinel.right  = &tree->sentinel;
#endif
	tree->sentinel.color  = SCF_RBTREE_BLACK;

	tree->root = &tree->sentinel;
}

int                scf_rbtree_insert (scf_rbtree_t* tree, scf_rbtree_node_t* node, scf_rbtree_node_do_pt cmp);
int                scf_rbtree_delete (scf_rbtree_t* tree, scf_rbtree_node_t* node);

scf_rbtree_node_t* scf_rbtree_min    (scf_rbtree_t* tree, scf_rbtree_node_t* root);
scf_rbtree_node_t* scf_rbtree_max    (scf_rbtree_t* tree, scf_rbtree_node_t* root);

scf_rbtree_node_t* scf_rbtree_find   (scf_rbtree_t* tree, void* data, scf_rbtree_node_do_pt cmp);

int                scf_rbtree_foreach(scf_rbtree_t* tree, scf_rbtree_node_t* root, void* data, scf_rbtree_node_do_pt done);

int                scf_rbtree_foreach_reverse(scf_rbtree_t* tree, scf_rbtree_node_t* root, void* data, scf_rbtree_node_do_pt done);


#endif

