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
    struct node *last_node_root = vector_empty(node_vector) ? NULL : vector_back_ptr_or_null(node_vector_root);

    vector_pop(node_vector);

    if (last_node == last_node_root)
    { // ensures there is no duplicates
        vector_pop(node_vector_root);
    }
    return last_node;
}
bool node_is_expressionable(struct node* node){ //NOdes that can be put in an expression
    return node->type == NODE_TYPE_EXPRESSION || node->type == NODE_TYPE_EXPRESSION_PARANTHESES || node->type == NODE_TYPE_UNARY || node->type == NODE_TYPE_IDENTIFIER ||node->type == NODE_TYPE_NUMBER || node->type == NODE_TYPE_STRING;
}

struct node* node_peek_expressionable_or_null(){
    struct node* last_node = node_peek_or_null();
    return node_is_expressionable(last_node) ? last_node : NULL; //checks if node can be put in an expression (function could not for example)
}
void make_exp_node(struct node* left_node, struct node* right_node, const char* op){
    assert(left_node);
    assert(right_node);
    node_create(&(struct node){.type=NODE_TYPE_EXPRESSION, .exp.left=left_node,.exp.right=right_node,.exp.op=op});
}

struct node* node_create(struct node* _node){
    struct node* node = malloc(sizeof(struct node));
    memcpy(node, _node, sizeof(struct node));
    #warning "We should set the binded owner and binded function here"
    node_push(node);
    return node;
}