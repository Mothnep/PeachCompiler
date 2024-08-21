#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>
static struct compile_process *current_process;
static struct token *parser_last_token;
extern struct expressionable_op_precedence_group op_precedence[TOTAL_OPERATOR_GROUPS]; // just referencing

struct history
{
    int flags;
};
struct history *history_begin(int flags)
{
    struct history *history = calloc(1, sizeof(struct history));
    history->flags = flags;
    return history;
}

struct history *history_down(struct history *history, int flags)
{
    struct history *new_history = calloc(1, sizeof(struct history));
    memcpy(new_history, history, sizeof(struct history));
    new_history->flags = flags;
    return new_history;
}

int parse_expressionable_single(struct history *history);
void parse_expressionable(struct history *history);

static void parser_ignore_nl_or_comment(struct token *token)
{
    while (token && token_is_nl_or_comment_or_newline_seperator(token))
    {
        // skip the token
        vector_peek(current_process->token_vec);
        token = vector_peek_no_increment(current_process->token_vec);
    }
}

static struct token *token_next()
{ // gonna grab next token to process
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    current_process->pos = next_token->pos; // Compiler can tell us where an error occured if anything hapenned
    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

static struct token *token_peek_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    return vector_peek_no_increment(current_process->token_vec);
}

void parse_single_token_to_node() // Tokens will be generating Nodes
{
    struct token *token = token_next();
    struct node *node = NULL;
    switch (token->type)
    { // Node_create always pushed nodes to the stack
    case TOKEN_TYPE_NUMBER:
        node = node_create(&(struct node){.type = NODE_TYPE_NUMBER, .llnum = token->llnum});
        break;

    case TOKEN_TYPE_IDENTIFIER:
        node = node_create(&(struct node){.type = NODE_TYPE_IDENTIFIER, .sval = token->sval});
        break;

    case TOKEN_TYPE_STRING:
        node = node_create(&(struct node){.type = NODE_TYPE_STRING, .sval = token->sval});
        break;

    default:
        compiler_error(current_process, "This is not a single token that can be converted to a node");
    }
}

void parse_expressionable_for_op(struct history *history, const char *op)
{

    parse_expressionable(history);
}

static int parser_get_precedence_for_operator(const char *op, struct expressionable_op_precedence_group **group_out)
{
    *group_out = NULL;
    for (int i = 0; i < TOTAL_OPERATOR_GROUPS; i++)
    {
        for (int b = 0; op_precedence[i].operators[b]; b++)
        { // will stop on the NULL in the array
            const char *_op = op_precedence[i].operators[b];
            if (S_EQ(op, _op))
            {
                *group_out = &op_precedence[i];
                return i;
            }
        }
    }
    return -1; // can't find precedence
}
static bool parser_left_op_has_priority(const char *op_left, const char *op_right) //checks if left operator has priority
{
    struct expressionable_op_precedence_group *group_left = NULL;
    struct expressionable_op_precedence_group *group_right = NULL;
    if (S_EQ(op_left, op_right))
    {
        return false;
    }
    int precedence_left = parser_get_precedence_for_operator(op_left, &group_left);
    int precedence_right = parser_get_precedence_for_operator(op_right, &group_right);

    if(group_left->associtivity ==ASSOCIATIVITY_RIGHT_TO_LEFT){//can't handle this here
        return false;
    }
    return precedence_left <= precedence_right;
}

void parser_node_shift_children_left(struct node* node){
    assert(node->type == NODE_TYPE_EXPRESSION);
    assert(node->exp.right->type == NODE_TYPE_EXPRESSION);
    const char* right_op = node->exp.right->exp.op;
    struct node* new_exp_left_node = node->exp.left;
    struct node* new_exp_right_node = node->exp.right->exp.left;
    make_exp_node(new_exp_left_node, new_exp_right_node, node->exp.op);
    struct node* new_left_operand = node_pop();
    struct node* new_right_operand = node->exp.right->exp.right;
    node->exp.left = new_left_operand;
    node->exp.right = new_right_operand;
    node->exp.op = right_op;
}

