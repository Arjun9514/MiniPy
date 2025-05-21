// main.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lexer.h"
#include "ast.h"
#include "memory.h"
#include "interpreter.h"
#include "colors.h"
#include "error_handling.h"
// #include "debug_alloc.h"

#define INITIAL_LINE_CAPACITY 100
#define MAX_LINE_LENGTH 1024

extern int error;

const char* keywords[] = {"exit","print","if","elif","else","True",
                        "False","None","debug","and","or","not","pass","while"}; 
const int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

char **lines;

int debug = 0;
int current_line = 0;
int script_ = 0;
int line_count;

void rstrip(char* str) {
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

int interactive(){
    char input[255];
    while (1) {
        printf(MAG ">>> " RESET);
        if (!fgets(input, sizeof input, stdin)) break;
        input[strcspn(input, "\r\n")] = '\0';
        
        if (strlen(input) == 0) continue;

        allocate_tokens();
        tokenize(input);
        
        if (debug){ printf("Tokens:\n"); print_tokens_debug();} //for debugging Tokens

        if (strcasecmp(input, "exit") == 0) {
            reset_tokens();
            break;
        }

        if (error) goto end;

        while (peek().type != TOKEN_EOF) {
            ASTNode* root = parse_statement(NULL);
            if (error){ ast_free(root); goto end;}
            if (debug){ printf("\nAST:\n"); print_ast_debug(root,0,0);} //for debugging AST
            eval(root);
            ast_free(root);
            if (error) goto end;
            if (debug){ printf("\nVariables:\n"); get_variables();} //for debugging Variable Table
        }
        end:
            reset_tokens();
    }    
    return 0;
}

char **load_lines(const char *path, int *line_count_out) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    int capacity = INITIAL_LINE_CAPACITY;
    char **lines = malloc(capacity * sizeof(char *));
    if (!lines) {
        perror("Failed to allocate memory for lines");
        fclose(file);
        return NULL;
    }

    char buffer[MAX_LINE_LENGTH];
    int count = 0;

    while (fgets(buffer, sizeof(buffer), file)) {
        // Remove trailing newline if present
        buffer[strcspn(buffer, "\r\n")] = 0;
        rstrip(buffer);
        if (strlen(buffer) == 0) continue; 
        // printf("%s %d\n", buffer,strlen(buffer));
        // Allocate memory for the line
        lines[count] = malloc(strlen(buffer) + 1);
        if (!lines[count]) {
            perror("Failed to allocate memory for a line");
            break;
        }
        strcpy(lines[count], buffer);
        count++;

        // Resize array if needed
        if (count >= capacity) {
            capacity *= 2;
            char **new_lines = realloc(lines, capacity * sizeof(char *));
            if (!new_lines) {
                perror("Failed to reallocate memory for lines");
                break;
            }
            lines = new_lines;
        }
    }

    fclose(file);

    if (line_count_out) {
        *line_count_out = count;
    }

    return lines;
}

void free_lines(char **lines, int line_count) {
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
}

int script(char *path){
    lines = load_lines("code.sap", &line_count);

    if (!lines) return 1;
    
    while(current_line < line_count){
        char* input = lines[current_line];
        if (debug) printf("%s, %d\n",input, strlen(input));
        current_line++;

        if (strlen(input) == 0) continue;

        allocate_tokens();
        tokenize(input);
        
        if (debug){ printf("Tokens:\n"); print_tokens_debug();} //for debugging Tokens

        if (strcasecmp(input, "exit") == 0) {
            reset_tokens();
            break;
        }

        if (error) goto end;

        while (peek().type != TOKEN_EOF) {
            ASTNode* root = parse_statement(NULL);
            if (error){ ast_free(root); goto end;}
            if (debug){ printf("\nAST:\n"); print_ast_debug(root,0,0);} //for debugging AST
            eval(root);
            ast_free(root);
            if (error) goto end;
            if (debug){ printf("\nVariables:\n"); get_variables();} //for debugging Variable Table
        }
        reset_tokens();
    }
    end:
        exit(1);
    free_lines(lines, line_count);
    return 0;
}

int main(int argc, char *argv[]){
    // script_ = 1;
    // script("");
    if(argc > 1){
        char *path = argv[1];
        script_ = 1;
        script(path);
    }else{
        interactive();
    }
    return 0;
}