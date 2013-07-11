#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <rbtree.h>

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

struct test_rbtree_node {
    struct rb_tree_node node;
    int test;
};

#define COLOR_BLACK         0x0
#define COLOR_RED           0x1

static
int rbtree_assert(struct test_rbtree_node *nodes, size_t num_nodes)
{
    for (size_t i = 0; i < num_nodes; ++i) {
        if (nodes[i].node.color == COLOR_RED) {
            struct rb_tree_node *left = nodes[i].node.left;
            struct rb_tree_node *right = nodes[i].node.right;

            TEST_ASSERT((!left || left->color == COLOR_BLACK) && (!right || right->color == COLOR_BLACK));
            if (left) {
                TEST_ASSERT(test_rbtree_compare(left->key, nodes[i].node.key) < 0);
            }
            if (right) {
                TEST_ASSERT(test_rbtree_compare(right->key, nodes[i].node.key) > 0);
            }
        }
    }

    return 0;
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
        printf(";\n");

        test_rbtree_print(node);
        printf(" -> ");
        if (right) {
            test_rbtree_print(right);
        } else {
            printf("nil");
        }
        printf(";\n");

#if 0
        if (node->parent != NULL) {
            test_rbtree_print(node);
            printf(" -> ");
            test_rbtree_print(node->parent);
            printf("[style=dashed]\n");
        }
#endif
    }
    printf("}\n");

}

int test_rbtree_lifecycle(size_t num_nodes)
{
    struct rb_tree my_tree;

    TEST_ASSERT_EQUALS(rb_tree_new(&my_tree, test_rbtree_compare), RB_OK);
    TEST_ASSERT_EQUALS(rb_tree_destroy(&my_tree, NULL), RB_OK);

    TEST_ASSERT_EQUALS(rb_tree_new(&my_tree, test_rbtree_compare), RB_OK);

    struct test_rbtree_node *nodes = (struct test_rbtree_node *)calloc(num_nodes, sizeof(*nodes));
    TEST_ASSERT_NOT_EQUALS(nodes, NULL);

    for (size_t i = 0; i < num_nodes; ++i) {
        nodes[i].node.key = (void*)( ((int64_t)i) +  ((i % 2) ? 42 : -42));
        TEST_ASSERT_EQUALS(rb_tree_insert(&my_tree, &(nodes[i].node)), RB_OK);
        if (rbtree_assert(nodes, num_nodes)) {
            rbtree_print(&my_tree, nodes, num_nodes);
            fprintf(stderr, "ERROR: tree is invalid after pseudo-random creation at node %zu.\n", i);
            return -1;
        }
    }

    for (size_t i = 0; i < num_nodes; i += 3) {
        TEST_ASSERT_EQUALS(rb_tree_remove(&my_tree, &(nodes[i].node)), RB_OK);
        if (rbtree_assert(nodes, num_nodes)) {
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
        fprintf(stderr, "Testing with %d nodes.\n", i);
        TEST_CASE(test_rbtree_lifecycle(i));
    }

    fprintf(stderr, "Tests complete. %d failures.\n", failures);

    return EXIT_SUCCESS;
}

