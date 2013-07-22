/*
  Copyright (c) 2011, Phil Vachon <phil@cowpig.ca>
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <rbtree.h>

#define COLOR_BLACK         0x0
#define COLOR_RED           0x1

#define TEST_ASSERT(x) \
    do {                            \
        if (!(x)) {                 \
            fprintf(stderr, "%s:%d - Assertion failure: " #x " == FALSE\n", __FILE__, __LINE__); \
            return -1;              \
        }                           \
    } while (0)

#define TEST_ASSERT_EQUALS(x, y) \
    do {                            \
        if (!((x) == (y))) {        \
            fprintf(stderr, "%s:%d - Equality assertion failed: " #x " != " #y "\n", __FILE__, __LINE__); \
            return -1;              \
        }                           \
    } while (0)

#define TEST_ASSERT_NOT_EQUALS(x, y) \
    do {                            \
        if (!((x) != (y))) {        \
            fprintf(stderr, "%s:%d - Equality assertion failed: " #x " == " #y "\n", __FILE__, __LINE__); \
            return -1;              \
        }                           \
    } while (0)

#define ARRAY_LEN(x)        ((sizeof((x))/sizeof((*x))))

/**
 * Sample item that one might store in a red-black tree.
 */
struct test_rbtree_node {
    /** Red-black tree node data */
    struct rb_tree_node node;
    /** Some satellite data for a node */
    int test;
};

static
int rbtree_assert(struct rb_tree *my_tree, struct test_rbtree_node *nodes,
                  size_t node_count)
{
    int prev_black_height = 0;

    for (size_t i = 0; i < node_count; ++i) {
        struct rb_tree_node *node = &(nodes[i].node);
        struct rb_tree_node *parent = node->parent;
        struct rb_tree_node *left = node->left;
        struct rb_tree_node *right = node->right;
        struct rb_tree_node *tmp_node = &(nodes[i].node);
        int height = 0;
        int black_height = 0;

        if (parent == NULL && left == NULL && right == NULL) {
            continue;
        }

        if (parent == NULL) {
            TEST_ASSERT_EQUALS(node->color, COLOR_BLACK);
        }
        
        if (node->color == COLOR_RED) {
            TEST_ASSERT((!left || left->color == COLOR_BLACK) && (!right || right->color == COLOR_BLACK));
        } else {
            TEST_ASSERT_EQUALS(node->color, COLOR_BLACK);
        } 

        if (left == NULL || right == NULL) {
            while (tmp_node != NULL) {
                height++;
                if (tmp_node->color == COLOR_BLACK) {
                    black_height++;
                }
                tmp_node = tmp_node->parent;
            }
            TEST_ASSERT((prev_black_height == 0) || (black_height == prev_black_height));
            prev_black_height = black_height;
        }

    }

    return 0;
}

static
int test_rbtree_compare(void *lhs, void *rhs)
{
    int64_t n_lhs = (int64_t)lhs;
    int64_t n_rhs = (int64_t)rhs;

    int64_t delta = n_lhs - n_rhs;

    int ret = 0;

    if (delta > 0) {
        ret = 1;
    } else if (delta < 0) {
        ret = -1;
    }

    return ret;
}

static
void test_rbtree_print(struct rb_tree_node *node)
{
    int64_t val = (int64_t)(node->key);
    printf("%d", (int)val);
}

static
void rbtree_print(struct rb_tree *tree, struct test_rbtree_node *nodes, size_t node_count)
{
    printf("digraph TreeDump {\n");
    for (size_t i = 0; i < node_count; ++i) {
        struct rb_tree_node *node = &(nodes[i].node);
        struct rb_tree_node *left = node->left;
        struct rb_tree_node *right = node->right;

        if (node->left == NULL && node->right == NULL && node->parent == NULL) {
            test_rbtree_print(node);
            printf("[color=blue, style=filled];\n");
            continue;
        }

        test_rbtree_print(node);
        printf("[color=%s, style=dotted, shape=%s];\n",
                node->color == COLOR_RED ? "red" : "black",
                node == tree->root ? "doublecircle" : "circle");

        test_rbtree_print(node);
        printf(" -> ");
        if (left) {
            test_rbtree_print(left);
        } else {
            printf("nil");
        }
        printf("[label=left];\n");

        test_rbtree_print(node);
        printf(" -> ");
        if (right) {
            test_rbtree_print(right);
        } else {
            printf("nil");
        }
        printf("[label=right];\n");
    }
    printf("}\n");

}

int test_rbtree_lifecycle(size_t num_nodes)
{
    struct rb_tree my_tree;

    TEST_ASSERT_EQUALS(rb_tree_new(&my_tree, test_rbtree_compare), RB_OK);
    TEST_ASSERT_EQUALS(rb_tree_destroy(&my_tree), RB_OK);

    TEST_ASSERT_EQUALS(rb_tree_new(&my_tree, test_rbtree_compare), RB_OK);

    struct test_rbtree_node *nodes = (struct test_rbtree_node *)calloc(num_nodes, sizeof(*nodes));
    TEST_ASSERT_NOT_EQUALS(nodes, NULL);

    for (size_t i = 0; i < num_nodes; ++i) {
        void *key = (void*)( ((int64_t)i) +  ((i % 2) ? 42 : -42));
        TEST_ASSERT_EQUALS(rb_tree_insert(&my_tree, key, &(nodes[i].node)), RB_OK);
        if (rbtree_assert(&my_tree, nodes, num_nodes)) {
            rbtree_print(&my_tree, nodes, num_nodes);
            fprintf(stderr, "ERROR: tree is invalid after pseudo-random creation at node %zu.\n", i);
            return -1;
        }
    }

    for (size_t i = 0; i < num_nodes; i += 3) {
        TEST_ASSERT_EQUALS(rb_tree_remove(&my_tree, &(nodes[i].node)), RB_OK);
        if (rbtree_assert(&my_tree, nodes, num_nodes)) {
            rbtree_print(&my_tree, nodes, num_nodes);
            fprintf(stderr, "ERROR: tree is invalid after deletion of node %zu\n", i);
            return -1;
        }
    }

    free(nodes);

    return 0;
}

#define TEST_CASE(x) \
    do {                \
        if ((x)) {                          \
            fprintf(stderr, "Failure in " #x "\n");  \
            failures++;                     \
        }                                   \
    } while (0)

int main(int argc, char *argv[])
{
    int failures = 0;
    int count = 512;

    if (argc == 2) {
        count = atoi(argv[1]);
    }

    fprintf(stderr, "Testing for %d iterations.\n", count);

    for (int i = 1; i < count; i++) {
        if (test_rbtree_lifecycle(i) < 0) {
            fprintf(stderr, "Test failure: %d nodes.\n", i);
        }
    }

    fprintf(stderr, "Tests complete. %d failures.\n", failures);

    return EXIT_SUCCESS;
}

