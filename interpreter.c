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

int is_truthy(Literal val) {
    switch (val.datatype) {
        case BOOLEAN: return val.boolean != 0;
        case INT: return val.numeric != 0;
        case FLOAT: return val.floating_point != 0.0;
        case STRING: return val.string && strlen(val.string) > 0;
        case NONE: return 0;
        default: return 0;
    }
}

ASTNode* operate(ASTNode* node){
    Literal left_val, right_val, result;
    left_val.owns_str = 0;
    right_val.owns_str = 0;
    result.owns_str = 0;
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
        case AST_STRING: 
            left_val = copy_literal(node->operate.left->literal);
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
        case AST_STRING:
            right_val = copy_literal(node->operate.right->literal);
            break;
        default:  
            goto type_error;
    }

    char op = node->operate.op;
    
    //Check for zero_division error
    switch (op){
        case '/': {
            switch (right_val.datatype){
                case INT: if(right_val.numeric == 0); goto zero_division_error;
                case FLOAT: if(right_val.floating_point == 0.0); goto zero_division_error;
                case BOOLEAN: if(right_val.boolean == 0); goto zero_division_error;
                default:break;
            }
            break;
        }
        case '&': case '|': case '!': goto andornot;
    }

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
            result.owns_str = 1;
        } else {
            goto type_error;
        }
    } else if (left_val.datatype == STRING && right_val.datatype == INT) {
        result.datatype = STRING;
        if (op == '*') {
            if (right_val.numeric > 0){
                size_t len_l = strlen(left_val.string);  
                char *buf = malloc((len_l * right_val.numeric) + 1);
                if (buf == NULL) {
                    raiseError(MEMORY_ERROR, "Memory allocation failed");
                    return NULL;
                }
                size_t k = 0;
                while (k < (size_t)right_val.numeric) {
                    memcpy(buf + (len_l * k), left_val.string, len_l);
                    k++;
                }
                buf[len_l * right_val.numeric] = '\0'; // Null-terminate the string
                result.string = buf;
                result.owns_str = 1;
            } else {
                result.string = "";
            }
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
        andornot:
            int left_true = is_truthy(left_val);
            int right_true = is_truthy(right_val);
            switch (op){
                case '&':
                    if (left_true == 0) {
                        result = copy_literal(left_val);
                    }else{
                        result = copy_literal(right_val);
                    }
                    break;
                case '|':
                    if (left_true != 0) {
                        result = copy_literal(left_val);
                    }else{
                        result = copy_literal(right_val);
                    }
                    break;
                case '!':
                    result.datatype = BOOLEAN;
                    result.boolean = !right_true;
                    break;
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
    
    if (left_val.owns_str) {
        free(left_val.string);
    }
    if (right_val.owns_str) {
        free(right_val.string);
    } 
    
    return temp;
}

int eval(ASTNode* node) {
    if (node != NULL){
        switch (node->type) {
            case AST_NUMERIC:
            case AST_FLOATING_POINT:
            case AST_STRING:
            case AST_BOOLEAN:
                print_literal(node->literal);
                break;

            case AST_NONE:
            case AST_PASS:
                break;

            case AST_BREAK:
                return 1;

            case AST_IDENTIFIER:{
                Literal lit = get_variable(node->name);
                if (lit.datatype != ERROR) print_literal(lit);
                break;
            }

            case AST_PRINT:
                eval(node->print.value); break;

            case AST_BLOCK:
                for(int i = 0; i < node->block.count; i++){
                    if(eval(node->block.statements[i])) return 1;
                }
                break;

            case AST_IF:
            case AST_ELIF:{
                ASTNode* condn = node->construct.condition;
                Literal lit;
                if (condn->type == AST_OPERATOR){
                    ASTNode* temp = operate(condn);
                    if (!temp) break;
                    lit = copy_literal(temp->literal);
                    ast_free(temp);
                } else if (condn->type == AST_IDENTIFIER){
                    lit = get_variable(condn->name);
                } else {
                    lit = copy_literal(condn->literal);
                }
                if (is_truthy(lit)){
                    if(eval(node->construct.code)) return 1;
                } else {
                    if(eval(node->construct.next)) return 1;
                }
                if(lit.owns_str) free(lit.string);
                break;
            }

            case AST_ELSE:
                if(eval(node->construct.code)) return 1;    
                break;
            
            case AST_WHILE:{
                ASTNode* condn = node->construct.condition;
                Literal lit;
                while(1){
                    if (condn->type == AST_OPERATOR){
                        ASTNode* temp = operate(condn);
                        if (!temp) break;
                        lit = copy_literal(temp->literal);
                        ast_free(temp);
                    } else if (condn->type == AST_IDENTIFIER){
                        lit = get_variable(condn->name);
                    } else {
                        lit = copy_literal(condn->literal);
                    }
                    if(is_truthy(lit)) {
                        if(eval(node->construct.code)) break;
                    }else{
                        if(eval(node->construct.next)) return 1;
                        break;
                    }
                    if(lit.owns_str) free(lit.string);
                }
                if(lit.owns_str) free(lit.string);
                break;
            }

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
    return 0;
}