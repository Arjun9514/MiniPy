// main.c
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "memory.h"
#include "interpreter.h"
#include "colors.h"
#include "error_handling.h"
// #include "debug_alloc.h"

extern int error;
extern int global_indent;

const char* keywords[] = {"exit","print","if","elif","else","True","False","None","debug"}; 
const int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

int debug = 0;

int interactive(){
    char input[255];
    while (1) {
        global_indent = 0;
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