// ast.h
#ifndef AST_H
#define AST_H

#include "memory.h"

typedef struct ASTNode ASTNode;

typedef enum {
    AST_BLOCK,
    AST_NONE,
    AST_NUMERIC,
    AST_FLOATING_POINT,
    AST_STRING,
    AST_BOOLEAN,
    AST_IDENTIFIER,
    AST_OPERATOR,
    AST_ASSIGNMENT,
    AST_PASS,
    AST_PRINT,
    AST_IF,
    AST_ELIF,
    AST_ELSE,
    AST_WHILE,
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;

    union {
        char* name; // for AST_IDENTIFIER
        
        struct Literal literal; // for AST_NUMERIC,AST_FLOATING_POINT,AST_STRING,AST_BOOLEAN

        struct { // for AST_BLOCK
            ASTNode** statements;
            int count;
        } block;

        struct { // for AST_OPERATOR
            struct ASTNode* left;
            char op;
            struct ASTNode* right;
        } operate;

        struct { // for AST_ASSIGNMENT
            char* name;
            struct ASTNode* value;
        } assign;

        struct { // for AST_PRINT
            struct ASTNode* value;
        } print;

        struct { // for AST_IF,AST_ELIF,AST_ELSE
            struct ASTNode* condition;
            struct ASTNode* code;
            struct ASTNode* next;
        } if_else;

        struct { // for AST_WHILE
            struct ASTNode* condition;
            struct ASTNode* code;
        } _while;
    };

} ASTNode;

const char* AST_node_name(ASTNodeType type);

void print_ast_debug(ASTNode* node, int indent, int is_last);
void ast_free(ASTNode *node);

Token peek();
Token advance();

ASTNode* new_node();
ASTNode* parse_expression();
ASTNode* parse_statement(ASTNode* parent_node);

#endif