/*
 *
 * Based on code from:
 * Eli Bendersky (eliben@gmail.com)
 * Retrieved from:
 * https://github.com/eliben/code-for-blog/blob/master/2009/regex_fsm/regex_parse.cpp
 *
 *
 * Modified 2015 - Mike Clark
 */
#include <assert.h>
#include <iostream>
#include <cstdlib>
#include "nfa.h"
#include "subset_construct.h"


//
// The BNF for our simple regexes is:
//
// expr     ::= concat '|' expr
//          |   concat
//
// concat   ::= rep . concat
//          |   rep
//
// rep      ::= atom '*'
//          |   atom '?'
//          |   atom
//
// atom     ::= chr
//          |   '(' expr ')'
//
// char     ::= alphanumeric character
//

using namespace std;


// A singleton scanner class, encapsulates the input stream
//
class scanner
{
public:
    void init(string data_)
    {
        data = preprocess(data_);
        next = 0;
        is_error = FALSE;
    }

    char peek(void)
    {
        return (char) ((next < data.size()) ? data[next] : 0);
    }

    char pop(void)
    {
        char cur = peek();

        if (next < data.size())
            ++next;

        return cur;
    }

    unsigned get_pos(void)
    {
        return next;
    }

    char is_error;

    friend scanner& my_scanner(void);

private:
    scanner()
    {}

    string preprocess(string in);

    string data;
    unsigned next;
};


// Generates concatenation chars ('.') where
// appropriate
//
string scanner::preprocess(string in)
{
    string out = "";

    string::const_iterator c = in.begin(), up = c + 1;

    // in this loop c is the current char of in, up is the next one
    //
    for (; up != in.end(); ++c, ++up)
    {
        out.push_back(*c);

        if ((isalnum(*c) || *c == ')' || *c == '*' || *c == '?') &&
            (*up != ')' && *up != '|' && *up != '*' && *up != '?'))
            out.push_back('.');
    }

    // don't forget the last char ...
    //
    if (c != in.end())
        out.push_back(*c);

    return out;
}


scanner& my_scanner(void)
{
    // scanner my_scan;
    static scanner my_scan;
    return my_scan;
}



typedef enum {CHR, STAR, QUESTION, ALTER, CONCAT} node_type;


// Parse node
//
struct parse_node
{
    parse_node(node_type type_, char data_, parse_node* left_, parse_node* right_)
            : type(type_), data(data_), left(left_), right(right_)
    {}

    node_type type;
    char data;
    parse_node* left;
    parse_node* right;
};


parse_node* expr();

NFA tree_to_nfa(parse_node* tree)
{
    assert(tree);

    switch (tree->type)
    {
        case CHR:
            return build_nfa_basic(tree->data);
        case ALTER:
            return build_nfa_alter(tree_to_nfa(tree->left), tree_to_nfa(tree->right));
        case CONCAT:
            return build_nfa_concat(tree_to_nfa(tree->left), tree_to_nfa(tree->right));
        case STAR:
            return build_nfa_star(tree_to_nfa(tree->left));
        case QUESTION:
            return build_nfa_alter(tree_to_nfa(tree->left), build_nfa_basic(EPS));
        default:
            assert(0);
    }
}


// RD parser
//

// char   ::= alphanumeric character
//
parse_node* chr()
{
    char data = my_scanner().peek();

    if (isalnum(data) || data == 0)
    {
        return new parse_node(CHR, my_scanner().pop(), 0, 0);
    }

    cerr 	<< "Parse error: expected alphanumeric, got "
    <<  my_scanner().peek() << " at #" << my_scanner().get_pos() << endl;
    //exit(1);
    my_scanner().is_error = TRUE;
    return NULL;
}


// atom ::= chr
//      |   '(' expr ')'
//
parse_node* atom()
{
    parse_node* atom_node;

    if (my_scanner().peek() == '(')
    {
        my_scanner().pop();
        atom_node = expr();

        if (my_scanner().pop() != ')')
        {
            cerr << "Parse error: expected ')'" << endl;
            //exit(1);
            my_scanner().is_error = TRUE;
            return NULL;
        }
    }
    else
    {
        atom_node = chr();
    }

    return atom_node;
}


// rep  ::= atom '*'
//      |   atom '?'
//      |   atom
//
parse_node* rep()
{
    parse_node* atom_node = atom();

    if (my_scanner().peek() == '*')
    {
        my_scanner().pop();

        parse_node* rep_node = new parse_node(STAR, 0, atom_node, 0);
        return rep_node;
    }
    else if (my_scanner().peek() == '?')
    {
        my_scanner().pop();

        parse_node* rep_node = new parse_node(QUESTION, 0, atom_node, 0);
        return rep_node;
    }
    else
    {
        return atom_node;
    }
}


// concat   ::= rep . concat
//          |   rep
//
parse_node* concat()
{
    parse_node* left = rep();

    if (my_scanner().peek() == '.')
    {
        my_scanner().pop();
        parse_node* right = concat();

        parse_node* concat_node = new parse_node(CONCAT, 0, left, right);
        return concat_node;
    }
    else
    {
        return left;
    }
}


// expr ::= concat '|' expr
//      |   concat
//
parse_node* expr(void)
{
    parse_node* left = concat();

    if (my_scanner().peek() == '|')
    {
        my_scanner().pop();
        parse_node* right = expr();

        parse_node* expr_node = new parse_node(ALTER, 0, left, right);
        return expr_node;
    }
    else
    {
        return left;
    }
}



/*
 * Bubblesort
 *
 */
#define swap(_x,_y) do{(_x) ^= (_y); (_y) ^= (_x); (_x) ^= (_y);}while (0)
void
order_symbols(SYMBOL_T *sym, size_t len)
{
    size_t i, top;
    do {
        top = 0;
        for (i = 1; i < len; i++) {
            if (sym[i-1] > sym[i]) {
                swap(sym[i - 1], sym[i]);
                top = i;
            }
        }
        len = top;
    } while(len != 0);
    return;
}


/* This routine takes in a null-terminated regex.
 * It compiles a FSM jmp table representation, a corresponding alpha_map table (which maps omnius input symbols to the
 * internal symbol enumeration), the number of symbols active (involved in at least one non-sink dst transition) in the
 * FSM.
 *
 *
 * the symbol_count pointer is used to pass that info back to the caller.
 * The alpha_map is used to pass in the location for this routine to place an allocated and initialized alpha_map
 *  for the FSM mp table generated from the regex.
 * The jmp_tbl is used to pass in the location for this routine to place an allocated and initialized jmp_tbl compiled
 *  from the regex.
 *
 *  The caller is responsible for deallocating the jmp table and alphamap, however if this routine is failing, it is
 *  responsible for freeing those.
 */
extern "C" int compile_regex(char *regex, SYMBOL_T *symbol_count, SYMBOL_T **alpha_map_p, STATE_T **jmp_tbl_p)
{
    my_scanner().init(regex);
    parse_node* n = expr();
    if (my_scanner().is_error || my_scanner().peek() != 0)
        return EXIT_FAILURE;

    DFA dfa = subset_construct(tree_to_nfa(n));
    size_t count = 0;
    if ((*alpha_map_p = (SYMBOL_T *) calloc(MAX_SYMBOL, sizeof(SYMBOL_T)))) {
        SYMBOL_T *alpha_map = *alpha_map_p;
        memset(alpha_map, FSM_NULL_STATE, MAX_SYMBOL * sizeof(SYMBOL_T)); /* default map to null state */
        count = dfa.construct_jmptbl(alpha_map, jmp_tbl_p);
        *symbol_count = count;
    }
    return count > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

