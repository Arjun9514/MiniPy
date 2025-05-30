//ast.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "interpreter.h"
#include "colors.h"
#include "error_handling.h"
// #include "debug_alloc.h"

extern Token* tokens;
extern int token_count;

extern char *keywords[];
extern char **lines;
extern const int num_keywords;

extern int error;
extern int debug;
extern int script_;
extern int line_count;
extern int current_line;

int current = 0;

int get_precedence(char op) {
    switch (op) {
        case '*': case '/': return 6;
        case '+': case '-': return 5;
        case '>': case '<': case 'g': case 'e': case 'l': case 'n': return 4;
        case '!': return 3;
        case '&': return 2; 
        case '|': return 1;
        default: return 0;
    }
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
        case AST_PASS: return "PASS";
        case AST_BREAK: return "BREAK";
        case AST_PRINT: return "PRINT";
        case AST_IF: return "IF";
        case AST_ELIF: return "ELIF";
        case AST_ELSE: return "ELSE";
        case AST_WHILE : return "WHILE";
        case AST_BLOCK: return "BLOCK";
        default: return "UNKNOWN";
    }
}

Token peek(){
    return tokens[current];
}

Token advance(){
    return tokens[current++];
}

void print_ast_debug(ASTNode* node, int indent, int is_last) {
    if (!node) return;

    // Print branch lines
    for (int i = 0; i < indent - 1; i++) {
        printf("%c   ", 179); // '│'
    }
    if (indent > 0) {
        printf("%c%c ", is_last ? 192 : 195, 196); // '└─' or '├─'
    }
    
    printf("%s", AST_node_name(node->type));

    switch (node->type) {
        case AST_OPERATOR:
            printf(" '%c'\n", node->operate.op);
            print_ast_debug(node->operate.left, indent + 1, 0);
            print_ast_debug(node->operate.right, indent + 1, 1);
            break;

        case AST_NUMERIC:
        case AST_FLOATING_POINT:
        case AST_BOOLEAN:
        case AST_STRING:
            printf(" ");
            print_literal(node->literal);
            break;

        case AST_IDENTIFIER:
            printf(" %s\n", node->name);
            break;

        case AST_PASS:
            printf("\n");
            break;
        
        case AST_BREAK:
            printf("\n");
            break;

        case AST_PRINT:
            printf("\n");
            print_ast_debug(node->print.value, indent + 1, 1);
            break;

        case AST_ASSIGNMENT:
            printf("\n");
            print_ast_debug(node->assign.value, indent + 1, 1);
            break;

        case AST_IF:
        case AST_ELIF:
            printf("\n");
            for (int i = 0; i < indent; i++) printf("%c   ", 179);
            printf("%c%c [IF_CONDITION]\n", 195, 196);
            print_ast_debug(node->construct.condition, indent + 2, 1);

            for (int i = 0; i < indent; i++) printf("%c   ", 179);
            printf("%c%c [IF_BODY]\n", 195, 196);
            print_ast_debug(node->construct.code, indent + 2, 1);

            if (node->construct.next) print_ast_debug(node->construct.next, indent, 1);
            break;

        case AST_ELSE:
            printf("\n");
            print_ast_debug(node->construct.code, indent + 1, 0);
            break;

        case AST_WHILE:
            printf("\n");
            for (int i = 0; i < indent; i++) printf("%c   ", 179);
            printf("%c%c [WHILE_CONDITION]\n", 195, 196);
            print_ast_debug(node->construct.condition, indent + 2, 1);

            for (int i = 0; i < indent; i++) printf("%c   ", 179);
            printf("%c%c [WHILE_BODY]\n", 195, 196);
            print_ast_debug(node->construct.code, indent + 2, 1);
            break;
        
        case AST_BLOCK:
            printf("\n");
            for (int i = 0; i < node->block.count; i++) {
                print_ast_debug(node->block.statements[i], indent + 1, (i == node->block.count - 1));
            }
            break;

        default:
            printf("\n");
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
            ast_free(node->assign.value);
            break;

        case AST_PRINT:
            ast_free(node->print.value);
            break;

        case AST_IF:
        case AST_ELIF:
            ast_free(node->construct.condition);
            ast_free(node->construct.code);
            ast_free(node->construct.next);
            break;

        case AST_ELSE:
            ast_free(node->construct.code);
            break;

        case AST_WHILE:
            ast_free(node->construct.condition);
            ast_free(node->construct.code);
            break;

        case AST_BLOCK:
            for (int i = 0; i < node->block.count; i++) {
                ast_free(node->block.statements[i]);
            }
            free(node->block.statements);
            break;

        default:
            break;
    }

    // Finally, free the node itself.
    // print_allocations();
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

ASTNode* parse_pass() {
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_PASS;
    return node;
}

ASTNode* parse_break() {
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_BREAK;
    return node;
}

ASTNode* parse_none() {
    advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_NONE;
    node->literal.datatype = NONE;
    return node;
}

ASTNode* parse_numeric() {
    Token tok = advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_NUMERIC;
    char *end;
    int val = (int)strtol(tok.text, &end, 10);
    node->literal.datatype = INT;
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
    node->literal.datatype = FLOAT;
    node->literal.floating_point = val;
    return node;
}

ASTNode* parse_string() {
    Token tok = advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_STRING;
    node->literal.datatype = STRING;
    node->literal.string = strdup(tok.text);
    return node;
}

ASTNode* parse_boolean() {
    Token tok = advance();
    ASTNode* node = new_node();
    if (!node) return NULL;
    char* end;
    node->type = AST_BOOLEAN;
    node->literal.datatype = BOOLEAN;
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
        case TOKEN_SEMICOLON: return NULL;
        case TOKEN_COLON: return NULL;
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
                left->literal.datatype = INT;
                left->literal.numeric = 0;
            }else if(peek().text[0] == '!'){
                left = new_node();
                left->type = AST_BOOLEAN;
                left->literal.datatype = BOOLEAN;
                left->literal.boolean = 1;
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
            ast_free(left);
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
    char op = advance().text[0]; // '='
    
    ASTNode* value;
    if (op == '='){
        value = parse_expression();
    } else {
        ASTNode* sub_node = new_node();
        if (!sub_node) return NULL;
        sub_node->type = AST_OPERATOR;
        sub_node->operate.op = op;
        ASTNode* id = new_node();
        if (!id) return NULL;
        id->type = AST_IDENTIFIER;
        id->name = strdup(name);
        sub_node->operate.left = id;
        sub_node->operate.right = parse_expression();
        value = sub_node;
    }

    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_ASSIGNMENT;
    node->assign.name = name;
    node->assign.value = value;
    return node;
}

ASTNode* new_block() {
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_BLOCK;
    node->block.count = 0;
    node->block.statements = NULL;
    return node;
}

ASTNode* update_block(ASTNode* block_node, ASTNode* stmt){
    block_node->block.count++;
    ASTNode** tmp = realloc(
        block_node->block.statements,
        sizeof(ASTNode*) * block_node->block.count
    );
    if (!tmp) {
        raiseError(MEMORY_ERROR,"Out of memory");
        ast_free(stmt);
        ast_free(block_node);
        return NULL;
    }
    block_node->block.statements = tmp;
    block_node->block.statements[block_node->block.count - 1] = stmt;
    return block_node;
}

int block(ASTNode* parent_node, int parent_indent){
    ASTNode* block_node = new_block();
    if (!block_node) goto mistake;

    char input[255];

    while (1) {
        reset_tokens();

        if (!script_) {
            printf(MAG "... " RESET);
            
            if (!fgets(input, sizeof input, stdin)) goto end;
            input[strcspn(input, "\r\n")] = '\0';

            // Check empty line -> end of block
            if (strlen(input) == 0) {
                if (block_node->block.count == 0){
                    raiseError(SYNTAX_ERROR,"Block of statements missing");
                    goto mistake;
                }
                allocate_tokens();
                add_token(TOKEN_EOF, "", 0);
                goto end;
            }
            
            allocate_tokens();
            tokenize(input);
            
            if (debug){ printf("Tokens:\n"); print_tokens_debug();}
            if (error) goto mistake;

            while(peek().type != TOKEN_EOF){        
                int indent = 0;
                while(peek().type == TOKEN_INDENT){
                    advance();
                    indent++;
                }
                // printf("%d %d\n",global_indent,indent);

                if (indent < parent_indent){
                    if (block_node->block.count == 0){
                        raiseError(SYNTAX_ERROR,"Block of statements missing");
                        goto mistake;
                    }
                    goto end;
                }
                ASTNode* stmt = parse_statement(parent_node);
                if (!stmt) goto mistake;

                if (stmt->type == AST_ELIF || stmt->type == AST_ELSE) {
                    if (parent_node && (parent_node->type == AST_IF || parent_node->type == AST_ELIF) && indent == parent_indent) {
                        parent_node->construct.next = stmt; // connect ELSE to IF
                        goto end;
                    } else if (stmt->type == AST_ELSE && parent_node && (parent_node->type == AST_WHILE) && indent == parent_indent) {
                        parent_node->construct.next = stmt; // connect ELSE to IF
                        goto end;
                    }else {
                        ast_free(stmt);
                        goto mistake;
                    }
                } else if (stmt->type == AST_IF || stmt->type == AST_WHILE) {
                    if (!update_block(block_node,stmt)) goto mistake;
                } else {
                    // Normal statement
                    if (indent <= parent_indent){
                        if (block_node->block.count == 0){
                            raiseError(SYNTAX_ERROR,"Improper Indentation");
                            goto mistake;
                        }
                        current = 0;
                        goto end;
                    }
                    if (!update_block(block_node,stmt)) goto mistake;
                }
            }
        } else {
            if (current_line >= line_count){
                allocate_tokens();
                add_token(TOKEN_EOF, "", 0);
                goto end;
            }
            strcpy(input,lines[current_line]);
            current_line++;

            allocate_tokens();
            tokenize(input);
            
            if (debug){ printf("Tokens:\n"); print_tokens_debug();}
            if (error) goto mistake;

            while(peek().type != TOKEN_EOF){        
                int indent = 0;
                while(peek().type == TOKEN_INDENT){
                    advance();
                    indent++;
                }
                // printf("%d %d\n",global_indent,indent);

                if (indent < parent_indent){
                    if (block_node->block.count == 0){
                        raiseError(SYNTAX_ERROR,"Block of statements missing");
                        goto mistake;
                    }
                    goto end;
                }
                int lc = current_line-1;
                ASTNode* stmt = parse_statement(parent_node);
                if (!stmt) goto mistake;

                if (stmt->type == AST_ELIF || stmt->type == AST_ELSE) {
                    if (parent_node && (parent_node->type == AST_IF || parent_node->type == AST_ELIF) && indent == parent_indent) {
                        parent_node->construct.next = stmt; // connect ELSE to IF
                        goto end;
                    } else if (stmt->type == AST_ELSE && parent_node && (parent_node->type == AST_WHILE) && indent == parent_indent) {
                        parent_node->construct.next = stmt; // connect ELSE to IF
                        goto end;
                    } else {
                        ast_free(stmt);
                        goto mistake;
                    }
                } else if (stmt->type == AST_IF || stmt->type == AST_WHILE) {
                    if (indent == parent_indent) {
                        ast_free(stmt);
                        current_line = lc;
                        goto end;
                    } else {
                        if (!update_block(block_node,stmt)) goto mistake;
                    }
                } else {
                    // Normal statement
                    if (indent <= parent_indent){
                        if (block_node->block.count == 0){
                            raiseError(SYNTAX_ERROR,"Improper Indentation");
                            goto mistake;
                        }
                        current = 0;
                        goto end;
                    }
                    if (!update_block(block_node,stmt)) goto mistake;
                }
            }
        }
    }
    mistake:
        reset_tokens();
        allocate_tokens();
        add_token(TOKEN_EOF, "", 0);
        ast_free(block_node);
        return 0;
    end:
        parent_node->construct.code = block_node;
        return 1;
}

ASTNode* parse_if(){
    int indent = 0, j = 0;
    while (tokens[j].type == TOKEN_INDENT){
        j++; indent++;
    }
    ASTNode* node = new_node();
    if (!node) return NULL;
    ASTNode* condition = parse_expression();
    if (!condition) return NULL;
    node->type = AST_IF;
    node->construct.condition = condition;
    node->construct.code = NULL;
    node->construct.next = NULL;
    if (advance().type == TOKEN_COLON){
        if(peek().type == TOKEN_EOF){
            advance();
            if (!block(node, indent)) goto end;
            return node;
        }
    }
    raiseError(SYNTAX_ERROR, "Missing colon");
    end:
        ast_free(node);
        return NULL;
}

ASTNode* parse_elif(){
    int indent = 0, j = 0;
    while (tokens[j].type == TOKEN_INDENT){
        j++; indent++;
    }
    ASTNode* node = new_node();
    if (!node) return NULL;
    ASTNode* condition = parse_expression();
    if (!condition) return NULL;
    node->type = AST_ELIF;
    node->construct.condition = condition;
    node->construct.code = NULL;
    node->construct.next = NULL;
    if (advance().type == TOKEN_COLON){
        if(peek().type == TOKEN_EOF){
            advance();
            if (!block(node, indent)) goto end;
            return node;
        }
    }
    raiseError(SYNTAX_ERROR, "Missing colon");
    end:
        ast_free(node);
        return NULL;
}

ASTNode* parse_else(){
    int indent = 0, j = 0;
    while (tokens[j].type == TOKEN_INDENT){
        j++; indent++;
    }
    ASTNode* node = new_node();
    if (!node) return NULL;
    node->type = AST_ELSE;
    node->construct.condition = NULL;
    node->construct.code = NULL;
    node->construct.next = NULL;
    if (advance().type == TOKEN_COLON){
        if(peek().type == TOKEN_EOF){
            advance();
            if (!block(node, indent)) goto end;
            return node;
        }
    }
    raiseError(SYNTAX_ERROR, "Missing colon");
    end:
        ast_free(node);
        return NULL;
}

ASTNode* parse_while(){
    int indent = 0, j = 0;
    while (tokens[j].type == TOKEN_INDENT){
        j++; indent++;
    }
    ASTNode* node = new_node();
    if (!node) return NULL;
    ASTNode* condition = parse_expression();
    if (!condition) return NULL;
    node->type = AST_WHILE;
    node->construct.condition = condition;
    node->construct.code = NULL;
    node->construct.next = NULL;
    if (advance().type == TOKEN_COLON){
        if(peek().type == TOKEN_EOF){
            advance();
            if (!block(node, indent)) goto end;
            return node;
        }
    }
    raiseError(SYNTAX_ERROR, "Missing colon");
    end:
        ast_free(node);
        return NULL;
}

ASTNode* parse_keyword(ASTNode* parent_node) {
    char* key = strdup(advance().text);// skip 'keyword' and get the keyword
    if (strcasecmp(key, "print") == 0){
        if (peek().type == TOKEN_LPAREN){
            ASTNode* val = parse_paren();
            ASTNode* node = new_node();
            if (!node) return NULL;
            node->type = AST_PRINT;
            node->print.value = val;
            return node;
        }else{
            raiseError(SYNTAX_ERROR, "Missing brackets");
            return NULL;
        }
    }else if (strcasecmp(key, "pass") == 0){
        return parse_pass();
    }else if (strcasecmp(key, "break") == 0){
        return parse_break();
    }else if (strcasecmp(key, "if") == 0){
        return parse_if();
    }else if (strcasecmp(key, "elif") == 0){
        if (!parent_node){raiseError(SYNTAX_ERROR, "Elif without matching If"); goto end;}
        return parse_elif();
    }else if (strcasecmp(key, "else") == 0){
        if (!parent_node){raiseError(SYNTAX_ERROR, "Else without matching If"); goto end;}
        return parse_else();
    }else if (strcasecmp(key, "while") == 0){
        return parse_while();
    }else{
        end:
            return NULL;
    }
}

ASTNode* parse_statement(ASTNode* parent_node) {
    if (peek().type == TOKEN_SEMICOLON){ 
        current++;
        return NULL;
    }
    if (peek().type == TOKEN_KEYWORD) {
        return parse_keyword(parent_node);
    }
    if (peek().type == TOKEN_IDENTIFIER && tokens[current + 1].type == TOKEN_ASSIGN) {
        return parse_assignment();
    }
    return parse_expression();
}