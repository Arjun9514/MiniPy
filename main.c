// main.c
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "interpreter.h"
#include "error_handling.h"

extern int current;
extern int error;

const char* keywords[] = {"exit","print","if","else","True","False"}; 
const int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

int debug = 0;

int interactive(){
    char input[255];
    do{
        current = 0;
        
        printf(">> ");
        scanf("%[^\n]%*c",input);

        tokenize(input);

        if (error)goto end;
        
        if (debug) print_tokens_debug();//for debugging

        if(strcasecmp(input,"exit") == 0) break;

        while (peek().type != TOKEN_EOF) {
            ASTNode* stmt = parse_statement();
            if (error) goto end;

            if (debug){//for debugging
                printf("************AST**************\n"); 
                print_ast_debug(stmt,0); 
                printf("*****************************\n");
            }
            
            eval(stmt);
            if (error) goto end;
        }
        end:reset_tokens();
    }while(1);
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