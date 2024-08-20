#include "compiler.h"
#include <assert.h>
#include "helpers/vector.h"

struct vector *node_vector = NULL;
struct vector *node_vector_root = NULL; // tree vector

void node_set_vector(struct vector *vec, struct vector *root_vec)
{ // is going to be called to use vector functionnalities cause we don't use vectors for pushes etc on nodes
    node_vector = vec;
    node_vector_root = root_vec;
}

void node_push(struct node *node)
{
    vector_push(node_vector, &node); // pushes a node to the back of the vector
}

struct node *node_peek_or_null()
{ // grab from the back of vector
    return vector_back_ptr_or_null(node_vector);
}

struct node *node_peek()
{
    return *(struct node **)(vector_back(node_vector)); // this is what we did for vector_back but just manually now
}

struct node *node_pop()
{ // pops last node
    struct node *last_node = vector_back_ptr(node_vector);
    // if node vector = NUL -> last node root = NULL otherwise equal to the last value of vector
    struct node *last_node_root = vector_empty(node_vector) ? NULL : vector_back_ptr(node_vector_root);

    vector_pop(node_vector);

    if (last_node == last_node_root)
    { // ensures there is no duplicates
        vector_pop(node_vector_root);
    }
    return last_node;
}