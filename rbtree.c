#include <rbtree.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <stdio.h>


#define COLOR_BLACK         0x0
#define COLOR_RED           0x1

#define RB_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define RB_LIKELY(x)   __builtin_expect(!!(x), 1)

#define RB_UNUSED      __attribute__((unused))

/* Macro to check if a given assertion about an argument is true */
#define RB_ASSERT_ARG(x) \
    do {                                \
        if (RB_UNLIKELY(!(x))) {        \
            assert((x));                \
            return RB_BAD_ARG;          \
        }                               \
    } while (0)

/**
 * \brief Allocate and construct a new RB-tree
 * Given a pointer to recieve the new RB-tree, allocate the required memory to
 * store the RB-tree metadata (from the heap) and return an initialized, empty
 * RB-tree.
 * \param tree Reference to a pointer to receive the new tree.
 * \param compare Function to be used to compare if a key is less than another key.
 * \return RB_OK on success, an error code otherwise
 */
rb_result_t rb_tree_new(struct rb_tree *tree,
                        rb_cmp_func_t compare)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);
    RB_ASSERT_ARG(compare != NULL);

    tree->root = NULL;
    tree->compare = compare;

    return result;
}

/**
 * \brief Destroy an RB-tree
 * Given a reference to a pointer to an RB-tree, use the cleanup functions provided to
 * destroy RB-tree items, and clear up the tree.
 * \param tree The reference to the pointer to the tree itself.
 * \param cleanup A function that will take a de-linked rb_tree_node and release its resources
 * \return RB_OK on success, an error code otherwise
 * \note Will set the pointer to the tree to NULL upon completion.
 */
rb_result_t rb_tree_destroy(struct rb_tree *tree,
                            rb_node_cleanup_t cleanup)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);

    if (tree->root != NULL) {
        RB_ASSERT_ARG(cleanup != NULL);

        /* Clean up remaining tree nodes using cleanup */
    }

    memset(tree, 0, sizeof(struct rb_tree));

    return result;
}

/**
 * \brief Check if an RB-tree is empty (has no nodes)
 * \param tree The tree to check
 * \param is_empty nonzero on true, 0 otherwise
 * \return RB_OK on success, an error code otherwise
 */
rb_result_t rb_tree_empty(struct rb_tree *tree,
                          int *is_empty)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);
    RB_ASSERT_ARG(is_empty != NULL);

    *is_empty = (tree->root == NULL);

    return result;
}

/**
 * \brief Find a node in the RB-tree given the specified key.
 * Given a key, search the RB-tree iteratively until the specified key is found.
 * \param tree The RB-tree to search
 * \param key The key to search for
 * \param value a reference to a pointer to receive the pointer to the rb_tree_node if key is found
 * \return RB_OK on success, an error code otherwise
 */
rb_result_t rb_tree_find(struct rb_tree *tree,
                         void *key,
                         struct rb_tree_node **value)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);
    RB_ASSERT_ARG(key != NULL);
    RB_ASSERT_ARG(value != NULL);

    *value = NULL;

    if (RB_UNLIKELY(tree->root == NULL)) {
        result = RB_NOT_FOUND;
        goto done;
    }

    struct rb_tree_node *node = tree->root;

    while (node != NULL) {
        int compare = tree->compare(key, node->key);

        if (compare < 0) {
            node = node->left;
        } else if (compare == 0) {
            break; /* We found our node */
        } else {
            /* Otherwise, we want the right node, and continue iteration */
            node = node->right;
        }
    }

    if (node == NULL) {
        result = RB_NOT_FOUND;
        goto done;
    }

    /* Return the node we found */
    *value = node;

done:
    return result;
}

/* Helper function to get a node's sibling */
static inline
struct rb_tree_node *__helper_get_sibling(struct rb_tree_node *node)
{
    if (node->parent == NULL) {
        return NULL;
    }

    struct rb_tree_node *parent = node->parent;

