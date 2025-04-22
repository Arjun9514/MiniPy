typedef enum{
    SYNTAX_ERROR,
    NAME_ERROR,
    TYPE_ERROR,
    ZERO_DIVISION_ERROR,
    VALUE_ERROR,
    LITERAL_ERROR,
    ASSIGNMENT_ERROR,
}ErrorType;

void raiseError(ErrorType type, char* error_msg);