// main.c
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "memory.h"
#include "interpreter.h"
#include "error_handling.h"
// #include "debug_alloc.h"

extern int current;
extern int error;

const char* keywords[] = {"exit","print","if","else","True","False","None"}; 
const int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

int debug = 1;

int interactive(){
    char input[255];
    while (1) {
        current = 0;
        printf(">> ");
        if (!fgets(input, sizeof input, stdin)) break;
        input[strcspn(input, "\r\n")] = '\0';
    
        tokenize(input);
        if (error) {
            reset_tokens();
            error = 0;
            continue;
        }
        if (strcasecmp(input, "exit") == 0) {
            reset_tokens();
            break;
        }
    
        if (peek().type != TOKEN_EOF) {
            ASTNode* root = parse_statement();
            if (error) {
                ast_free(root);
                reset_tokens();
                error = 0;
                continue;
            }
            eval(root);
            ast_free(root);
            if (error) {
                reset_tokens();
                error = 0;
                continue;
            }
            if (debug) get_variables();
        }
    
        reset_tokens();
        error = 0;
    }    
    return 0;
}

int main() {
    interactive();
    return 0;
}

// #include <stdlib.h>
// #include <time.h>
// #include <ctype.h>
// #include <math.h>

// // int loader(char *);
// int interactive();

// int main(int argc, char *argv[]){
//     if(argc > 1){
//          // char *path = argv[1];
//         // loader(path);
//     }else{
//         interactive();
//     }
//     return 0;
// }

// // int loader(char *path){
// //     printf("loading file: %s\n",path);
// //     FILE *pF = fopen(path,"r");
// //     if (pF == NULL) {
// //         perror("Error opening file");
// //         return 1;
// //     }
// //     printf("File loaded successfully!");
// //     char buffer[1024];
// //     while(fgets(buffer,1024,pF) != NULL){
// //         printf("%s",buffer);
// //     }
// //     printf("\n");
// //     fclose(pF);
// //     return 0;
// // }