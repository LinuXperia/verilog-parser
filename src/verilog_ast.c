/*!
@file verilog_ast.c
@brief Contains definitions of functions which 
       operate on the Verilog Abstract Syntax Tree (AST)
*/

#include <assert.h>
#include <stdio.h>

#include "verilog_ast.h"

/*!
@defgroup ast-utility-mem-manage Memory Management
@{
    @brief Helps to manage memory allocated during AST construction.
@ingroup ast-utility
*/

/*!
@brief A simple linked list element holder for just about everything.
*/
typedef struct ast_alloc_t ast_alloc;
struct ast_alloc_t{
    ast_alloc * next;   //!< Next element in the linked list.
    void * data;        //!< Data held in the list.
};

//! Used to store the head of the linked list of allocated data.
ast_alloc * head = NULL;
//! Used to walk along the list when adding or freeing.
ast_alloc * walker = NULL;

/*!
@brief A simple wrapper around calloc.
@details This function is identical to calloc, but uses the head and
walker variables above to keep a linked list of all heap memory that the
AST construction allocates. This makes it very easy to clean up afterward
using the @ref ast_free_all function.
@param [in] num - Number of elements to allocate space for.
@param [in] size - The size of each element being allocated.
@returns A pointer to the start of the block of memory allocated.
*/
void * ast_calloc(size_t num, size_t size)
{
    if(head == NULL)
    {
        head =  calloc(1,sizeof(ast_alloc));
        walker = head;
    }
    else
    {
        walker -> next = calloc(1,sizeof(ast_alloc));
        walker = walker -> next;
    }

    walker -> data = calloc(num,size);
    return walker -> data;
}

/*!
@brief Frees all memory allocated using @ref ast_calloc.
@details Free's all data stored in the linked list pointed to by the
@ref head variable.
@post @ref walker and @ref head are NULL. All memory allocated by ast_calloc
has been freed.
*/
void ast_free_all()
{
    if(head == NULL)
        return; // No memory was allocated in the first place.

    while(head -> next != NULL)
    {
        walker = head;
        head = head -> next;
        free(walker -> data);
        free(walker);
    }

    walker = NULL;
    head = NULL;
}

/*!@}*/

/*!
@brief Creates a new empty ast_node and returns it.
@deprecated Do not use!
*/
ast_node * ast_node_new()
{
    ast_node * tr = ast_calloc(1, sizeof(ast_node));;

    tr-> type         = NONE;
    tr-> parent       = NULL;
    tr-> children     = NULL;
    tr-> child_count  = 0;
    
    return tr;
}

/*!
@brief Creates and returns a new node for the tree which contains a
       single simple identifier.
*/
ast_node * ast_new_identifier_node(ast_identifier identifier)
{
    ast_node * tr = ast_node_new();

    tr->type         = IDENTIFIER;
    tr->value.string = identifier;

    return tr;
}

/*!
@brief Creates and returns as a pointer a new attribute descriptor.
*/
ast_node_attributes * ast_new_attributes(ast_identifier name, 
                                         ast_expression * value)
{
    ast_node_attributes * tr = ast_calloc(1, sizeof(ast_node_attributes));
    tr->attr_name   = name;
    tr->attr_value  = value;
    return tr;
}


/*!
@brief Creates and returns a new attribute node with the specified value
       and name.
*/
ast_node * ast_new_attribute_node(ast_node_attributes * value)
{
    ast_node * tr = ast_node_new();

    tr -> type = ATTRIBUTE_LIST;
    tr -> value.attributes = value;

    return tr;
}

/*!
@brief Creates and returns a new attribute node with the specified value
       and name.
@param [inout] parent - Pointer to the node which represents the list of
                        attribute name,value pairs.
@param [in]    toadd  - The new attribute to add.
*/
void ast_append_attribute(ast_node_attributes * parent, 
                          ast_node_attributes * toadd)
{
    // Add the new attribute to the end of the list.

    ast_node_attributes * walker = parent -> next;
    while(walker -> next != NULL)
        walker = walker -> next;
    walker -> next = toadd;

}

/*!
@brief Creates and returns a new @ref ast_lvalue pointer, with the data type
       being a single identifier of either @ref NET_IDENTIFIER or
       @ref VAR_IDENTIFIER.
*/
ast_lvalue * ast_new_lvalue_id(ast_lvalue_type type, ast_identifier id)
{
    assert(type == NET_IDENTIFIER 
        || type == VAR_IDENTIFIER
        || type == GENVAR_IDENTIFIER);
    ast_lvalue * tr = ast_calloc(1, sizeof(ast_lvalue));
    tr -> type = type;
    tr -> data.identifier = id;
    return tr;
}

/*!
@brief Creates and returns a new @ref ast_lvalue pointer, with the data type
       being a concatenation holder of either @ref NET_CONCATENATION or
       @ref VAR_CONCATENATION.
*/
ast_lvalue * ast_new_lvalue_concat(ast_lvalue_type type, 
                                   ast_concatenation*concat)
{
    assert(type == NET_CONCATENATION 
        || type == VAR_CONCATENATION);
    ast_lvalue * tr = ast_calloc(1, sizeof(ast_lvalue));
    tr -> type = type;
    tr -> data.concatenation = concat;
    return tr;
}

/*!
@brief Creates a new ast primary which is part of a constant expression tree
       with the supplied type and value.
*/
ast_primary * ast_new_constant_primary(ast_primary_value_type type)
{
    ast_primary * tr = ast_calloc(1, sizeof(ast_primary));

    tr -> primary_type  = CONSTANT_PRIMARY;
    tr -> value_type    = type;

    return tr;
}


/*!
@brief Creates a new AST primary wrapper around a function call.
*/
ast_primary * ast_new_primary_function_call(ast_function_call * call)
{
    ast_primary * tr = ast_calloc(1, sizeof(ast_primary));
    assert(tr!=NULL);

    tr -> primary_type  = PRIMARY;
    tr -> value_type    = PRIMARY_FUNCTION_CALL;
    tr -> value.function_call = call;

    return tr;
}


/*!
@brief Creates a new ast primary which is part of an expression tree
       with the supplied type and value.
*/
ast_primary * ast_new_primary(ast_primary_value_type type)
{
    ast_primary * tr = ast_calloc(1, sizeof(ast_primary));

    tr -> primary_type  = PRIMARY;
    tr -> value_type    = type;

    return tr;
}

/*!
@brief Creates a new ast primary which is part of a constant expression tree
       with the supplied type and value.
*/
ast_primary * ast_new_module_path_primary(ast_primary_value_type type)
                                          
