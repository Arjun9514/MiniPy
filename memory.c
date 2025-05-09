// memory.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "lexer.h"
#include "ast.h"
#include "memory.h"
#include "error_handling.h"
#include "interpreter.h"
// #include "debug_alloc.h"

Variable* symbol_table = NULL;

#include <stdlib.h>
#include <string.h>

void copy_literal(Literal dest, const Literal src) {
    dest.datatype = src.datatype;
    dest.owns_str = 0;
    switch (src.datatype) {
        case 'i': // integer
            dest.numeric = src.numeric;
            break;
        case 'f': // float
            dest.floating_point = src.floating_point;
            break;
        case 'b': // boolean
            dest.boolean = src.boolean;
            break;
        case 's': // string
            if (src.string) {
                dest.string = strdup(src.string);
                dest.owns_str = 1;
            } else {
                dest.string = NULL;
                dest.owns_str = 0;
            }
            break;
    }
}


void set_variable(const char* name, Literal lit) {
    Variable* var = symbol_table;
    Literal literal;
    if(lit.datatype == 's'){
        literal.datatype = 's';
        literal.string = strdup(lit.string);
        literal.owns_str = 0;
    }else{
        literal = lit;
    }
    while (var != NULL) {
        if (strcmp(var->name, name) == 0) {
            if(var->literal.datatype == 's') free(var->literal.string);
            var->literal = literal;
            return;
        }
        var = var->next;
    }

    // Not found, add new
    Variable* new_var = malloc(sizeof(Variable));
    new_var->name = strdup(name);
    new_var->literal = literal;
    new_var->next = symbol_table;
    symbol_table = new_var;
}

Literal get_variable(const char* name) {
    Variable* var = symbol_table;
    while (var != NULL) {
        if (strcmp(var->name, name) == 0) {
            return var->literal;
        }
        var = var->next;
    }
    char msg[255] = "Undefined variable -> ";
    strcat(msg,name);
    raiseError(NAME_ERROR, msg);
    Literal lit;
    lit.datatype = '0';
    return lit;
}

void get_variables(){
    Variable* var = symbol_table;
    while (var != NULL) {
        printf("%s: ", var->name);
        print_literal(var->literal);
        var = var->next;
    }
}