    if (node == parent->left) {
        return parent->right;
    } else {
        return parent->left;
    }
}

/* Helper function to get a node's grandparent */
static inline
struct rb_tree_node *__helper_get_grandparent(struct rb_tree_node *node)
{
    if (node->parent == NULL) {
        return NULL;
    }

    struct rb_tree_node *parent_node = node->parent;

    return parent_node->parent;
}

/* Helper function to get a node's uncle */
static inline
struct rb_tree_node *__helper_get_uncle(struct rb_tree_node *node)
{
    struct rb_tree_node *grandparent = __helper_get_grandparent(node);

    if (grandparent == NULL) {
        return NULL;
    }

    if (node->parent == grandparent->left) {
        return grandparent->right;
    } else {
        return grandparent->left;
    }
}

/* Helper function to do a left rotation of a given node */
static inline
void __helper_rotate_left(struct rb_tree *tree,
                          struct rb_tree_node *node)
{
    struct rb_tree_node *x = node;
    struct rb_tree_node *y = x->right;

    x->right = y->left;

    if (y->left != NULL) {
        struct rb_tree_node *yleft = y->left;
        yleft->parent = x;
    }

    y->parent = x->parent;

    if (x->parent == NULL) {
        tree->root = y;
    } else {
        struct rb_tree_node *xp = x->parent;
        if (x == xp->left) {
            xp->left = y;
        } else {
            xp->right = y;
        }
    }

    y->left = x;
    x->parent = y;
}

/* Helper function to do a right rotation of a given node */
static inline
void __helper_rotate_right(struct rb_tree *tree,
                           struct rb_tree_node *node)
{
    struct rb_tree_node *x = node;
    struct rb_tree_node *y = x->left;

    x->left = y->right;

    if (y->right != NULL) {
        struct rb_tree_node *yright = y->right;
        yright->parent = x;
    }

    y->parent = x->parent;

    if (x->parent == NULL) {
        tree->root = y;
    } else {
        struct rb_tree_node *xp = x->parent;
        if (x == xp->left) {
            xp->left = y;
        } else {
            xp->right = y;
        }
    }

    y->right = x;
    x->parent = y;
}

/* Function to perform a RB tree rebalancing after an insertion */
static
void __helper_rb_tree_insert_rebalance(struct rb_tree *tree,
                                       struct rb_tree_node *node)
{
    struct rb_tree_node *new_node_parent = node->parent;

    if (new_node_parent != NULL && new_node_parent->color != COLOR_BLACK) {
        struct rb_tree_node *pnode = node;

        /* Iterate until we're at the root (which we just color black) or 
         * until we the parent node is no longer red.
         */
        while ((tree->root != pnode) && (pnode->parent != NULL) &&
                    (pnode->parent->color == COLOR_RED))
        {
            struct rb_tree_node *parent = pnode->parent;
            struct rb_tree_node *grandparent = __helper_get_grandparent(pnode);
            struct rb_tree_node *uncle = NULL;
            int uncle_is_left;

            assert(pnode->color == COLOR_RED);

            if (parent == grandparent->left) {
                uncle_is_left = 0;
                uncle = grandparent->right;
            } else {
                uncle_is_left = 1;
                uncle = grandparent->left;
            }

            /* Case 1: Uncle is not black */
            if (uncle && uncle->color == COLOR_RED) {
                /* Color parent and uncle black */
                parent->color = COLOR_BLACK;
                uncle->color = COLOR_BLACK;

                /* Color Grandparent as Black */
                grandparent->color = COLOR_RED;
                pnode = grandparent;
                /* Continue iteration, processing grandparent */
            } else {
                /* Case 2 - node's parent is red, but uncle is black */
                if (!uncle_is_left && parent->right == pnode) {
                    pnode = pnode->parent;
                    __helper_rotate_left(tree, pnode);
                } else if (uncle_is_left && parent->left == pnode) {
                    pnode = pnode->parent;
                    __helper_rotate_right(tree, pnode);
                }

                /* Case 3 - Recolor and rotate*/
                parent = pnode->parent;
                parent->color = COLOR_BLACK;

                grandparent = __helper_get_grandparent(pnode);
                grandparent->color = COLOR_RED;
                if (!uncle_is_left) {
                    __helper_rotate_right(tree, grandparent);
                } else {
                    __helper_rotate_left(tree, grandparent);
                }
            }
        }

        /* Make sure the tree root is black (Case 1: Continued) */
        struct rb_tree_node *tree_root = tree->root;
        tree_root->color = COLOR_BLACK;
    }
}