void parser_reorder_expression(struct node **node_out)
{
    struct node *node = *node_out; // gives direct access to the node
    if (node->type != NODE_TYPE_EXPRESSION)
    {
        return; // Can't deal with no expressions
    } // No expressions, nothing to do
    if (node->exp.left->type != NODE_TYPE_EXPRESSION && node->exp.right && node->exp.right->type != NODE_TYPE_EXPRESSION)
    { // Checks if left node and right node are not expressions
        return;
    }
    // Possible expression if right operand is expression like this 50*E(30+20)
/*
* 50*E(30+20)
* 50*EXPRESSION                     Watch video30 at around 14:00 on TL
* EXPRESSION(50*EXPRESSION) -> "root expression"
* code below is going to try and do (50*30)+20
*
*/

    if (node->exp.left->type != NODE_TYPE_EXPRESSION && node->exp.right && node->exp.right->type == NODE_TYPE_EXPRESSION)
    {
        const char *right_op = node->exp.right->exp.op;
        if(parser_left_op_has_priority(node->exp.op, right_op)){ /* 50 in example above is not an expression,
        so grabbing the '*' with node->exp.op and the '+' will be putt in rightt_op */
            
            parser_node_shift_children_left(node);

            parser_reorder_expression(&node->exp.left);
            parser_reorder_expression(&node->exp.right);
        }
    }
}

void parse_exp_normal(struct history *history) // Normal exp is like 30+10
{                                              // If lost watch video nb 28 at around 15:00 on the TL
    struct token *op_token = token_peek_next();
    const char *op = op_token->sval;
    struct node *node_left = node_peek_expressionable_or_null(); // peeks to last node pushed to stack
    if (!node_left)
    {
        return;
    }
    // pop off operator token
    token_next();
    // Pop off the left node
    node_pop();
    node_left->flags |= NODE_FLAG_INSIDE_EXPRESSION;
    parse_expressionable_for_op(history_down(history, history->flags), op);
    struct node *node_right = node_pop(); // pop off node from line above
    node_right->flags |= NODE_FLAG_INSIDE_EXPRESSION;

    make_exp_node(node_left, node_right, op); // creating an expression node with left, op and right
    struct node *exp_node = node_pop();

    // Reorder the expression
    parser_reorder_expression(&exp_node);
    node_push(exp_node);
}

int parse_exp(struct history *history)
{ // parsing operating and merging with right operant
    parse_exp_normal(history);
    return 0;
}

int parse_expressionable_single(struct history *history)
{
    struct token *token = token_peek_next();
    if (!token)
    {
        return -1;
    }
    history->flags |= NODE_FLAG_INSIDE_EXPRESSION;
    int res = -1;
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        parse_single_token_to_node();
        res = 0; // succesfull operation
        break;

    case TOKEN_TYPE_OPERATOR:
        parse_exp(history);
        res = 0;
        break;
    }
    return res;
}
void parse_expressionable(struct history *history)
{
    while (parse_expressionable_single(history) == 0)
    {
    }
}

int parse_next()
{
    struct token *token = token_peek_next();
    if (!token)
    {
        return -1;
    }

    int res = 0;
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
    case TOKEN_TYPE_IDENTIFIER:
    case TOKEN_TYPE_STRING:
        parse_expressionable(history_begin(0));
        break;
    }

    return 0;
}

int parse(struct compile_process *process)
{
    current_process = process;
    parser_last_token = NULL;
    node_set_vector(process->node_vec, process->node_tree_vec);
    struct node *node = NULL;

    vector_set_peek_pointer(process->token_vec, 0);
    while (parse_next() == 0)
    {
        node = node_peek();
        vector_push(process->node_tree_vec, &node);
    }
    return PARSE_ALL_OK;
}