//interpreter.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "interpreter.h"
#include "memory.h"
#include "error_handling.h"

extern char error_msg[255];
extern int error;
extern const char* AST_node_name(ASTNodeType type);

ASTNode* operate(ASTNode* node){
    Value left_val, right_val;

    ASTNode* temp = malloc(sizeof(ASTNode));

    // Resolve left value
    if (node->operate.left->type == AST_OPERATOR) {
        node->operate.left = operate(node->operate.left);
    }
    if (node->operate.left->type == AST_IDENTIFIER) {
        left_val = get_variable(node->operate.left->name);
    } else if (node->operate.left->type == AST_FLOATING_POINT) {
        left_val.datatype = 'f';
        left_val.floating_point = node->operate.left->floating_point;
    } else if (node->operate.left->type == AST_NUMERIC) {
        left_val.datatype = 'd';
        left_val.numeric = node->operate.left->numeric;
    }else if (node->operate.left->type == AST_STRING) {
        left_val.datatype = 's';
        left_val.string = node->operate.left->string;
    }

    // Resolve right value
    if (node->operate.right->type == AST_OPERATOR) {
        node->operate.right = operate(node->operate.right);
    }
    if (node->operate.right->type == AST_IDENTIFIER) {
        right_val = get_variable(node->operate.right->name);
    } else if (node->operate.right->type == AST_FLOATING_POINT) {
        right_val.datatype = 'f';
        right_val.floating_point = node->operate.right->floating_point;
    } else if (node->operate.right->type == AST_NUMERIC) {
        right_val.datatype = 'd';
        right_val.numeric = node->operate.right->numeric;
    }else if (node->operate.right->type == AST_STRING) {
        right_val.datatype = 's';
        right_val.string = node->operate.right->string;
    }
    
    char op = node->operate.op;
    
    // Numeric operation
    if (left_val.datatype == 'd' && right_val.datatype == 'd') {
        temp->type = AST_NUMERIC;
        int l = left_val.numeric, r = right_val.numeric;
        if (op == '+') temp->numeric = l + r;
        else if (op == '-') temp->numeric = l - r;
        else if (op == '*') temp->numeric = l * r;
        else if (op == '/') {temp->type = AST_FLOATING_POINT;temp->floating_point = (float)l / r;}
    }
    // Floating point operation
    else if (left_val.datatype == 'f' && right_val.datatype == 'f') {
        temp->type = AST_FLOATING_POINT;
        float l = left_val.floating_point, r = right_val.floating_point;
        if (op == '+') temp->floating_point = l + r;
        else if (op == '-') temp->floating_point = l - r;
        else if (op == '*') temp->floating_point = l * r;
        else if (op == '/') temp->floating_point = l / r;
    }else if ((left_val.datatype == 'd' && right_val.datatype == 'f') ||
                (left_val.datatype == 'f' && right_val.datatype == 'd')) {
        temp->type = AST_FLOATING_POINT;
        float l = (left_val.datatype == 'f') ? left_val.floating_point : (float)left_val.numeric;
        float r = (right_val.datatype == 'f') ? right_val.floating_point : (float)right_val.numeric;
        if (op == '+') temp->floating_point = l + r;
        else if (op == '-') temp->floating_point = l - r;
        else if (op == '*') temp->floating_point = l * r;
        else if (op == '/') temp->floating_point = l / r;
    }
    //String concatenation
    else if(left_val.datatype == 's' && right_val.datatype == 's'){
        temp->type = AST_STRING;
        if (op == '+'){
            strcat(left_val.string,right_val.string);
            temp->string = left_val.string;
        }else{
            goto if_error;
        }
    }else {
        if_error:
            char msg[255];
            sprintf(msg, "Unsupported operand type(s) for \'%c\': \'%c\' and \'%c\'", op, left_val.datatype, right_val.datatype);
            raiseError(TYPE_ERROR, msg);
            return NULL;
    }
    return temp;
}

void eval(ASTNode* node) {
    if (node != NULL){
        switch (node->type) {
            case AST_NUMERIC:
                printf("%d\n", node->numeric); break;
            case AST_FLOATING_POINT:
                printf("%f\n", node->floating_point); break;
            case AST_STRING:
                printf("\'%s\'\n", node->string); break;

            case AST_IDENTIFIER:{
                Value val = get_variable(node->name);
                switch (val.datatype) {
                    case 'd': printf("%d\n", val.numeric); break;
                    case 'f': printf("%f\n", val.floating_point); break;
                    case 's': printf("%s\n", val.string); break;
                    default:
                        goto if_error;
                }
                break;
            }

            case AST_KEYWORD:
                // printf("%s",node->keyword.key);
                eval(node->keyword.value); break;

            case AST_OPERATOR: {
                ASTNode* temp = operate(node);
                eval(temp);break;
            }

            case AST_ASSIGNMENT:{
                ASTNode* val = node->assign.value;
                switch (val->type) {
                    case AST_NUMERIC:
                        set_variable(node->assign.name, 'd', val->numeric);
                        break;
                    case AST_FLOATING_POINT:
                        set_variable(node->assign.name, 'f', val->floating_point);
                        break;
                    case AST_STRING:
                        set_variable(node->assign.name, 's', val->string);
                        break;
                    case AST_OPERATOR:
                        ASTNode* temp1 = operate(val);
                        ASTNode* temp2 = malloc(sizeof(ASTNode));
                        temp2->type = AST_ASSIGNMENT;
                        temp2->assign.name=node->assign.name;
                        temp2->assign.value = temp1;
                        eval(temp2);
                        break;
                    case AST_IDENTIFIER:
                        Value val_of = get_variable(val->name);
                        switch (val_of.datatype)
                        {
                        case 'd':set_variable(node->assign.name, val_of.datatype, val_of.numeric);break;
                        case 'f':set_variable(node->assign.name, val_of.datatype, val_of.floating_point);break;
                        case 's':set_variable(node->assign.name, val_of.datatype, val_of.string);break;
                        default:
                            if_error:
                                char msg[255] = "Undefined variable -> ";
                                strcat(msg,node->name);
                                raiseError(NAME_ERROR, msg);
                                break;
                        }
                        break;
                    default:
                        printf("%s node\n", AST_node_name(val->type));
                        raiseError(ASSIGNMENT_ERROR, "Unsupported assignment type");
                        break;
                }
                break;
            }
        }
    }
}