// memory.h
#ifndef MEMORY_H
#define MEMORY_H

typedef struct Literal{
    char datatype;
    union {
        int numeric; // for NUMERIC
        float floating_point; // for FLOATING_POINT
        char* string;// for STRING
        int boolean;// for BOOLEAN
    };
} Literal;

typedef struct Variable {
    char* name;
    Literal literal;
    struct Variable* next;
} Variable;

extern Variable* symbol_table;

void set_variable(const char* name, Literal literal);
Literal get_variable(const char* name);

#endif