{
    ast_primary * tr = ast_calloc(1, sizeof(ast_primary));

    tr -> primary_type  = MODULE_PATH_PRIMARY;
    tr -> value_type    = type;

    return tr;
}

/*!
@brief Creates and returns a new expression primary.
@details This is simply an expression instance wrapped around a
primary instance for the purposes of mirroring the expression tree gramamr.
Whether or not the expression is constant is denoted by the type member
of the passed primary.
*/
ast_expression * ast_new_expression_primary(ast_primary * p)
{
    assert(sizeof(ast_expression) != 0);
    ast_expression * tr = ast_calloc(1, sizeof(ast_expression));
    assert(tr != NULL);
    
    tr -> attributes    = NULL;
    tr -> right         = NULL;
    tr -> left          = NULL;
    tr -> aux           = NULL;
    tr -> type          = PRIMARY_EXPRESSION;
    tr -> primary       = p;
    tr -> constant      = p -> primary_type == CONSTANT_PRIMARY ? AST_TRUE 
                                                                : AST_FALSE;

    return tr;
}


/*!
@brief Creates a new unary expression with the supplied operation.
*/
ast_expression * ast_new_unary_expression(ast_expression * operand,
                                          ast_operator     operation,
                                          ast_node_attributes * attr,
                                          ast_boolean       constant)
{
    ast_expression * tr = ast_calloc(1, sizeof(ast_expression));
    
    tr -> operation     = operation;
    tr -> attributes    = attr;
    tr -> right         = operand;
    tr -> left          = NULL;
    tr -> aux           = NULL;
    tr -> type          = UNARY_EXPRESSION;
    tr -> constant      = constant;

    return tr;
}

/*!
@brief Creates a new range expression with the supplied operands.
*/
ast_expression * ast_new_range_expression(ast_expression * left,
                                          ast_expression * right)
{
    ast_expression * tr = ast_calloc(1, sizeof(ast_expression));
    
    tr -> attributes    = NULL;
    tr -> right         = right;
    tr -> left          = left;
    tr -> aux           = NULL;
    tr -> type          = RANGE_EXPRESSION_UP_DOWN;

    return tr;
}
                                           
/*!
@brief Creates a new range index expression with the supplied operands.
*/
ast_expression * ast_new_index_expression(ast_expression * left)
{
    ast_expression * tr = ast_calloc(1, sizeof(ast_expression));
    
    tr -> attributes    = NULL;
    tr -> right         = NULL;
    tr -> left          = left;
    tr -> aux           = NULL;
    tr -> type          = RANGE_EXPRESSION_INDEX;

    return tr;
}

/*!
@brief Creates a new primary expression with the supplied operation
and operands.
@note Sets the type of the expression 
*/
ast_expression * ast_new_binary_expression(ast_expression * left,
                                           ast_expression * right,
                                           ast_operator     operation,
                                           ast_node_attributes * attr,
                                           ast_boolean      constant)
{
    ast_expression * tr = ast_calloc(1, sizeof(ast_expression));
    
    tr -> operation     = operation;
    tr -> attributes    = attr;
    tr -> right         = right;
    tr -> left          = left;
    tr -> aux           = NULL;
    tr -> type          = BINARY_EXPRESSION;
    tr -> constant      = constant;

    return tr;
}


/*!
@brief Creates a new string expression.
*/
ast_expression * ast_new_string_expression(ast_string string)
{
    ast_expression * tr = ast_calloc(1, sizeof(ast_expression));

    tr -> attributes    = NULL;
    tr -> right         = NULL;
    tr -> left          = NULL;
    tr -> aux           = NULL;
    tr -> type          = STRING_EXPRESSION;
    tr -> constant      = AST_TRUE;
    tr -> string        = string;
    
    return tr;
}


/*!
@brief Creates a new conditional expression node. 
@note The condition is stored in the aux member, if_true in left, and if_false
on the right.
*/
ast_expression * ast_new_conditional_expression(ast_expression * condition,
                                                ast_expression * if_true,
                                                ast_expression * if_false,
                                                ast_node_attributes * attr)
{
    ast_expression * tr = ast_calloc(1, sizeof(ast_expression));

    tr -> attributes    = attr;
    tr -> right         = if_false;
    tr -> left          = if_true;
    tr -> aux           = condition;
    tr -> type          = CONDITIONAL_EXPRESSION;
    
    return tr;
}

/*!
@brief Creates a new (min,typical,maximum) expression.
@details If the mintypmax expression only specifies a typical value,
then the min and max arguments should be NULL, and only typ set. 
*/
ast_expression * ast_new_mintypmax_expression(ast_expression * min,
                                              ast_expression * typ,
                                              ast_expression * max)
{
    ast_expression * tr = ast_calloc(1, sizeof(ast_expression));

    tr -> attributes    = NULL;
    tr -> right         = max;
    tr -> left          = min;
    tr -> aux           = typ;
    tr -> type          = MINTYPMAX_EXPRESSION;
    
    return tr;
}


/*!
@brief Creates and returns a new node representing a function call.
@param [in] id - The function identifier.
@param [in] attr - Attributes to be passed to the function call.
@param [in] system - Is this a system function call?
@param [in] constant - Is this a constant function call?
@param [in] arguments - list of elements of type ast_expression
representing the various parameters to the function. If the function has
no arguments, then it is an empty list, not NULL. If this is supplied as
NULL, then an empty list is added automatically by the function.
*/
ast_function_call * ast_new_function_call(ast_identifier  id,
                                          ast_boolean     constant,
                                          ast_boolean     system,
                                          ast_node_attributes * attr,
                                          ast_list      * arguments)
{
    ast_function_call * tr = ast_calloc(1, sizeof(ast_function_call));

    tr -> function  = id;
    tr -> constant  = constant;
    tr -> system    = system;
    tr -> arguments = arguments;
    tr -> attributes = attr;
    
    if(tr -> arguments == NULL)
    {
        tr -> arguments = ast_list_new();
    }

    return tr;
}


/*!
@brief Creates a new AST concatenation element with the supplied type and
initial starting value.
@param [in] repeat - Used for replications or multiple_concatenation.
@param [in] type - The kind of value being concatenated.
@param [in] first_value - The first value at the LHS of the concatenation.
@details Depending on the type supplied, the type of first_value
should be:
    - CONCATENATION_EXPRESSION          : ast_expression
    - CONCATENATION_CONSTANT_EXPRESSION : ast_expression
    - CONCATENATION_NET                 : TBD
    - CONCATENATION_VARIABLE            : TBD
    - CONCATENATION_MODULE_PATH         : TBD
@todo Better implement repetition of elements.
*/
ast_concatenation * ast_new_concatenation(ast_concatenation_type type,
                                          ast_expression * repeat,
                                          void * first_value)
{
    ast_concatenation * tr = ast_calloc(1,sizeof(ast_concatenation));

    tr -> repeat = repeat;
    tr -> type   = type;
    tr -> items  = ast_list_new();
    ast_list_append(tr -> items, first_value);

    return tr;
}

