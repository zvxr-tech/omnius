//
// Created by icarus on 04/07/15.
//

#ifndef SECMEM_REGEX_PARSE_H
#define SECMEM_REGEX_PARSE_H

#include <stddef.h>
#include "../fsm_descriptor.h"

void order_symbols(SYMBOL_T *, size_t);
int compile_regex(char *, SYMBOL_T *, SYMBOL_T **, STATE_T **);
#endif //SECMEM_REGEX_PARSE_H
