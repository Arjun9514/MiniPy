// lexer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "error_handling.h"
// #include "debug_alloc.h"

#define MAX_TOKENS 1000

Token* tokens;
int token_count = 0;

extern int error;
extern int debug;
extern int current;

extern char *keywords[];
extern const int num_keywords;

char* strndup(const char* s, size_t n) {
    char* out = malloc(n + 1);
    if (!out) return NULL;
    strncpy(out, s, n);
    out[n] = '\0';
    return out;
}

char* process_str(const char* s, size_t len, int size){
    char* out = malloc(size + 1);
    if (!out) return NULL;
    char* p = out;
    const char* end = s + len;
    while (s < end) {
        if (*s == '\\' && s + 1 < end) {
            s++;
            switch (*s) {
                case 'n':  *p++ = '\n'; break;
                case 'r':  *p++ = '\r'; break;
                case 't':  *p++ = '\t'; break;
                case '\\': *p++ = '\\'; break;
                case '\'': *p++ = '\''; break;
                case '"':  *p++ = '"';  break;
                default:
                    char msg[255];
                    sprintf(msg,"Invalid escape sequence ->\'\\%c\'",*s);
                    raiseError(LITERAL_ERROR,msg);
                    return NULL;
            }
            s++;
        }
        else {
            *p++ = *s++;
        }
    }
    return out;
}