/*!
@brief Creates and returns a new empty concatenation of the specified type.
*/
ast_concatenation * ast_new_empty_concatenation(ast_concatenation_type type)
{
    ast_concatenation * tr = ast_calloc(1,sizeof(ast_concatenation));

    tr -> repeat = NULL;
    tr -> type   = type;
    tr -> items  = ast_list_new();

    return tr;
}


/*!
@brief Adds a new data element on to the end of a concatenation.
@details Appends to the front because this naturally follows the
behaviour of a left-recursive grammar.
@todo Better implement repetition of elements.
*/
void                ast_extend_concatenation(ast_concatenation * element,
                                             ast_expression * repeat,
                                             void * data)
{
    ast_list_preappend(element -> items, data);
}


/*!
@brief Creates and returns a new path declaration type. Expects that the data
be filled in manually;
*/
ast_path_declaration * ast_new_path_declaration(ast_path_declaration_type type)
{
    ast_path_declaration * tr = ast_calloc(1,sizeof(ast_path_declaration));

    tr -> type = type;
    tr -> state_expression = NULL;

    return tr;
}

/*!
@brief Creates and returns a pointer to a new simple parallel path declaration.
*/
ast_simple_parallel_path_declaration * ast_new_simple_parallel_path_declaration
    (
        ast_identifier      input_terminal,
        ast_operator        polarity,
        ast_identifier      output_terminal,
        ast_list        *   delay_value
    )
{
    ast_simple_parallel_path_declaration * tr = 
            ast_calloc(1, 
            sizeof(ast_simple_parallel_path_declaration));

    tr -> input_terminal = input_terminal;
    tr -> polarity       = polarity;
    tr -> output_terminal= output_terminal;
    tr -> delay_value    = delay_value;

    return tr;
}


/*!
@brief Creates and returns a pointer to a new simple full path declaration.
*/
ast_simple_full_path_declaration * ast_new_simple_full_path_declaration
    (
        ast_list        *   input_terminals,
        ast_operator        polarity,
        ast_list        *   output_terminals,
        ast_list        *   delay_value
    )
{
    ast_simple_full_path_declaration * tr = 
            ast_calloc(1, 
            sizeof(ast_simple_full_path_declaration));

    tr -> input_terminals= input_terminals;
    tr -> polarity       = polarity;
    tr -> output_terminals=output_terminals;
    tr -> delay_value    = delay_value;

    return tr;
}


/*! 
@brief Describes a single edge sensitive path declaration
*/
ast_edge_sensitive_parallel_path_declaration * 
  ast_new_edge_sensitive_parallel_path_declaration(
    ast_edge            edge,               //!< edge_identifier
    ast_identifier      input_terminal,     //!< specify_input_terminal_descriptor
    ast_operator        polarity,           //!< polarity_operator
    ast_identifier      output_terminal,    //!< specify_output_terminal_descriptor
    ast_expression  *   data_source,        //!< data_source_expression
    ast_list        *   delay_value         //!< path_delay_value
)
{
    ast_edge_sensitive_parallel_path_declaration * tr = 
        ast_calloc(1,sizeof(ast_edge_sensitive_parallel_path_declaration));

    tr -> edge            = edge           ;
    tr -> input_terminal  = input_terminal ;
    tr -> polarity        = polarity       ;
    tr -> output_terminal = output_terminal;
    tr -> data_source     = data_source    ;
    tr -> delay_value     = delay_value    ;

    return tr;
}


/*! 
@brief Describes a parallel edge sensitive path declaration
*/
ast_edge_sensitive_full_path_declaration * 
  ast_new_edge_sensitive_full_path_declaration(
    ast_edge            edge,               //!< edge_identifier
    ast_list        *   input_terminal,     //!< list_of_path_inputs
    ast_operator        polarity,           //!< polarity_operator
    ast_list        *   output_terminal,    //!< list_of_path_outputs
    ast_expression  *   data_source,        //!< data_source_expression
    ast_list        *   delay_value         //!< path_delay_value
)
{
    ast_edge_sensitive_full_path_declaration * tr = 
        ast_calloc(1,sizeof(ast_edge_sensitive_full_path_declaration));

    tr -> edge            = edge           ;
    tr -> input_terminal  = input_terminal ;
    tr -> polarity        = polarity       ;
    tr -> output_terminal = output_terminal;
    tr -> data_source     = data_source    ;
    tr -> delay_value     = delay_value    ;

    return tr;
}


/*!
@brief creates and returns a pointer to a new task-enable statement.
*/
ast_task_enable_statement * ast_new_task_enable_statement(
    ast_list        * expressions,
    ast_identifier    identifier, 
    ast_boolean       is_system   
)
{
    ast_task_enable_statement * tr = ast_calloc(1,
                                        sizeof(ast_task_enable_statement));

    tr -> expressions = expressions;
    tr -> identifier  = identifier;
    tr -> is_system   = is_system;

    return tr;
}


/*!
@brief Creates and returns a new forever loop statement.
@param inner_statement - Pointer to the inner body of statements which
make upt the loop body.
*/
ast_loop_statement * ast_new_forever_loop_statement(
    ast_statement * inner_statement
)
{
    ast_loop_statement * tr = ast_calloc(1,sizeof(ast_loop_statement));
    
    tr -> type              = LOOP_FOREVER;
    tr -> inner_statement   = inner_statement;
    tr -> initial           = NULL;
    tr -> condition         = NULL;
    tr -> modify            = NULL;

    return tr;
}

/*!
@brief Creates and returns a new for loop statement.
@param inner_statement - Pointer to the inner body of statements which
make upt the loop body.
@param initial_condition - Assignement which sets up the initial condition
of the iterator.
@param modify_assignment - How the iterator variable changes with each
loop iteration.
@param continue_condition - Expression which governs whether the loop should
continue or break.
*/
ast_loop_statement * ast_new_for_loop_statement(
    ast_statement  * inner_statement,
    ast_single_assignment * initial_condition,
    ast_single_assignment * modify_assignment,
    ast_expression * continue_condition
)
{
    ast_loop_statement * tr = ast_calloc(1,sizeof(ast_loop_statement));
    
    tr -> type              = LOOP_FOR;
    tr -> inner_statement   = inner_statement;
    tr -> initial           = initial_condition;
    tr -> condition         = continue_condition;
    tr -> modify            = modify_assignment;

    return tr;
}

