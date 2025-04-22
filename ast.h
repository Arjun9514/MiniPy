// ast.h
typedef enum {
    AST_NUMERIC,
    AST_FLOATING_POINT,
    AST_STRING,
    AST_BOOLEAN,
    AST_IDENTIFIER,
    AST_OPERATOR,
    AST_ASSIGNMENT,
    AST_KEYWORD
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;

    union {
        int numeric; // for AST_NUMERIC
        float floating_point; // for AST_FLOATING_POINT
        char* string;// for AST_STRING
        int boolean;// for AST_BOOLEAN

        char* name; // for AST_IDENTIFIER
        
        struct { // for AST_OPERATOR
            struct ASTNode* left;
            char op;
            struct ASTNode* right;
        } operate;

        struct { // for AST_ASSIGNMENT
            char* name;
            struct ASTNode* value;
        } assign;

        struct { // for AST_KEYWORD
            char* key;
            struct ASTNode* value;
        } keyword;
    };

} ASTNode;

const char* AST_node_name(ASTNodeType type);

void print_ast_debug(ASTNode* node, int indent);

Token peek();
Token advance();

ASTNode* parse_expression();
ASTNode* parse_statement();