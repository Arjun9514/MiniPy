#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    //DATA TYPES
    TOKEN_NONE,
    TOKEN_NUMERIC,
    TOKEN_FLOATING_POINT,
    TOKEN_STRING,
    TOKEN_BOOLEAN,
    //
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    //BRACKETS
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_BRACE_OPEN,
    TOKEN_BRACE_CLOSE,
    //
    TOKEN_ASSIGN,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char* text;
} Token;

char* strndup(const char* s, size_t n);

void print_tokens_debug();

void add_token(TokenType type, const char* start, int length);

void allocate_tokens();

void reset_tokens();

const char* token_name(TokenType type);

void tokenize(const char* src);

#endif