/**
 * \brief Insert a node into the tree
 * Given a node with a populated key, insert the node into the RB-tree
 * \param tree the RB tree to insert the node into
 * \param node the node to be inserted into the tree
 * \return RB_OK on sucess, an error code otherwise
 */
rb_result_t rb_tree_insert(struct rb_tree *tree,
                           struct rb_tree_node *node)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);
    RB_ASSERT_ARG(node != NULL);

    /* Case 1: Simplest case -- tree is empty */
    if (RB_UNLIKELY(tree->root == NULL)) {
        tree->root = node;
        node->color = COLOR_BLACK;
        goto done;
    }

    /* Otherwise, insert the node as you would typically in a BST */
    struct rb_tree_node *nd = tree->root;
    node->color = COLOR_RED;

    /* Insert a node into the tree as you normally would */
    while (nd != NULL) {
        int compare = tree->compare(node->key, nd->key);

        if (compare == 0) {
            result = RB_DUPLICATE;
            goto done;
        }

        if (compare < 0) {
            if (nd->left == NULL) {
                nd->left = node;
                break;
            } else {
                nd = nd->left;
            }
        } else {
            if (nd->right == NULL) {
                nd->right = node;
                break;
            } else {
                nd = nd->right;
            }
        }
    }

    node->parent = nd;

    /* Rebalance the tree about the node we just added */
    __helper_rb_tree_insert_rebalance(tree, node);

done:
    return result;
}

/**
 * Find the minimum of the subtree starting at node
 */
static
struct rb_tree_node *__helper_rb_tree_find_minimum(struct rb_tree_node *node)
{
    struct rb_tree_node *x = node;

    while (x->left != NULL) {
        x = x->left;
    }

    return x;
}

static
struct rb_tree_node *__helper_rb_tree_find_successor(struct rb_tree_node *node)
{
    struct rb_tree_node *x = node;

    if (x->right != NULL) {
        return __helper_rb_tree_find_minimum(x->right);
    }

    struct rb_tree_node *y = x->parent;

    while (y != NULL && x == y->right) {
        x = y;
        y = y->parent;
    }

    return y;
}

/* Replace x with y, inserting y where x previously was */
static
void __helper_rb_tree_swap_node(struct rb_tree *tree,
                                struct rb_tree_node *x,
                                struct rb_tree_node *y)
{
    struct rb_tree_node *left = x->left;
    struct rb_tree_node *right = x->right;
    struct rb_tree_node *parent = x->parent;

    y->parent = parent;

    if (parent != NULL) {
        if (parent->left == x) {
            parent->left = y;
        } else {
            parent->right = y;
        }
    } else {
        if (tree->root == x) {
            tree->root = y;
        }
    }

    y->right = right;
    if (right != NULL) {
        right->parent = y;
    }
    x->right = NULL;

    y->left = left;
    if (left != NULL) {
        left->parent = y;
    }
    x->left = NULL;

    y->color = x->color;
    x->parent = NULL;
}

