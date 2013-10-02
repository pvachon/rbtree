/*
  Copyright (c) 2013, Phil Vachon <phil@cowpig.ca>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  - Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/** \defgroup rb_tree_implementation Implementation Details
 * All the implementation details for the red-black tree, including functions for
 * the maintenance of tree properties.
 * @{
 */

/** \file rbtree.c
 * An implementation of an intrusive red-black self-balancing tree, that can
 * be used to implement red-black trees in situations where memory allocation
 * is not an option.
 *
 * This file exclusively contains implementation details for the red-black tree, so
 * probably is not of much interest to most people.
 *
 * \see rbtree.h
 * \see rb_tree
 * \see rb_tree_node
 */

#include <rbtree.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

/** \defgroup rb_tree_colors Colors for the red-black tree nodes
 * @{
 */

/**
 * Node is black
 */
#define COLOR_BLACK         0x0

/**
 * Node is red
 */
#define COLOR_RED           0x1
/**@}*/

/** \defgroup rb_tree_compiler_prims Compiler Abstractions
 * Primitives used to abstract compiler-specific syntax for common details used in
 * providing hints to the compiler for optimization or linker details.
 * @{
 */

/**
 * The tagged branch is unlikely to be taken
 */
#define RB_UNLIKELY(x) __builtin_expect(!!(x), 0)

/**
 * The tagged branch is likely to be taken
 */
#define RB_LIKELY(x)   __builtin_expect(!!(x), 1)

/**
 * Indicate that the tagged item can legally be unused in the compilation unit and
 * that the compiler should not generate a warning
 */
#define RB_UNUSED      __attribute__((unused))
/**@}*/

/**
 * Macro to check if a given assertion about an argument is true
 */
#define RB_ASSERT_ARG(x) \
    do {                                \
        if (RB_UNLIKELY(!(x))) {        \
            assert(#x && 0);            \
            return RB_BAD_ARG;          \
        }                               \
    } while (0)

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

rb_result_t rb_tree_destroy(struct rb_tree *tree)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);

    memset(tree, 0, sizeof(struct rb_tree));

    return result;
}

rb_result_t rb_tree_empty(struct rb_tree *tree,
                          int *is_empty)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);
    RB_ASSERT_ARG(is_empty != NULL);

    *is_empty = !!(tree->root == NULL);

    return result;
}

rb_result_t rb_tree_find(struct rb_tree *tree,
                         const void *key,
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

rb_result_t rb_tree_insert(struct rb_tree *tree,
                           const void *key,
                           struct rb_tree_node *node)
{
    rb_result_t result = RB_OK;

    RB_ASSERT_ARG(tree != NULL);
    RB_ASSERT_ARG(node != NULL);

    node->key = key;

    /* Case 1: Simplest case -- tree is empty */
    if (RB_UNLIKELY(tree->root == NULL)) {
        tree->root = node;
        tree->leftmost = node;
        node->color = COLOR_BLACK;
        goto done;
    }

    /* Otherwise, insert the node as you would typically in a BST */
    struct rb_tree_node *nd = tree->root;
    node->color = COLOR_RED;

    int leftmost = 1;

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
            leftmost = 0;
            if (nd->right == NULL) {
                nd->right = node;
                break;
            } else {
                nd = nd->right;
            }
        }
    }

    node->parent = nd;

    /* Update the leftmost node of the tree if appropriate */
    if (1 == leftmost) {
        tree->leftmost = node;
    }

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

rb_result_t rb_tree_remove(struct rb_tree *tree,
                           struct rb_tree_node *node)
{
    rb_result_t result = RB_OK;
    struct rb_tree_node *y;
    struct rb_tree_node *x, *xp;
    int y_color = 0, is_left = 0;

    RB_ASSERT_ARG(tree != NULL);
    RB_ASSERT_ARG(node != NULL);


    if (node->left == NULL || node->right == NULL) {
        y = node;
        if (node == tree->leftmost) {
            /* The new left-most node is our successor */
            tree->leftmost = __helper_rb_tree_find_successor(node);
        }
    } else {
        y = __helper_rb_tree_find_successor(node);
    }

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

    y_color = y->color;

    /* Swap in the node */
    if (y != node) {
        __helper_rb_tree_swap_node(tree, node, y);
        if (xp == node) {
            xp = y;
        }
    }

    if (y_color == COLOR_BLACK) {
        __helper_rb_tree_delete_rebalance(tree, x, xp, is_left);
    }

    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;

    return result;
}

/** @} rb_tree_implementation */

/**
 * \mainpage A Non-Intrusive Red-Black Tree
 *
 * The goal of this implementation is to be both easy to use, but also
 * sufficiently powerful enough to perform all the operations that one might
 * typically want to do with a red-black tree.
 *
 * To make a structure usable with an rb_tree, you must embed the structure
 * struct rb_tree_node. 
 * \code
    struct my_sample_struct {
        const char *name;
        int data;
        struct rb_tree_node rnode;
    };
 * \endcode
 * \note `rb_tree_node` need not be initialized -- it is initialized during the
 *       insertion operation.
 *
 * Next, you must declare a comparison function that, given a pointer to two
 * keys, returns a value less than 0 if the left-hand side is less than the
 * right-hand side, 0 if the left-hand side is equal to the right-hand side,
 * or greater than 0 if the left-hand side is greater than the left-hand side.
 *
 * A simple example for a string might use the `strcmp(3)` function directly,
 * as such:
 *
 * \code
    int my_sample_struct_compare_keys(void *lhs, void *rhs)
    {
        return strcmp((const char *)lhs, (const char *)rhs);
    }
 * \endcode
 * \note the function you create for your comparison function must conform to
 *       rb_cmp_func_t, or the compiler will generate a warning and, if you're
 *       unlucky, you will fail catastrophically at a later date.
 *
 * Then, to create a new, empty red-black tree, call rb_tree_new, as so:
 * \code
    struct rb_tree my_rb_tree;
    if (rb_tree_new(&my_rb_tree, my_sample_struct_compare_keys) != RB_OK) {
        exit(EXIT_FAILURE);
    }
 * \endcode
 *
 * Items can be added to the red-black tree using the function `rb_tree_insert`:
 * \code
    struct my_sample_struct node = { .name = "test1", .date = 42 };
    if (rb_tree_insert(&my_rb_tree, node.name, &(node.rnode)) != RB_OK) {
        printf("Failed to insert a node into the RB tree!\n");
        exit(EXIT_FAILURE);
    }
 * \endcode
 *
 * \see rb_tree
 * \see rb_tree_node
 * \see rb_functions
 * \see rbtree.h
 */

