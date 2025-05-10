//interpreter.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "interpreter.h"
#include "memory.h"
#include "error_handling.h"
// #include "debug_alloc.h"

extern const char* AST_node_name(ASTNodeType type);

void print_literal(Literal lit){
    switch (lit.datatype) {
        case INT: printf("%d\n", lit.numeric); break;
        case FLOAT: printf("%f\n", lit.floating_point); break;
        case STRING: printf("\'%s\'\n", lit.string); break;
        case BOOLEAN: {
            if (lit.boolean){
                printf("True\n"); break;
            }
            printf("False\n"); break;
        }
    }
}

ASTNode* operate(ASTNode* node){
    Literal left_val, right_val, result;

    ASTNode* temp = malloc(sizeof(ASTNode));

    // Resolve left value
    if (node->operate.left->type == AST_OPERATOR) {
        node->operate.left = operate(node->operate.left);
    }
    switch (node->operate.left->type){
        case AST_IDENTIFIER:
            Literal lit = get_variable(node->operate.left->name);
            if(lit.datatype == ERROR){
                free(temp);
                return NULL;
            }
            left_val = copy_literal(lit);
            break;
        case AST_NONE:
        case AST_NUMERIC: 
        case AST_FLOATING_POINT: 
        case AST_BOOLEAN: 
            left_val = node->operate.left->literal;
            left_val.owns_str = 0; break;
        case AST_STRING: 
            left_val.datatype = STRING; 
            left_val.string = strdup(node->operate.left->literal.string);
            left_val.owns_str = 1;
            break;
        default: goto type_error;
    }

    // Resolve right value
    if (node->operate.right->type == AST_OPERATOR) {
        node->operate.right = operate(node->operate.right);
    }
    switch (node->operate.right->type){
        case AST_IDENTIFIER:
            Literal lit = get_variable(node->operate.right->name);
            if(lit.datatype == ERROR){
                free(temp);
                return NULL;
            }
            right_val = copy_literal(lit);
            break;
        case AST_NONE:
        case AST_NUMERIC: 
        case AST_FLOATING_POINT: 
        case AST_BOOLEAN: 
            right_val = node->operate.right->literal;
            right_val.owns_str = 0; break;
        case AST_STRING:
            right_val.datatype = STRING; 
            right_val.string = strdup(node->operate.right->literal.string);
            right_val.owns_str = 1;
            break;
        default:  
            goto type_error;
    }

    char op = node->operate.op;
    
    //Check for zero_division error
    if (op == '/'){
        switch (right_val.datatype){
            case INT: if(right_val.numeric == 0); goto zero_division_error;
            case FLOAT: if(right_val.floating_point == 0.0); goto zero_division_error;
            case BOOLEAN: if(right_val.boolean == 0); goto zero_division_error;
            default:break;
        }
    }

    result.owns_str = 0;

    // Numeric & Boolean operation
    if ((left_val.datatype == INT && right_val.datatype == INT) || 
        (left_val.datatype == BOOLEAN && right_val.datatype == BOOLEAN) ||
        ((left_val.datatype == INT && right_val.datatype == BOOLEAN) || 
        (left_val.datatype == BOOLEAN && right_val.datatype == INT))) {
        int l = (left_val.datatype == BOOLEAN) ? left_val.boolean : left_val.numeric;
        int r = (right_val.datatype == BOOLEAN) ? right_val.boolean : right_val.numeric;
        result.datatype = INT;
        switch (op){
            case '+': result.numeric = l + r; break;
            case '-': result.numeric = l - r; break;
            case '*': result.numeric = l * r; break;
            case '/': result.datatype = FLOAT; result.floating_point = (float)l / r; break;
            case '&': 
                result.numeric = (l == 0) ? 0 : (r == 0) ? 0 : r;
                if (left_val.datatype == BOOLEAN && right_val.datatype == BOOLEAN) result.datatype = BOOLEAN;
                break;
            case '|': 
                result.numeric = (l != 0) ? l : ((r != 0) ? r : 0);
                if (left_val.datatype == BOOLEAN && right_val.datatype == BOOLEAN) result.datatype = BOOLEAN;
                break;
            case '>': case '<': case 'g': case 'e': case 'l': case 'n': goto comparative_operation;
            default: goto type_error;
        }
    }
    // Floating point operation
    else if ((left_val.datatype == FLOAT && right_val.datatype == FLOAT) || 
            (((left_val.datatype == INT || left_val.datatype == BOOLEAN) && right_val.datatype == FLOAT) ||
            (left_val.datatype == FLOAT && (right_val.datatype == INT || right_val.datatype == BOOLEAN)))) {
        float l = (left_val.datatype == FLOAT) ? left_val.floating_point : (left_val.datatype == INT) ? (float)left_val.numeric : (float)left_val.boolean;
        float r = (right_val.datatype == FLOAT) ? right_val.floating_point : (right_val.datatype == INT) ? (float)right_val.numeric : (float)right_val.boolean;
        result.datatype = FLOAT;
        switch (op){
            case '+': result.floating_point = l + r; break;
            case '-': result.floating_point = l - r; break;
            case '*': result.floating_point = l * r; break;
            case '/': result.floating_point = l / r; break;
            case '>': case '<': case 'g': case 'e': case 'l': case 'n': goto comparative_operation;
            default: goto type_error;
        }
    }
    //String operation
    else if (left_val.datatype == STRING && right_val.datatype == STRING) {
        result.datatype = STRING;
        if (op == '+') {
            // printf("[DEBUG] Concatenating '%s' + '%s'\n", left_val.string, right_val.string);
            size_t len_l = strlen(left_val.string);
            size_t len_r = strlen(right_val.string);
            char *buf = malloc(len_l + len_r + 1);
            if (buf == NULL) {
                raiseError(MEMORY_ERROR, "Memory allocation failed");
                return NULL;
            }
            memcpy(buf, left_val.string, len_l);
            memcpy(buf + len_l, right_val.string, len_r);
            buf[len_l + len_r] = '\0'; // Null-terminate the string
    
            result.string = buf;
            if (left_val.owns_str) {
                free(left_val.string);
            }
            if (right_val.owns_str) {
                free(right_val.string);
            }
            result.owns_str = 1;
            // printf("[DEBUG] Concatenated result: '%s'\n", result.string);
        } else {
            goto type_error;
        }
    }
    
    else {
        type_error:
            char msg[255];
            if (left_val.owns_str) {
                free(left_val.string);
            }
            if (right_val.owns_str) {
                free(right_val.string);
            }  
            sprintf(msg, "Unsupported operand type(s) for \'%c\': \'%c\' and \'%c\'", op, left_val.datatype, right_val.datatype);
            raiseError(TYPE_ERROR, msg);
            free(temp);
            return NULL;
        zero_division_error:
            raiseError(ZERO_DIVISION_ERROR, "Division by zero");
            free(temp);
            return NULL;
        //Comparative Operation
        comparative_operation: 
            float l = (left_val.datatype == FLOAT) ? left_val.floating_point : (left_val.datatype == INT) ? (float)left_val.numeric : (float)left_val.boolean;
            float r = (right_val.datatype == FLOAT) ? right_val.floating_point : (right_val.datatype == INT) ? (float)right_val.numeric : (float)right_val.boolean;
            result.datatype = BOOLEAN;
            switch (op){
                case '>': result.boolean = l > r; break;
                case '<': result.boolean = l < r; break;
                case 'g': result.boolean = l >= r; break;
                case 'e': result.boolean = l == r; break;
                case 'l': result.boolean = l <= r; break;
                case 'n': result.boolean = l != r; break;
            }
    }

    switch (result.datatype){
        case INT: temp->type = AST_NUMERIC; break;
        case FLOAT: temp->type = AST_FLOATING_POINT; break;
        case STRING: temp->type = AST_STRING; break;
        case BOOLEAN: temp->type = AST_BOOLEAN; break;
        default:break;
    }
    temp->literal = result;
    return temp;
}