/*!
@brief Creates and returns a while loop statement.
@param inner_statement - Pointer to the inner body of statements which
make upt the loop body.
@param continue_condition - Expression which governs whether the loop should
continue or break.
*/
ast_loop_statement * ast_new_while_loop_statement(
    ast_statement  * inner_statement,
    ast_expression * continue_condition
)
{
    ast_loop_statement * tr = ast_calloc(1,sizeof(ast_loop_statement));
    
    tr -> type              = LOOP_WHILE;
    tr -> inner_statement   = inner_statement;
    tr -> initial           = NULL;
    tr -> condition         = continue_condition;
    tr -> modify            = NULL;

    return tr;
}

/*!
@brief Creates and returns a repeat loop statement.
@param inner_statement - Pointer to the inner body of statements which
make upt the loop body.
@param continue_condition - Expression which governs whether the loop should
continue or break.
*/
ast_loop_statement * ast_new_repeat_loop_statement(
    ast_statement  * inner_statement,
    ast_expression * continue_condition
)
{
    ast_loop_statement * tr = ast_calloc(1,sizeof(ast_loop_statement));
    
    tr -> type              = LOOP_REPEAT;
    tr -> inner_statement   = inner_statement;
    tr -> initial           = NULL;
    tr -> condition         = continue_condition;
    tr -> modify            = NULL;

    return tr;
}


/*!
@brief Create and return a new item in a cast statement.
@param conditions - The conditions on which the item is executed.
@param body - Executes when any of the conditions are met.
*/
ast_case_item * ast_new_case_item(ast_list      * conditions,
                                  ast_statement * body)
{
    ast_case_item * tr = ast_calloc(1,sizeof(ast_case_item));
    
    tr -> conditions = conditions;
    tr -> body       = body;
    tr -> is_default = AST_FALSE;

    return tr;
}


/*!
@brief Creates and returns a new case statement.
@param expression - The expression evaluated to select a case.
@param cases - list of possible cases.
*/
ast_case_statement * ast_new_case_statement(ast_expression * expression,
                                            ast_list       * cases,
                                            ast_case_statement_type type)
{
    ast_case_statement * tr = ast_calloc(1,sizeof(ast_case_statement));

    tr -> expression = expression;
    tr -> cases      = cases;
    tr -> type       = type;
    tr -> is_function = AST_FALSE;

    int i;
    for(i = 0; i < tr -> cases -> items; i ++)
    {
        ast_case_item * the_case = (ast_case_item*)ast_list_get(tr->cases,i);

        if(the_case == NULL)
            break;

        if(the_case -> is_default == AST_TRUE)
        {
            tr -> default_item = ast_list_get(tr -> cases, i);
            break;
        }
    }

    return tr;
}


/*!
@brief Creates and returns a new conditional statement.
@param statement - what to run if the condition holds true.
@param condtion  - the condition on which statement is run.
*/
ast_conditional_statement * ast_new_conditional_statement(
    ast_statement * statement,
    ast_expression * condition
)
{
    ast_conditional_statement * tr = 
        ast_calloc(1,sizeof(ast_conditional_statement));

    tr -> statement = statement;
    tr -> condition = condition;

    return tr;
}


/*!
@brief Creates a new if-then-else-then statement.
@param if_condition - the conditional statement.
@param else_condition - What to do if no conditional statements are executed.
This can be NULL.
@details This node also supports "if then elseif then else then" statements,
and uses the ast_extend_if_else function to append a new 
ast_conditional_statement to the end of a list of if-else conditions.
Priority of exectuion is given to items added first.
*/
ast_if_else * ast_new_if_else(
    ast_conditional_statement * if_condition,
    ast_statement             * else_condition
)
{
    ast_if_else * tr = ast_calloc(1, sizeof(ast_if_else));

    tr -> else_condition         = else_condition;
    tr -> conditional_statements = ast_list_new();

    ast_list_append(tr -> conditional_statements, if_condition);

    return tr;
}


/*!
@brief Adds an additional conditional (ha..) to an existing if-else
statement.
@param conditional_statements - the existing if-else tree.
@param new_statement - The new statement to add at the end of the existing
if-then conditions, but before any else_condtion.
*/
void  ast_extend_if_else(
    ast_if_else     * conditional_statements,
    ast_list        * new_statements
)
{
    if(new_statements != NULL)
    {
        ast_list_concat(conditional_statements -> conditional_statements,
                        new_statements);
    }
}


/*!
@brief Creates and returns a new wait statement.
*/
ast_wait_statement * ast_new_wait_statement(
    ast_expression * wait_for,
    ast_statement  * statement
)
{
    ast_wait_statement * tr = ast_calloc(1, sizeof(ast_wait_statement));
    
    tr -> expression = wait_for;
    tr -> statement  = statement;

    return tr;
}

/*!
@brief Creates a new event expression node
@param trigger_edge - the edge on which the trigger is activated.
@param expression - the expression to monitor the waveforms of.
*/
ast_event_expression * ast_new_event_expression(
    ast_edge trigger_edge,
    ast_expression * expression
)
{
    ast_event_expression * tr = ast_calloc(1,sizeof(ast_event_expression));

    assert(trigger_edge != EDGE_NONE);

    if(trigger_edge == EDGE_POS)
    {
        tr -> type = EVENT_POSEDGE;
        tr -> expression = expression;
    }
    else if (trigger_edge == EDGE_NEG)
    {
        tr -> type = EVENT_NEGEDGE;
        tr -> expression = expression;
    }
    else if (trigger_edge == EDGE_ANY)
    {
        tr -> type = EVENT_EXPRESSION;
        tr -> expression = expression;
    }

    return tr;
}

/*!
@brief Creates a new event expression node, which is itself a sequence of
sub-expressions.
*/
ast_event_expression * ast_new_event_expression_sequence(
    ast_event_expression * left,
    ast_event_expression * right
)
{
    ast_event_expression * tr = ast_calloc(1,sizeof(ast_event_expression));

    tr -> type = EVENT_SEQUENCE;
    tr -> sequence = ast_list_new();

    ast_list_append(tr -> sequence, right);
    ast_list_append(tr -> sequence, left );

    return tr;
}

/*!
@brief Creates and returns a new event control specifier.
*/
ast_event_control * ast_new_event_control(
    ast_event_control_type type,
    ast_event_expression * expression
)
{
    ast_event_control * tr = ast_calloc(1,sizeof(ast_event_control));

    if(type == EVENT_CTRL_ANY)
        assert(expression == NULL);

    tr -> type = type;
    tr -> expression = expression;

    return tr;
}

