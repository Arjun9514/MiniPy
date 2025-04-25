//error_handling.c
#include <stdio.h>
#include "colors.h"
#include "error_handling.h"

int error = 0;

const char* error_name(ErrorType type) {
    switch (type) {
        case SYNTAX_ERROR: return "Syntax Error";
        case NAME_ERROR: return "Name Error";
        case TYPE_ERROR: return "Type Error";
        case VALUE_ERROR: return "Value Error";
        case ZERO_DIVISION_ERROR: return "Zero Division Error";
        case LITERAL_ERROR: return "Literal Error";
        case ASSIGNMENT_ERROR: return "Assignment Error";
        case MEMORY_ERROR: return "Memory Error";
        case INDENTATION_ERROR: return "Indentation Error";
        default: return "Unknown Error";
    }
}

void raiseError(ErrorType type, char* error_msg){
    printf(RED "%s" RESET ": " RED "%s\n" RESET, error_name(type), error_msg);
    error = 1;
}