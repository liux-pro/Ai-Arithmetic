#include "expression.h"
#include <stdio.h>
#include <stdlib.h>  // For atoi function

#define MAX_SIZE 16

// 用栈处理运算符优先级
typedef struct {
    float _data[MAX_SIZE];  // 使用 float 类型以支持浮点数
    int top;
} Stack;

static void push(Stack *stack, float value) {
    if (stack->top < MAX_SIZE - 1) {
        stack->_data[++(stack->top)] = value;
    }
}

static float pop(Stack *stack) {
    if (stack->top >= 0) {
        return stack->_data[(stack->top)--];
    }
    return 0.0f; // 空栈返回 0
}

static float peek(Stack *stack) {
    if (stack->top >= 0) {
        return stack->_data[stack->top];
    }
    return 0.0f;
}

static int getPriority(uint8_t op) {
    if (op == 10 || op == 11) { // 加法和减法
        return 1;
    }
    if (op == 12 || op == 13) { // 乘法和除法
        return 2;
    }
    return 0;
}

float expression_calc(uint8_t item[], uint8_t n) {
    Stack numStack, opStack;
    float current, left, right, result;
    uint8_t op;
    uint8_t i;
    numStack.top = -1;  // 初始化栈为空
    opStack.top = -1;

    result = 0.0f; // 初始化结果为 0.0

    i = 0;
    while (i < n) {
        current = item[i];

        // 如果当前是数字 (0-9)，可能是多位数的一部分
        if (current >= 0 && current <= 9) {
            // 处理连续的数字组成多位数
            int number = current;
            i++;
            while (i < n && item[i] >= 0 && item[i] <= 9) {
                number = number * 10 + item[i];
                i++;
            }
            push(&numStack, (float)number);  // 将组合后的数字入栈
        }
        // 如果当前是运算符 (10-13)
        else if (current >= 10 && current <= 13) {
            while (opStack.top >= 0 && getPriority(peek(&opStack)) >= getPriority(current)) {
                right = pop(&numStack);
                left = pop(&numStack);
                op = pop(&opStack);

                switch (op) {
                    case 10: left += right; break;  // 加法
                    case 11: left -= right; break;  // 减法
                    case 12: left *= right; break;  // 乘法
                    case 13: 
                        if (right == 0.0f) {
                            printf("除数不能为0!\n");
                            return 0.0f;  // 防止除以零
                        }
                        left /= right; break;  // 除法
                }
                push(&numStack, left);
            }
            push(&opStack, current);  // 当前运算符入栈
            i++;
        }
    }

    // 处理剩余的运算符
    while (opStack.top >= 0) {
        right = pop(&numStack);
        left = pop(&numStack);
        op = pop(&opStack);

        switch (op) {
            case 10: left += right; break;  // 加法
            case 11: left -= right; break;  // 减法
            case 12: left *= right; break;  // 乘法
            case 13: 
                if (right == 0.0f) {
                    printf("除数不能为0!\n");
                    return 0.0f;  // 防止除以零
                }
                left /= right; break;  // 除法
        }
        push(&numStack, left);
    }

    return pop(&numStack);  // 最终结果
}