/*!
@brief creates and returns a new delay control statement.
*/
ast_delay_ctrl * ast_new_delay_ctrl_value(ast_delay_value * value)
{
    ast_delay_ctrl * tr = ast_calloc(1,sizeof(ast_event_control));

    tr -> type = DELAY_CTRL_VALUE;
    tr -> value = value;

    return tr;
}

/*!
@brief creates and returns a new delay control statement.
*/
ast_delay_ctrl * ast_new_delay_ctrl_mintypmax(
    ast_expression * mintypmax
)
{
    ast_delay_ctrl * tr = ast_calloc(1,sizeof(ast_event_control));

    tr -> type = DELAY_CTRL_MINTYPMAX;
    tr -> mintypmax = mintypmax;

    return tr;
}

/*!
@brief Creates and returns a new timing control statement node.
*/
ast_timing_control_statement * ast_new_timing_control_statement_delay(
    ast_timing_control_statement_type   type,
    ast_statement                     * statement,
    ast_delay_ctrl                    * delay_ctrl
)
{
    ast_timing_control_statement * tr = 
        ast_calloc(1,sizeof(ast_timing_control_statement));

    assert(type == TIMING_CTRL_DELAY_CONTROL);

    tr -> type = type;
    tr -> delay = delay_ctrl;
    tr -> statement = statement;
    tr -> repeat = NULL;

    return tr;
}


/*!
@brief Creates and returns a new timing control statement node.
*/
ast_timing_control_statement * ast_new_timing_control_statement_event(
    ast_timing_control_statement_type   type,
    ast_expression                    * repeat,
    ast_statement                     * statement,
    ast_event_control                 * event_ctrl
)
{
    ast_timing_control_statement * tr = 
        ast_calloc(1,sizeof(ast_timing_control_statement));

    assert(type == TIMING_CTRL_EVENT_CONTROL ||
           type == TIMING_CTRL_EVENT_CONTROL_REPEAT);

    tr -> type = type;
    tr -> event_ctrl = event_ctrl;
    tr -> statement = statement;
    tr -> repeat = repeat;

    return tr;
}

/*!
@brief Creates and returns a new assignment.
*/
ast_single_assignment * ast_new_single_assignment(
    ast_lvalue * lval,
    ast_expression * expression
)
{
    ast_single_assignment * tr = ast_calloc(1,sizeof(ast_single_assignment));

    tr -> lval = lval;
    tr -> expression = expression;

    return tr;
}

/*!
@brief Creates a new hybrid assignment of the specified type.
*/
ast_assignment * ast_new_hybrid_assignment(
    ast_hybrid_assignment_type type,
    ast_single_assignment * assignment
)
{
    ast_assignment * tr = ast_calloc(1,sizeof(ast_assignment));

    tr -> type = ASSIGNMENT_HYBRID;
    tr -> hybrid = ast_calloc(1,sizeof(ast_hybrid_assignment));
    tr -> hybrid -> type = type;
    tr -> hybrid -> assignment = assignment;

    return tr;
}


/*!
@brief Creates a new hybrid assignment of the specified type.
*/
ast_assignment * ast_new_hybrid_lval_assignment(
    ast_hybrid_assignment_type type,
    ast_lvalue *lval 
)
{
    ast_assignment * tr = ast_calloc(1,sizeof(ast_assignment));

    tr -> type = ASSIGNMENT_HYBRID;
    tr -> hybrid = ast_calloc(1,sizeof(ast_hybrid_assignment));
    tr -> hybrid -> type = type;
    tr -> hybrid -> lval = lval;

    return tr;
}

/*!
@brief Creates and returns a new blocking procedural assignment object.
*/
ast_assignment * ast_new_blocking_assignment(
    ast_lvalue * lval,
    ast_expression  * expression,
    ast_timing_control_statement* delay_or_event
)
{
    ast_assignment * tr = ast_calloc(1,sizeof(ast_assignment));
    tr -> type          = ASSIGNMENT_BLOCKING;

    tr -> procedural    =  ast_calloc(1,sizeof(ast_procedural_assignment));
    tr -> procedural    -> lval = lval;
    tr -> procedural    -> expression = expression;
    tr -> procedural    -> delay_or_event = delay_or_event;

    return tr;
}

/*!
@brief Creates and returns a new nonblocking procedural assignment object.
*/
ast_assignment * ast_new_nonblocking_assignment(
    ast_lvalue * lval,
    ast_expression  * expression,
    ast_timing_control_statement * delay_or_event
)
{
    ast_assignment * tr = ast_calloc(1,sizeof(ast_assignment));
    tr -> type          = ASSIGNMENT_NONBLOCKING;

    tr -> procedural    =  ast_calloc(1,sizeof(ast_procedural_assignment));
    tr -> procedural    -> lval = lval;
    tr -> procedural    -> expression = expression;
    tr -> procedural    -> delay_or_event = delay_or_event;

    return tr;
}


/*!
@brief Creates and returns a new continuous assignment object.
*/
ast_assignment * ast_new_continuous_assignment(
    ast_list * assignments,
    ast_drive_strength * strength,
    ast_delay3 * delay
)
{
    ast_continuous_assignment * trc = ast_calloc(1,
                sizeof(ast_continuous_assignment));
    trc -> assignments = assignments;
    trc -> drive_strength = strength;
    trc -> delay = delay;

    ast_assignment * tr = ast_calloc(1, sizeof(ast_assignment));

    tr -> type = ASSIGNMENT_CONTINUOUS;
    tr -> continuous = trc;

    return tr;
}


/*!
@brief Creates and returns a new statement block of the specified type
*/
ast_statement_block * ast_new_statement_block(
    ast_block_type   type,
    ast_identifier   block_identifier,
    ast_list       * declarations,
    ast_list       * statements
)
{
    ast_statement_block * tr = ast_calloc(1,sizeof(ast_statement_block));

    tr -> type = type;
    tr -> block_identifier = block_identifier;
    tr -> declarations = declarations;
    tr -> statements = statements;

    return tr;
}

//! Creates and returns a pointer to a new disable statement.
ast_disable_statement * ast_new_disable_statement(ast_identifier   id)
{
    ast_disable_statement * tr = ast_calloc(1, sizeof(ast_disable_statement));
    tr -> id = id;
    return tr;
}