int is_keyword(const char* str) {
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int is_bool(const char* str) {
    if (strcmp(str, "True") == 0 || strcmp(str, "False") == 0) {
        return 1;
    }
    return 0;
}

int is_andor(const char* str) {
    if (strcmp(str, "and") == 0 || strcmp(str, "or") == 0) {
        return 1;
    }
    return 0;
}

int is_none(const char* str) {
    if (strcmp(str, "None") == 0) {
        return 1;
    }
    return 0;
}

void print_tokens_debug(){
    for (int i = 0; i < token_count; i++) {
        printf("%s(%s)\n", token_name(tokens[i].type), tokens[i].text);
    }
}

const char* token_name(TokenType type) {
    switch (type) {
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMERIC: return "NUMERIC";
        case TOKEN_FLOATING_POINT: return "FLOATING_POINT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_BOOLEAN: return "BOOLEAN";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_LPAREN: return "L_PAREN";
        case TOKEN_RPAREN: return "R_PAREN";
        case TOKEN_BRACE_OPEN: return "L_BRACE";
        case TOKEN_BRACE_CLOSE: return "R_BRACE";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_EOF: return "EOF";
        case TOKEN_INDENT: return "INDENT";
        case TOKEN_SEMICOLON: return "SEMI_COLON";
        case TOKEN_COLON: return "COLON";
        default: return "UNKNOWN";
    }
}

void add_token(TokenType type, const char* start, int length) {
    Token* tok = &tokens[token_count++];
    tok->type = type;
    tok->text = strndup(start, length);
}

void allocate_tokens(){
    tokens = malloc(sizeof(Token) * MAX_TOKENS);
}

void reset_tokens() {
    for (int i = 0; i < token_count; i++) {
        free(tokens[i].text);
        tokens[i].text = NULL;
    }
    free(tokens);
    tokens = NULL;
    token_count = 0;
    error = 0;
    current = 0;
}

void tokenize(const char* src) {
    const char* p = src;
    int spaces = 0;

    while (*p) {
        if(*p == '\t'){
            add_token(TOKEN_INDENT,"",0);
            p++; continue;
        }

        if (isspace(*p)) {
            spaces++;
            if (spaces == 4){
                spaces = 0;
                add_token(TOKEN_INDENT,"",0);
            }
            p++; continue;
        }

        spaces = 0;

        if (isalpha(*p) || *p == '_') {
            const char* start = p;
            while (isalnum(*p) || *p == '_') p++;
            int len = p - start;
            char* str = strndup(start, len);
            if (is_keyword(str)) {
                if (is_bool(str)){
                    add_token(TOKEN_BOOLEAN, start, len);
                }else if (is_none(str)){
                    add_token(TOKEN_NONE, start, len);
                }else if (is_andor(str)){
                    char* op = (strcmp(str, "and") == 0) ? "&" : "|";
                    add_token(TOKEN_OPERATOR, op, 1);
                }else if (strcmp(str, "not") == 0){
                    add_token(TOKEN_OPERATOR, "!", 1);
                }else if(strcasecmp(str,"debug") == 0){
                    p++;
                    switch (*p){
                    case '1': debug = 1; break;
                    case '0': debug = 0; break;
                    default:    
                        raiseError(SYNTAX_ERROR, "Improper command parameters");
                        break;
                    }    
                    break;
                }else{
                    add_token(TOKEN_KEYWORD, start, len);
                }
            } else {
                add_token(TOKEN_IDENTIFIER, start, len);
            }
            free(str);
            continue;
        }

        if (isdigit(*p) || *p == '.') {
            const char* start = p;
            int fp = 0;
            if (*p == '.') {
                fp++;
                p++;
                if (!isdigit(*p)) {
                    raiseError(VALUE_ERROR, "Improper floating point literal (dot not followed by digit)");
                    break;
                }
                while (isdigit(*p) || *p == '.'){ 
                    p++;
                    if (*p == '.') fp++;
                }
            } else {
                while (isdigit(*p) || *p == '.'){ 
                    p++;
                    if (*p == '.') fp++;
                }
            }
        
            if (fp > 1) {
                raiseError(VALUE_ERROR, "Improper floating point literal (multiple dots)");
                break;
            }
        
            if (fp == 1) {
                add_token(TOKEN_FLOATING_POINT, start, p - start);
            } else {
                add_token(TOKEN_NUMERIC, start, p - start);
            }
            continue;
        }
        
        if (*p == '\"' || *p == '\'') {
            int chr = 0;
            char quote_type = *p;           // Either ' or "
            const char* start = p + 1;      // Skip opening quote
            p++;
            
            // Go until we find the matching quote or end of string
            while (*p && *p != quote_type) {
                if (*p == '\\' && *(p+1)) { // Handle escaped characters
                    p += 2; // skip \"
                } else {
                    p++;
                }
                chr++;
            }
        
            if (*p == quote_type) {
                const char* str = process_str(start, p - start, chr);
                if (!str) break;
                add_token(TOKEN_STRING, str, chr);
                p++; // Skip closing quote
                continue;
            } else {
                raiseError(LITERAL_ERROR,"Unterminated string literal");
                break;
            }
        }

        switch (*p) {
            case '=': 
                if(*(p+1) == '='){
                    char *op = "e";
                    add_token(TOKEN_OPERATOR, op, 1);
                    p++;
                }else{
                    add_token(TOKEN_ASSIGN, p, 1); 
                }
                break;
            case '!': 
                if(*(p+1) == '='){
                    char *op = "n";
                    add_token(TOKEN_OPERATOR, op, 1);
                    p++;
                }else{
                    add_token(TOKEN_UNKNOWN, p, 1); 
                    raiseError(SYNTAX_ERROR, "Improper token used");
                }
                break;
            case '>': 
                if(*(p+1) == '='){
                    char *op = "g";
                    add_token(TOKEN_OPERATOR, op, 1);
                    p++;
                }else{
                    add_token(TOKEN_OPERATOR, p, 1); 
                }
                break;
            case '<': 
                if(*(p+1) == '='){
                    char *op = "l";
                    add_token(TOKEN_OPERATOR, op, 1);
                    p++;
                }else{
                    add_token(TOKEN_OPERATOR, p, 1); 
                }
                break;
            case '+': add_token(TOKEN_OPERATOR, p, 1); break;
            case '-': add_token(TOKEN_OPERATOR, p, 1); break;
            case '*': add_token(TOKEN_OPERATOR, p, 1); break;
            case '/': add_token(TOKEN_OPERATOR, p, 1); break;
            case '(': add_token(TOKEN_LPAREN, p, 1); break;
            case ')': add_token(TOKEN_RPAREN, p, 1); break;
            case '{': add_token(TOKEN_BRACE_OPEN, p, 1); break;
            case '}': add_token(TOKEN_BRACE_CLOSE, p, 1); break;
            case ';': add_token(TOKEN_SEMICOLON, p, 1); break;
            case ':': add_token(TOKEN_COLON, p, 1); break;
            default:
                add_token(TOKEN_UNKNOWN, p, 1); 
                raiseError(SYNTAX_ERROR, "Improper token used");
                break;
        }
        p++;
    }

    add_token(TOKEN_EOF, "", 0);
}