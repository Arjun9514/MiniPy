//ast.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "interpreter.h"
#include "error_handling.h"
// #include "debug_alloc.h"

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
    printf("%s ", AST_node_name(node->type));
    switch (node->type){
        case AST_OPERATOR:
            printf(" '%c'\n", node->operate.op);
            print_ast_debug(node->operate.left, indent + 1);
            print_ast_debug(node->operate.right, indent + 1);
            break;
        case AST_NUMERIC:
        case AST_FLOATING_POINT:
        case AST_BOOLEAN:
        case AST_STRING:
            print_literal(node->literal);break;
        case AST_IDENTIFIER:
            printf(" %s\n", node->name);break;
        case AST_ASSIGNMENT:
            printf("\n");
            print_ast_debug(node->assign.value, indent+1);
            break;
    default:
        break;
    }
}

void ast_free(ASTNode *node);

void ast_free(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_IDENTIFIER:
            free(node->name);
            break;

        case AST_NUMERIC:
        case AST_FLOATING_POINT:
        case AST_BOOLEAN:
            // No heap allocations inside these literal types.
            break;

        case AST_STRING:
            // The literal.string was malloc'd by process_str/strdup.
            free(node->literal.string);
            break;

        case AST_OPERATOR:
            ast_free(node->operate.left);
            ast_free(node->operate.right);
            break;

        case AST_ASSIGNMENT:
            free(node->assign.name);
            free(node->assign.value);
            break;

        case AST_KEYWORD:
            free(node->keyword.key);
            ast_free(node->keyword.value);
            break;
        // If you have other node types, handle them here.
        default:
            break;
    }

    // Finally, free the node itself.
    free(node);
}

ASTNode* new_node(){
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        raiseError(MEMORY_ERROR, "Out of memory");
        return NULL;
    }
    return node;
}

ASTNode* parse_none() {
    Token tok = advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_NONE;
    node->literal.datatype = 'n';
    return node;
}

ASTNode* parse_numeric() {
    Token tok = advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_NUMERIC;
    char *end;
    int val = (int)strtol(tok.text, &end, 10);
    node->literal.datatype = 'd';
    node->literal.numeric = val;
    return node;
}

ASTNode* parse_floating_point() {
    Token tok = advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_FLOATING_POINT;
    char *end;
    float val = strtof(tok.text, &end);
    node->literal.datatype = 'f';
    node->literal.floating_point = val;
    return node;
}

ASTNode* parse_string() {
    Token tok = advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_STRING;
    node->literal.datatype = 's';
    node->literal.string = strdup(tok.text);
    return node;
}

ASTNode* parse_boolean() {
    Token tok = advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
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
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_IDENTIFIER;
    node->name = strdup(tok.text);
    return node;
}

ASTNode* parse_paren() {
    advance();//consume '(' token
    ASTNode* node = parse_expression();
    if (!node) return NULL;
    if (peek().type != TOKEN_RPAREN) {
        raiseError(SYNTAX_ERROR, "Unmatched '('");
        return NULL;
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
        if(peek().type == TOKEN_OPERATOR){
            if(peek().text[0] == '+' || peek().text[0] == '-'){
                left = new_node();
                left->type = AST_NUMERIC;
                left->literal.datatype = 'd';
                left->literal.numeric = 0;
            }else{
                goto syntax_error;
            }
        }else{
            syntax_error:
                raiseError(SYNTAX_ERROR, "Missing left operand");
                return NULL;
        }
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

        ASTNode* node = new_node();
    if (!node) return NULL;
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

    ASTNode* node = new_node();
    if (!node) return NULL;
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
    ASTNode* node = new_node();
    if (!node) return NULL;
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