/*!
@brief Creates a new AST statement and returns it.
@note Requires the data field of the union to be filled out manually.
*/
ast_statement * ast_new_statement(
    ast_node_attributes * attr,
    ast_boolean         is_function_statement,
    void             *  data,
    ast_statement_type  type
)
{
    ast_statement * tr = ast_calloc(1,sizeof(ast_statement));

    tr -> type = type;
    tr -> is_function_statement = is_function_statement;
    tr -> is_generate_statement = AST_FALSE;
    tr -> data = data;
    tr -> attributes = attr;

    return tr;
}


/*!
@brief Creates a new UDP port AST node
@details
@returns A pointer to the new port
*/
ast_udp_port * ast_new_udp_port(
    ast_port_direction    direction,
    ast_identifier        identifier,
    ast_node_attributes * attributes,
    ast_boolean           reg,
    ast_expression      * default_value
)
{
    ast_udp_port * tr = ast_calloc(1,sizeof(ast_udp_port));
    tr -> direction     = direction;
    assert(direction != PORT_INPUT);
    tr -> identifier    = identifier;
    tr -> attributes    = attributes;
    tr -> reg           = reg;
    tr -> default_value = default_value;
    return tr;
}


/*!
@brief Creates a new UDP port AST node
@details
@returns A pointer to the new port
*/
ast_udp_port * ast_new_udp_input_port(
    ast_list            * identifiers,
    ast_node_attributes * attributes
)
{
    ast_udp_port * tr = ast_calloc(1,sizeof(ast_udp_port));
    tr -> direction     = PORT_INPUT;
    tr -> identifiers   = identifiers;
    tr -> attributes    = attributes;
    tr -> reg           = AST_FALSE;
    tr -> default_value = NULL;
    return tr;
}

/*!
@brief Creates a new UDP declaration node
@details
@returns A pointer to the new node.
*/
ast_udp_declaration * ast_new_udp_declaration(
    ast_node_attributes * attributes,
    ast_identifier        identifier,
    ast_list            * ports,
    ast_udp_body        * body
)
{
    ast_udp_declaration * tr = ast_calloc(1,sizeof(ast_udp_declaration));

    tr -> attributes    = attributes;
    tr -> identifier    = identifier;
    tr -> ports         = ports;
    tr -> body_entries  = body -> entries;
    tr -> initial       = body -> initial;
    tr -> body_type     = body -> body_type;

    free(body);

    return tr;
}

/*!
@brief Creates a new instance of a UDP.
@details
@returns A pointer to the new instance.
*/
ast_udp_instance * ast_new_udp_instance(
    ast_identifier        identifier,
    ast_range           * range,
    ast_lvalue          * output,
    ast_list            * inputs
){
    ast_udp_instance * tr = ast_calloc(1,sizeof(ast_udp_instance));

    tr -> identifier    = identifier;
    tr -> range         = range;
    tr -> output        = output;
    tr -> inputs        = inputs;

    return tr;
}

/*!
@brief Creates a new list of UDP instances with shared properties.
@details 
@returns A pointer to the new list.
*/
ast_udp_instantiation * ast_new_udp_instantiation(
    ast_list            * instances,
    ast_identifier        identifier,
    ast_drive_strength  * drive_strength,
    ast_delay2          * delay
){
    ast_udp_instantiation * tr = ast_calloc(1,sizeof(ast_udp_instantiation));

    tr -> instances     = instances;
    tr -> identifier    = identifier;
    tr -> drive_strength= drive_strength;
    tr -> delay         = delay;

    return tr;
}


//! Creates a new initial statement node.
ast_udp_initial_statement * ast_new_udp_initial_statement(
    ast_identifier   output_port,
    ast_number     * initial_value
){
    ast_udp_initial_statement *tr=ast_calloc(1,sizeof(ast_udp_initial_statement));
    
    tr -> output_port   = output_port;
    tr -> initial_value = initial_value;

    return tr;
}


//! Creates and returns a new sequential UDP body representation.
ast_udp_body * ast_new_udp_sequential_body(
    ast_udp_initial_statement * initial_statement,
    ast_list                  * sequential_entries
){
    ast_udp_body * tr = ast_calloc(1,sizeof(ast_udp_body));
    
    tr -> body_type = UDP_BODY_SEQUENTIAL;
    tr -> initial   = initial_statement;
    tr -> entries   = sequential_entries;

    return tr;
}

//! Creates and returns a new combinatorial UDP body representation.
ast_udp_body * ast_new_udp_combinatoral_body(
    ast_list                  * combinatorial_entries
){
    ast_udp_body * tr = ast_calloc(1,sizeof(ast_udp_body));

    tr -> body_type = UDP_BODY_COMBINATORIAL;
    tr -> entries = combinatorial_entries;

    return tr;
}

//! Creates a new combinatorial entry for a UDP node.
ast_udp_combinatorial_entry * ast_new_udp_combinatoral_entry(
    ast_list * input_levels,
    ast_udp_next_state output_symbol
){
    ast_udp_combinatorial_entry * tr = 
        ast_calloc(1,sizeof(ast_udp_combinatorial_entry));

    tr -> input_levels = input_levels;
    tr -> output_symbol = output_symbol;

    return tr;
}

//! Creates a new sequntial body entry for a UDP node.
ast_udp_sequential_entry * ast_new_udp_sequential_entry(
    ast_udp_seqential_entry_prefix prefix_type,
    ast_list    *                  levels_or_edges,
    ast_level_symbol               current_state,
    ast_udp_next_state             output
){
    ast_udp_sequential_entry * tr = ast_calloc(1,
                                        sizeof(ast_udp_sequential_entry));

    tr -> entry_prefix = prefix_type;

    if(prefix_type == PREFIX_EDGES)
        tr -> edges = levels_or_edges;
    else
        tr -> levels = levels_or_edges;

    tr -> current_state = current_state;
    tr -> output        = output;

    return tr;
}


/*!
@brief Creates and returns a new item which exists inside a generate statement.
@details Wraps around ast_new_statement and sets appropriate internal flags
to represent this as a statment in a generate block.
@note the void* type of the construct parameter allows for a single
constructor function rather than one per member of the union inside the
ast_generate_item structure.
*/
ast_statement * ast_new_generate_item(
    ast_statement_type type,
    void    *          construct
){
    ast_statement * tr = ast_new_statement(NULL, AST_FALSE, construct,type);

    tr -> is_generate_statement = AST_TRUE;

    return tr;
}


//! Creates and returns a new block of generate items.
ast_generate_block * ast_new_generate_block(
    ast_identifier   identifier,
    ast_list       * generate_items
){
    ast_generate_block * tr = ast_calloc(1,sizeof(ast_generate_block));

    tr -> generate_items = generate_items;
    tr -> identifier     = identifier;

    return tr;
}


