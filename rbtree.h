#ifndef __INCLUDED_RBTREE_H__
#define __INCLUDED_RBTREE_H__

struct rb_tree;

/* Structure that represents a node in an rb-tree */
struct rb_tree_node {
    struct rb_tree_node *left, *right, *parent;
    void *key;
    int color;
};

typedef int rb_result_t;

/* Pointer to a function to compare two keys, and returns as follows:
 *  1 if lhs > rhs
 *  0 if lhs == rhs
 * -1 if lhs < rhs
 */
typedef int (*rb_cmp_func_t)(void *lhs, void *rhs);

/* Function to clean up a value (free memory) */
typedef void (*rb_node_cleanup_t)(struct rb_tree_node *value);

/* Actual structure representing an RB tree */
struct rb_tree {
    struct rb_tree_node *root;
    rb_cmp_func_t compare;
};

/* Legal result codes */
#define RB_OK           0x0
#define RB_NOT_FOUND    0x1     /** Element was not found */
#define RB_NO_MEM       0x2     /** Ran out of memory (only happens in rb_tree_new) */
#define RB_BAD_ARG      0x3     /** Bad argument provided to function (typically unexpected NULL) */
#define RB_DUPLICATE    0x4     /** Node is a duplicate of an existing node */

/* Create a new Red-Black tree */
rb_result_t rb_tree_new(struct rb_tree *tree,
                        rb_cmp_func_t compare);

/* Destroy a Red-Black tree */
rb_result_t rb_tree_destroy(struct rb_tree *tree,
                            rb_node_cleanup_t cleanup);

/* Check if the Red-Black tree is empty */
rb_result_t rb_tree_empty(struct rb_tree *tree, int *is_empty);

/* Retrieve an element from the Red-Black tree */
rb_result_t rb_tree_find(struct rb_tree *tree,
                         void *key,
                         struct rb_tree_node **value);

/* Insert an element into the Red-Black tree */
rb_result_t rb_tree_insert(struct rb_tree *tree,
                           struct rb_tree_node *node);

/* Remove an element from the Red-Black tree */
rb_result_t rb_tree_remove(struct rb_tree *tree,
                           struct rb_tree_node *node);

#endif /* __INCLUDED_RBTREE_H__ */

