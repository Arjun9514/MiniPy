// memory.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "memory.h"

Variable* symbol_table = NULL;

void set_variable(const char* name, Literal lit) {
    Variable* var = symbol_table;
    while (var != NULL) {
        if (strcmp(var->name, name) == 0) {
            var->literal = lit;
            return;
        }
        var = var->next;
    }

    // Not found, add new
    Variable* new_var = malloc(sizeof(Variable));
    new_var->name = strdup(name);
    new_var->literal = lit;
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
    Literal lit;
    lit.datatype = '0';
    return lit;
}