static
void __helper_rb_tree_delete_rebalance(struct rb_tree *tree,
                                       struct rb_tree_node *node,
                                       struct rb_tree_node *parent,
                                       int node_is_left)
{
    struct rb_tree_node *x = node;
    struct rb_tree_node *xp = parent;
    int is_left = node_is_left;

    while (x != tree->root && (x == NULL || x->color == COLOR_BLACK)) {
        struct rb_tree_node *w = is_left ? xp->right : xp->left;    /* Sibling */

        if (w != NULL && w->color == COLOR_RED) {
            /* Case 1: */
            w->color = COLOR_BLACK;
            xp->color = COLOR_RED;
            if (is_left) {
                __helper_rotate_left(tree, xp);
            } else {
                __helper_rotate_right(tree, xp);
            }
            w = is_left ? xp->right : xp->left;
        }

        struct rb_tree_node *wleft = w != NULL ? w->left : NULL;
        struct rb_tree_node *wright = w != NULL ? w->right : NULL;
        if ( (wleft == NULL || wleft->color == COLOR_BLACK) &&
             (wright == NULL || wright->color == COLOR_BLACK) )
        {
            /* Case 2: */
            if (w != NULL) {
                w->color = COLOR_RED;
            }
            x = xp;
            xp = x->parent;
            is_left = xp && (x == xp->left);
        } else {
            if (is_left && (wright == NULL || wright->color == COLOR_BLACK)) {
                /* Case 3a: */
                w->color = COLOR_RED;
                if (wleft) {
                    wleft->color = COLOR_BLACK;
                }
                __helper_rotate_right(tree, w);
                w = xp->right;
            } else if (!is_left && (wleft == NULL || wleft->color == COLOR_BLACK)) {
                /* Case 3b: */
                w->color = COLOR_RED;
                if (wright) {
                    wright->color = COLOR_BLACK;
                }
                __helper_rotate_left(tree, w);
                w = xp->left;
            }

            /* Case 4: */
            wleft = w->left;
            wright = w->right;

            w->color = xp->color;
            xp->color = COLOR_BLACK;

            if (is_left && wright != NULL) {
                wright->color = COLOR_BLACK;
                __helper_rotate_left(tree, xp);
            } else if (!is_left && wleft != NULL) {
                wleft->color = COLOR_BLACK;
                __helper_rotate_right(tree, xp);
            }
            x = tree->root;
        }
    }

    if (x != NULL) {
        x->color = COLOR_BLACK;
    }
}


static RB_UNUSED
void rb_tree_dump_node(const char *label, struct rb_tree_node *node)
{
    fprintf(stderr, "%s Node: %p (left = %p, right = %p)\n",
            label, node->key,
            (node->left ? node->left->key : NULL),
            (node->right ? node->right->key : NULL));
}

/**
 * \brief Remove the specified key from the rb_tree
 * Removes a specified key from the rb_tree and returns a pointer to the node.
 * \param tree The tree we want to remove the node from
 * \param key The key for the node we want to find and remove
 * \param removed The node we just removed
 * \return RB_OK on success, an error code otherwise
 */
rb_result_t rb_tree_remove(struct rb_tree *tree,
                           struct rb_tree_node *node)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);
    RB_ASSERT_ARG(node != NULL);

    struct rb_tree_node *y;

    if (node->left == NULL || node->right == NULL) {
        y = node;
    } else {
        y = __helper_rb_tree_find_successor(node);
    }

    struct rb_tree_node *x, *xp;

    if (y->left != NULL) {
        x = y->left;
    } else {
        x = y->right;
    }

    if (x != NULL) {
        x->parent = y->parent;
        xp = x->parent;
    } else {
        xp = y->parent;
    }

    int is_left = 0;
    if (y->parent == NULL) {
        tree->root = x;
        xp = NULL;
    } else {
        struct rb_tree_node *yp = y->parent;
        if (y == yp->left) {
            yp->left = x;
            is_left = 1;
        } else {
            yp->right = x;
            is_left = 0;
        }
    }

    /* Swap in the node */
    if (y != node) {
        __helper_rb_tree_swap_node(tree, node, y);
        if (xp == node) {
            xp = y;
        }
    }

    if (y->color == COLOR_BLACK) {
        __helper_rb_tree_delete_rebalance(tree, x, xp, is_left);
    }

    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;

    return result;
}

