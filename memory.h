// memory.h
typedef struct Variable {
    char* name;
    char datatype;
    union {
        int numeric; // for NUMERIC
        float floating_point; // for FLOATING_POINT
        char* string;// for STRING
    };
    struct Variable* next;
} Variable;

typedef struct {
    char datatype;
    union {
        int numeric;
        float floating_point;
        char* string;
    };
} Value;

extern Variable* symbol_table;

void set_variable(const char* name,const char datatype,...);
Value get_variable(const char* name);