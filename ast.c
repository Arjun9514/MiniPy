//ast.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "error_handling.h"

extern Token* tokens;
extern int token_count;

extern char *keywords[];
extern const int num_keywords;

int current = 0;

int get_precedence(char op) {
    switch (op) {
        case '*': case '/': return 3;
        case '+': case '-': return 2;
        case '>': case '<': case 'g': case 'e': case 'l': case 'n': return 1;
        default: return 0;
    }
}

char* get_key(){
    return tokens[current-1].text;
}

const char* AST_node_name(ASTNodeType type) {
    switch (type) {
        case AST_NONE: return "NONE";
        case AST_NUMERIC: return "NUMERIC";
        case AST_FLOATING_POINT: return "FLOATING_POINT";
        case AST_STRING: return "STRING";
        case AST_BOOLEAN: return "BOOLEAN";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_OPERATOR: return "OPERATOR";
        case AST_ASSIGNMENT: return "ASSIGN";
        case AST_KEYWORD: return "KEYWORD";
        default: return "UNKNOWN";
    }
}

Token peek(){
    return tokens[current];
}

Token advance(){
    return tokens[current++];
}

int match(TokenType type){
    if(tokens[current].type == type){
        current++;
        return 1;
    }
    return 0;
}

void print_ast_debug(ASTNode* node, int indent) {
    for (int i = 0; i < indent; i++) printf("|-");
    printf("%s", AST_node_name(node->type));
    if (node->type == AST_OPERATOR) {
        printf(" '%c'\n", node->operate.op);
        print_ast_debug(node->operate.left, indent + 1);
        print_ast_debug(node->operate.right, indent + 1);
    } else if (node->type == AST_NUMERIC) {
        printf(" %c %d\n", node->literal.datatype, node->literal.numeric);
    } else if (node->type == AST_FLOATING_POINT) {
        printf(" %c %f\n", node->literal.datatype, node->literal.floating_point);
    } else if (node->type == AST_BOOLEAN) {
        printf(" %c %d\n", node->literal.datatype, node->literal.boolean);
    } else if (node->type == AST_STRING) {
        printf(" %c \"%s\"\n", node->literal.datatype, node->literal.string);
    } else if (node->type == AST_IDENTIFIER) {
        printf(" %s\n", node->name);
    } else {
        printf("\n");
    }
}

ASTNode* parse_none() {
    Token tok = advance();
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NONE;
    node->literal.datatype = 'n';
    return node;
}

ASTNode* parse_numeric() {
    Token tok = advance();
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_NUMERIC;
    char *end;
    int val = (int)strtol(tok.text, &end, 10);
    node->literal.datatype = 'd';
    node->literal.numeric = val;
    return node;
}

ASTNode* parse_floating_point() {
    Token tok = advance();
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_FLOATING_POINT;
    char *end;
    float val = strtof(tok.text, &end);
    node->literal.datatype = 'f';
    node->literal.floating_point = val;
    return node;
}

ASTNode* parse_string() {
    Token tok = advance();
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_STRING;
    node->literal.datatype = 's';
    node->literal.string = tok.text;
    return node;
}

ASTNode* parse_boolean() {
    Token tok = advance();
    ASTNode* node = malloc(sizeof(ASTNode));
    char* end;
    node->type = AST_BOOLEAN;
    node->literal.datatype = 'b';
    if (strcmp(tok.text,"True") == 0){
        node->literal.boolean = 1;
    }else{
        node->literal.boolean = 0;
    }
    return node;
}

ASTNode* parse_identifier() {
    Token tok = advance();
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->name = strdup(tok.text);
    return node;
}

ASTNode* parse_paren() {
    advance();//consume '(' token
    int start = current;
    ASTNode* node = malloc(sizeof(ASTNode));
    while(peek().type != TOKEN_RPAREN){
        if (peek().type == TOKEN_EOF){
            raiseError(SYNTAX_ERROR, "Missing brackets");
            return NULL;
        }
        node = parse_expression();
    }
    advance();//consume ')' token
    return node;
}

ASTNode* parse_primary() {
    Token tok = peek();
    switch (tok.type){
        case TOKEN_NONE: return parse_none();
        case TOKEN_NUMERIC: return parse_numeric();
        case TOKEN_FLOATING_POINT: return parse_floating_point();
        case TOKEN_STRING: return parse_string();
        case TOKEN_BOOLEAN: return parse_boolean();
        case TOKEN_IDENTIFIER: return parse_identifier();
        case TOKEN_LPAREN: return parse_paren();
        default: return NULL;
    }
}

ASTNode* parse_expression_prec(int min_prec);

ASTNode* parse_expression() {
    return parse_expression_prec(0);
}

ASTNode* parse_expression_prec(int min_prec) {
    ASTNode* left = parse_primary();
    if (!left){
        raiseError(SYNTAX_ERROR, "Missing left operand");
        return NULL;
    }
    while (peek().type == TOKEN_OPERATOR) {
        char op = peek().text[0];
        int prec = get_precedence(op);

        if (prec < min_prec) break;

        advance(); // consume operator

        // precedence climbing: right side must be at least prec + 1
        ASTNode* right = parse_expression_prec(prec + 1);
        if (!right) {
            raiseError(SYNTAX_ERROR, "Expected expression after operator");
            return NULL;
        }

        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = AST_OPERATOR;
        node->operate.op = op;
        node->operate.left = left;
        node->operate.right = right;
        left = node;
    }

    return left;
}

ASTNode* parse_assignment() {
    char *name = strdup(advance().text);
    advance(); // '='

    ASTNode* value = parse_expression();

    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_ASSIGNMENT;
    node->assign.name = name;
    node->assign.value = value;
    return node;
}

ASTNode* parse_keyword() {
    match(TOKEN_KEYWORD); // skip 'keyword'
    char* key = get_key();
    match(TOKEN_LPAREN);
    ASTNode* val = parse_expression();
    match(TOKEN_RPAREN);
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_KEYWORD;
    node->keyword.key = strdup(key);
    node->keyword.value = val;
    return node;
}

ASTNode* parse_statement() {
    for (int i = 0; i < num_keywords; i++) {
        if (peek().type == TOKEN_KEYWORD && strcmp(peek().text, keywords[i]) == 0) {
            return parse_keyword();
        }
    }
    if (peek().type == TOKEN_IDENTIFIER && tokens[current + 1].type == TOKEN_ASSIGN) {
        return parse_assignment();
    }
    return parse_expression();
}
