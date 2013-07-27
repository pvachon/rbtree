#ifndef __INCLUDED_RBTREE_H__
#define __INCLUDED_RBTREE_H__

/** \file rbtree.h
 * Declaration of associated structures and functions for a simple, intrusive
 * red-black tree implementation.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** \defgroup rb_tree_state State Structures
 * Structures that are used to represent state of a red-black tree, including the
 * state of the tree itself, comparison functions used to determine how the tree
 * is to be traversed, and representations of red-black tree nodes themselves.
 * @{
 */

/**
 * Structure that represents a node in a red-black tree. Embed this in your own
 * structure in order to add your structure to the given red-black tree.
 * Users of the rb_tree_node would embed it something like
 * \code{.c}
    struct my_sample_struct {
        char *name;
        int data;
        struct rb_tree_node rnode;
    };
 * \endcode
 *
 * \note No user of `struct rb_tree_node` should ever modify or inspect any
 *       members of the structure.
 */
struct rb_tree_node {
    /**
     * The left child (`NULL` if empty)
     */
    struct rb_tree_node *left;

    /** 
     * The right child (`NULL` if empty)
     */
    struct rb_tree_node *right;

    /**
     * The parent of this node (`NULL` if at root)
     */
    struct rb_tree_node *parent;

    /**
     * The key for this node
     */
    void *key;

    /**
     * The color of the node
     */
    int color;
};

/**
 * Pointer to a function to compare two keys, and returns as follows:
 *  (0, +inf] if lhs > rhs
 *  0 if lhs == rhs
 *  [-inf, 0) if lhs < rhs
 */
typedef int (*rb_cmp_func_t)(const void *lhs, const void *rhs);

/**
 * Structure representing an RB tree's associated state. Contains all
 * the information needed to manage the lifecycle of a RB tree.
 * \note Typically users should not directly manipulate the structure,
 *       but rather use the provided accessor functions.
 */
struct rb_tree {
    /**
     * The root of the tree
     */
    struct rb_tree_node *root;

    /**
     * Predicate used for traversing the tree
     */
    rb_cmp_func_t compare;
};

/**@} rb_tree_state */

/** \defgroup rb_result Function Results and Error Handling
 * @{
 */
/** \typedef rb_result_t
 * Value of a returned result code from a red-black tree function.
 */
typedef int rb_result_t;

/** \defgroup rb_result_code Result Codes
 * Error codes that can be returned from any function that returns an rb_result_t.
 * @{
 */

/**
 * Function was successful
 */
#define RB_OK           0x0
/**
 * Element was not found
 */
#define RB_NOT_FOUND    0x1
/**
 * Bad argument provided to function (typically unexpected NULL)
 */
#define RB_BAD_ARG      0x2
/**
 * Node is a duplicate of an existing node
 */
#define RB_DUPLICATE    0x3

/**@} rb_result_code */
/**@} rb_result */

/** \brief Helper to get a pointer to a containing structure
 * Given a pointer to an rb_tree_node, a target type and a member name,
 * return a pointer to the structure containing the `struct rb_tree_node`.
 * \code{.c}
    struct sample {
        const char *name;
        struct rb_tree_node node;
    };

    void test(void)
    {
        struct sample samp = { .name = "Test 123" };
        struct rb_tree_node *samp_node = &(samp.node);
        struct sample *samp2 = RB_CONTAINER_OF(samp_node, struct sample, node);

        assert(&samp == samp2);
    }
 * \endcode
 * \param x The pointer to the node
 * \param type The type of the containing structure
 * \param memb The name of the `struct rb_tree_node` in the containing structure
 * \return Pointer to the containing structure of the specified type
 */
#define RB_CONTAINER_OF(x, type, memb) \
    ({                                                              \
        const __typeof__( ((type *)0)->memb ) *__member = (x);      \
        (type *)( (char *)__member - __offsetof__(type, memb) );    \
    })


/** \defgroup rb_functions Functions for Manipulating Red-Black Trees
 * All functions associated with manipulating Red-Black trees using `struct rb_tree`,
 * inluding lifecycle functions and member manipulation and state checking functions.
 * @{
 */
/**
 * \brief Construct a new, empty red-black tree
 * Given a region of memory at least the size of a struct rb_tree to
 * store the red-black tree metadata, update it to contain an initialized, empty
 * red-black tree.
 * \param tree Pointer to the new tree.
 * \param compare Function used to traverse the tree.
 * \return RB_OK on success, an error code otherwise
 */
rb_result_t rb_tree_new(struct rb_tree *tree,
                        rb_cmp_func_t compare);

/**
 * \brief Destroy an RB-tree
 * Erase an RB-tree.
 * \param tree The reference to the pointer to the tree itself.
 * \return RB_OK on success, an error code otherwise
 */
rb_result_t rb_tree_destroy(struct rb_tree *tree);

/**
 * \brief Check if an RB-tree is empty (has no nodes)
 * \param tree The tree to check
 * \param is_empty nonzero on true, 0 otherwise
 * \return RB_OK on success, an error code otherwise
 */
rb_result_t rb_tree_empty(struct rb_tree *tree, int *is_empty);

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
                         struct rb_tree_node **value);

/**
 * \brief Insert a node into the tree
 * Given a node with a populated key, insert the node into the RB-tree
 * \param tree the RB tree to insert the node into
 * \param key The key for the node (must live as long as the node itself is in the tree)
 * \param node the node to be inserted into the tree
 * \return RB_OK on sucess, an error code otherwise
 */
rb_result_t rb_tree_insert(struct rb_tree *tree,
                           void *key,
                           struct rb_tree_node *node);

/**
 * \brief Remove the specified node from the rb_tree
 * Removes a specified node from the red-black tree.
 * \param tree The tree we want to remove the node from
 * \param node The the node we want to remove
 * \return RB_OK on success, an error code otherwise
 */
rb_result_t rb_tree_remove(struct rb_tree *tree,
                           struct rb_tree_node *node);

/**@} rb_functions */

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* __INCLUDED_RBTREE_H__ */