/*!
@brief Creates and returns a new set of module instances with shared
parameters.
*/
ast_module_instantiation * ast_new_module_instantiation(
    ast_identifier          module_identifer,
    ast_list              * module_parameters,
    ast_list              * module_instances
){
    ast_module_instantiation * tr = 
        ast_calloc(1,sizeof(ast_module_instantiation));
    
    tr -> module_identifer  = module_identifer;
    tr -> module_parameters = module_parameters;
    tr -> module_instances  = module_instances;

    return tr;
}

/*!
@brief Creates and returns a new instance of a module with a given identifer
and set of port connections.
*/
ast_module_instance * ast_new_module_instance(
    ast_identifier          instance_identifier,
    ast_list              * port_connections
){
    ast_module_instance * tr = ast_calloc(1,sizeof(ast_module_instance));

    tr -> instance_identifier = instance_identifier;
    tr -> port_connections    = port_connections;

    return tr;
}

/*!
@brief Creates and returns a new port connection representation.
@param port_name - The port being assigned to.
@param expression - The thing inside the module the port connects to.
*/
ast_port_connection * ast_new_named_port_connection(
    ast_identifier   port_name,
    ast_expression * expression
){
    ast_port_connection * tr = ast_calloc(1,sizeof(ast_port_connection));

    tr -> port_name = port_name;
    tr -> expression = expression;

    return tr;
}


//! Instances a new switch type with a delay3.
ast_switch_gate * ast_new_switch_gate_d3(
    ast_switchtype type,
    ast_delay3     delay
){
    assert(type != SWITCH_TRAN && type != SWITCH_RTRAN);
    ast_switch_gate * tr = ast_calloc(1,sizeof(ast_switch_gate));

    tr -> type   = type;
    tr -> delay3 = delay;

    return tr;
}

//! Instances a new switch type with a delay2.
ast_switch_gate * ast_new_switch_gate_d2(
    ast_switchtype type,
    ast_delay2     delay
){
    assert(type == SWITCH_TRAN || type == SWITCH_RTRAN);
    ast_switch_gate * tr = ast_calloc(1,sizeof(ast_switch_gate));

    tr -> type   = type;
    tr -> delay2 = delay;

    return tr;
}

//! Creates and returns a new structure describing primitive net strength.
ast_primitive_pull_strength * ast_new_primitive_pull_strength(
    ast_pull_direction       direction,
    ast_primitive_strength   strength_1,
    ast_primitive_strength   strength_0
){
    ast_primitive_pull_strength * tr = 
        ast_calloc(1,sizeof(ast_primitive_pull_strength));

    tr -> direction  = direction;
    tr -> strength_1 = strength_1;
    tr -> strength_0 = strength_0;

    return tr;
}

/*! @brief Describes a single pull gate instance.*/
ast_pull_gate_instance * ast_new_pull_gate_instance(
    ast_identifier      name,
    ast_lvalue        * output_terminal
){
    ast_pull_gate_instance * tr = ast_calloc(1,sizeof(ast_pull_gate_instance));

    tr -> name = name;
    tr -> output_terminal = output_terminal;

    return tr;
}

/*! @brief A single pass transistor instance.*/
ast_pass_switch_instance * ast_new_pass_switch_instance(
    ast_identifier      name,
    ast_lvalue        * terminal_1,
    ast_lvalue        * terminal_2
){
    ast_pass_switch_instance * tr = 
                        ast_calloc(1,sizeof(ast_pass_switch_instance));

    tr -> name = name;
    tr -> terminal_1 = terminal_1;
    tr -> terminal_2 = terminal_2;

    return tr;
}

/*! @brief An N-input gate instance. e.g. 3-to-1 NAND.*/
ast_n_input_gate_instance * ast_new_n_input_gate_instance(
    ast_identifier      name,
    ast_list          * input_terminals,
    ast_lvalue        * output_terminal
){
    ast_n_input_gate_instance * tr =
                        ast_calloc(1,sizeof(ast_n_input_gate_instance));

    tr -> name = name;
    tr -> input_terminals = input_terminals;
    tr -> output_terminal = output_terminal;

    return tr;
}

/*! @brief A single Enable gate instance.*/
ast_enable_gate_instance * ast_new_enable_gate_instance(
    ast_identifier      name,
    ast_lvalue        * output_terminal,
    ast_expression    * enable_terminal,
    ast_expression    * input_terminal
){
    ast_enable_gate_instance * tr = 
        ast_calloc(1,sizeof(ast_enable_gate_instance));

    tr -> name = name;
    tr -> output_terminal = output_terminal;
    tr -> enable_terminal = enable_terminal;
    tr -> input_terminal  = input_terminal;

    return tr;
}

/*! @brief A single MOS switch (transistor) instance.*/
ast_mos_switch_instance * ast_new_mos_switch_instance(
    ast_identifier      name,
    ast_lvalue        * output_terminal,
    ast_expression    * enable_terminal,
    ast_expression    * input_terminal
){
    ast_mos_switch_instance * tr = 
        ast_calloc(1,sizeof(ast_mos_switch_instance));

    tr -> name = name;
    tr -> output_terminal = output_terminal;
    tr -> enable_terminal = enable_terminal;
    tr -> input_terminal  = input_terminal;

    return tr;
}

/*! @brief A single CMOS switch (transistor) instance.*/
ast_cmos_switch_instance * ast_new_cmos_switch_instance(
    ast_identifier      name,
    ast_lvalue        * output_terminal,
    ast_expression    * ncontrol_terminal,
    ast_expression    * pcontrol_terminal,
    ast_expression    * input_terminal
){
    ast_cmos_switch_instance * tr = 
        ast_calloc(1,sizeof(ast_cmos_switch_instance));

    tr -> name = name;
    tr -> output_terminal = output_terminal;
    tr -> ncontrol_terminal = ncontrol_terminal;
    tr -> pcontrol_terminal = pcontrol_terminal;
    tr -> input_terminal = input_terminal;

    return tr;
}

/*!
@brief Creates and returns a new pass enable switch instance.
*/
ast_pass_enable_switch * ast_new_pass_enable_switch(
    ast_identifier      name,
    ast_lvalue        * terminal_1,
    ast_lvalue        * terminal_2,
    ast_expression    * enable
){
    ast_pass_enable_switch * tr = 
        ast_calloc(1,sizeof(ast_pass_enable_switch));

    tr -> name = name;
    tr -> terminal_1 = terminal_1;
    tr -> terminal_2 = terminal_2;
    tr -> enable = enable;

    return tr;
}


