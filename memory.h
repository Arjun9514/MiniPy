// memory.h
#ifndef MEMORY_H
#define MEMORY_H

typedef enum {
    NONE,
    INT,
    FLOAT,
    STRING,
    BOOLEAN,
    ERROR,
} DataType;

typedef struct Literal{
    DataType datatype;
    int owns_str;
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

Literal copy_literal(const Literal src);
void set_variable(const char* name, Literal literal);
Literal get_variable(const char* name);
void get_variables();

#endif