void eval(ASTNode* node) {
    if (node != NULL){
        switch (node->type) {
            case AST_NUMERIC:
            case AST_FLOATING_POINT:
            case AST_STRING:
            case AST_BOOLEAN:
                print_literal(node->literal);
                break;
            case AST_NONE:
                break;
            case AST_IDENTIFIER:{
                Literal lit = get_variable(node->name);
                if (lit.datatype != ERROR) print_literal(lit);
                break;
            }

            case AST_PRINT:
                eval(node->print.value); break;

            case AST_BLOCK:
                for(int i = 0; i < node->block.count; i++){
                    eval(node->block.statements[i]);
                }
                break;

            case AST_IF:
            case AST_ELIF:
                ASTNode* condn = node->if_else.condition;
                ASTNode* temp;
                if (condn->type != AST_OPERATOR && condn->type != AST_IDENTIFIER) {
                    temp = new_node();
                    temp->type = condn->type;
                    temp->literal = copy_literal(condn->literal);
                } else if (condn->type == AST_OPERATOR){
                    temp = operate(node->if_else.condition);
                    if (!temp) break;
                } else if (condn->type == AST_IDENTIFIER){
                    temp = new_node();
                    Literal lit = get_variable(node->if_else.condition->name);
                    temp->literal = copy_literal(lit);
                    switch (lit.datatype){
                        case INT: temp->type = AST_NUMERIC; break;
                        case FLOAT: temp->type = AST_FLOATING_POINT; break;
                        case STRING: temp->type = AST_STRING; break;
                        case BOOLEAN: temp->type = AST_BOOLEAN; break;
                        default:break;
                    }
                }
                switch (temp->type) {
                    case AST_NUMERIC:
                        if (temp->literal.numeric != 0) {eval(node->if_else.code); break;}
                        eval(node->if_else.next); break;
                    case AST_FLOATING_POINT:
                        if (temp->literal.floating_point != 0.0) {eval(node->if_else.code); break;}
                        eval(node->if_else.next); break;
                    case AST_STRING:
                        if (strlen(temp->literal.string) != 0) {eval(node->if_else.code); break;}
                        eval(node->if_else.next); break;
                    case AST_BOOLEAN:
                        if (temp->literal.boolean != 0) {eval(node->if_else.code); break;}
                        eval(node->if_else.next); break;
                    case AST_NONE:
                        eval(node->if_else.next); break;
                    default:
                        eval(node->if_else.next);
                        break;
                }
                ast_free(temp);
                break;
            
            case AST_ELSE:
                eval(node->if_else.code);    
                break;

            case AST_OPERATOR: {
                ASTNode* temp = operate(node);
                if (!temp) break;
                eval(temp);
                if(temp->literal.owns_str) free(temp->literal.string);
                free(temp);
                break;
            }

            case AST_ASSIGNMENT:{
                ASTNode* sub_node = node->assign.value;
                switch (sub_node->type) {
                    case AST_NONE:
                    case AST_NUMERIC:
                    case AST_FLOATING_POINT:
                    case AST_STRING:
                    case AST_BOOLEAN:
                        set_variable(node->assign.name, sub_node->literal);
                        break;
                
                    case AST_OPERATOR:
                        ASTNode* temp1 = operate(sub_node);
                        if(!temp1)break;
                        ASTNode* temp2 = malloc(sizeof(ASTNode));
                        temp2->type = AST_ASSIGNMENT;
                        temp2->assign.name=node->assign.name;
                        temp2->assign.value = temp1;
                        eval(temp2);
                        if(temp1->literal.datatype == STRING) free(temp1->literal.string);
                        free(temp1);
                        if(temp2->literal.datatype == STRING) free(temp2->literal.string);
                        free(temp2);
                        break;
                    case AST_IDENTIFIER:
                        Literal lit = get_variable(sub_node->name);
                        if (lit.datatype != ERROR) set_variable(node->assign.name, lit);
                        break;
                    default:
                        printf("%s node\n", AST_node_name(sub_node->type));
                        raiseError(ASSIGNMENT_ERROR, "Unsupported assignment type");
                        break;
                }
                break;
            }
        }
    }
}