/*!
@brief Creates and returns a collection of pass enable switches.
*/
ast_pass_enable_switches * ast_new_pass_enable_switches(
    ast_pass_enable_switchtype    type,
    ast_delay2                    delay,
    ast_list                    * switches 
){
    ast_pass_enable_switches * tr = 
                              ast_calloc(1,sizeof(ast_pass_enable_switches));

    tr -> type = type;
    tr -> delay = delay;
    tr -> switches = switches;

    return tr;
}


/*! 
@brief Creates collection of n-input gates with the same type and properties.
*/
ast_n_input_gate_instances * ast_new_n_input_gate_instances(
    ast_gatetype_n_input    type,
    ast_delay3            * delay,
    ast_drive_strength    * drive_strength,
    ast_list              * instances
){
    ast_n_input_gate_instances * tr = 
                            ast_calloc(1,sizeof(ast_n_input_gate_instances));

    tr -> type = type;
    tr -> delay = delay;
    tr -> drive_strength = drive_strength;
    tr -> instances = instances;

    return tr;
}

//! Creates collection of enable gates with the same type and properties.
ast_enable_gate_instances * ast_new_enable_gate_instances(
    ast_gatetype_n_input    type,
    ast_delay3            * delay,
    ast_drive_strength    * drive_strength,
    ast_list              * instances
){
    ast_enable_gate_instances * tr = 
                            ast_calloc(1,sizeof(ast_enable_gate_instances));

    tr -> type = type;
    tr -> delay = delay;
    tr -> drive_strength = drive_strength;
    tr -> instances = instances;

    return tr;
}


/*!
@brief Creates and returns a new n_output gate instance.
@see ast_n_output_gate_instances
*/
ast_n_output_gate_instance * ast_new_n_output_gate_instance(
    ast_identifier                name,
    ast_list                    * outputs,
    ast_expression              * input
){
    ast_n_output_gate_instance * tr =
                            ast_calloc(1,sizeof(ast_n_output_gate_instance));

    tr -> name = name;
    tr -> outputs = outputs;
    tr -> input = input;

    return tr;
}

/*!
@brief Creates and returns a set of n_output gates with the same properties.
*/
ast_n_output_gate_instances * ast_new_n_output_gate_instances(
    ast_n_output_gatetype         type,
    ast_delay2                  * delay,
    ast_drive_strength          * drive_strength,
    ast_list                    * instances
){
    ast_n_output_gate_instances * tr = 
                          ast_calloc(1,sizeof(ast_n_output_gate_instances));

    tr -> type = type;
    tr -> delay = delay;
    tr -> drive_strength = drive_strength;
    tr -> instances = instances;

    return tr;
}

/*!
@brief creat and return a new collection of AST switches.
*/
ast_switches * ast_new_switches(ast_switch_gate * type, ast_list * switches)
{
    ast_switches * tr = calloc(1,sizeof(ast_switches));

    tr -> type = type;
    tr -> switches = switches;

    return tr;
}

//! Create and return a new pull strength indicator for 1 and 0.
ast_pull_strength * ast_new_pull_stregth(
    ast_primitive_strength strength_1,
    ast_primitive_strength strength_2
){
    ast_pull_strength * tr = calloc(1,sizeof(ast_pull_strength));

    tr -> strength_1 = strength_1;
    tr -> strength_2 = strength_2;

    return tr;
}

/*!
@brief Creates and returns a new gate instantiation descriptor.
@details Expects the data fields to be filled out manually after the structure
is returned.
*/
ast_gate_instantiation * ast_new_gate_instantiation(ast_gate_type type)
{
    ast_gate_instantiation * tr = calloc(1,sizeof(ast_gate_instantiation));
    tr -> type = type;
    return tr;
}

/*!
@brief creates and returns a new set of parameter declarations of the same type
@param [in] assignments - The list of individual assignments.
@param [in] signed_values - are the bit vectors signed?
@param [in] range - Bit range
@param [in] type - type of the parameters.
*/
ast_parameter_declarations * ast_new_parameter_declarations(
    ast_list        * assignments,
    ast_boolean       signed_values,
    ast_boolean       local,
    ast_range       * range,
    ast_parameter_type  type 
){
    ast_parameter_declarations * tr = 
            ast_calloc(1,sizeof(ast_parameter_declarations));


    tr -> assignments = assignments;
    tr -> signed_values = signed_values;
    tr -> range = range;
    tr -> type = type;
    tr -> local = local;

    if(type != PARAM_GENERIC){
        tr -> range = NULL;
        tr -> signed_values = AST_FALSE;
    }

    return tr;
}

/*!
@brief Creates and returns a new port declaration representation.
*/
ast_port_declaration * ast_new_port_declaration(
    ast_port_direction  direction,      //!< [in] Input / output / inout etc.
    ast_net_type        net_type,       //!< [in] Wire/reg etc
    ast_boolean         net_signed,     //!< [in] Signed value?
    ast_boolean         is_reg,         //!< [in] Is explicitly a "reg"
    ast_boolean         is_variable,    //!< [in] Variable or net?
    ast_range         * range,          //!< [in] Bus width.
    ast_list          * port_names      //!< [in] The names of the ports.
){
    ast_port_declaration * tr = ast_calloc(1,sizeof(ast_port_declaration));
    
    tr -> direction   =  direction  ;
    tr -> net_type    =  net_type   ;
    tr -> net_signed  =  net_signed ;
    tr -> is_reg      =  is_reg     ;
    tr -> is_variable =  is_variable;
    tr -> range       =  range      ;
    tr -> port_names  = port_names  ;
    
    return tr;
}

/*!
@brief Creates and returns a node to represent the declaration of a new
module item construct.
@details Because of the complex nature of the grammar for these declarations,
(bourne from the number of optional modifiers) no single constructor function
is provided. Rather, one can create a new type declaration of a
known type, but must otherwise fill out the data members as they go along.
All pointer members are initialised to NULL, and all boolean members will
initially be false.
*/
ast_type_declaration * ast_new_type_declaration(ast_declaration_type type)
{
    ast_type_declaration * tr = calloc(1,sizeof(ast_type_declaration));

    tr -> type = type;
    tr -> identifiers = NULL;
    tr -> delay = NULL;
    tr -> drive_strength = NULL;
    tr -> charge_strength = NULL;
    tr -> range = NULL;
    tr -> vectored = AST_FALSE;
    tr -> scalared = AST_FALSE;
    tr -> is_signed = AST_FALSE;
    tr -> net_type  = NET_TYPE_NONE;

    return tr;
}   
