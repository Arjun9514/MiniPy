// memory.c
#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

Variable* symbol_table = NULL;

void set_variable(const char* name,const char datatype, ...) {
    Variable* var = symbol_table;
    va_list args;
    va_start(args, datatype);
    while (var != NULL) {
        if (strcmp(var->name, name) == 0) {
            var->datatype = datatype;
            switch (datatype) {
                case 'd': var->numeric = va_arg(args, int); break;
                case 'f': var->floating_point = va_arg(args, double); break; // float is promoted to double
                case 's': var->string = strdup(va_arg(args, char*)); break;
                case 'b': var->boolean = va_arg(args, int); break;
                default: break;
            }
            va_end(args);
            return;
        }
        var = var->next;
    }

    // Not found, add new
    Variable* new_var = malloc(sizeof(Variable));
    new_var->name = strdup(name);
    new_var->datatype = datatype;
    switch (datatype) {
        case 'd': new_var->numeric = va_arg(args, int); break;
        case 'f': new_var->floating_point = va_arg(args, double); break;
        case 's': new_var->string = strdup(va_arg(args, char*)); break;
        case 'b': new_var->boolean = va_arg(args, int); break;
        default: break;
    }
    new_var->next = symbol_table;
    symbol_table = new_var;
    va_end(args);
}

Value get_variable(const char* name) {
    Variable* var = symbol_table;

    while (var != NULL) {
        if (strcmp(var->name, name) == 0) {
            Value val;
            val.datatype = var->datatype;
            switch (var->datatype)
            {
            case 'd': val.numeric = var->numeric; break;
            case 'f': val.floating_point = var->floating_point; break;
            case 's': val.string = strdup(var->string); break;
            case 'b': val.boolean = var->boolean; break;
            }
            return val;
        }
        var = var->next;
    }
    Value val;
    val.datatype = '0';
    return val;
}