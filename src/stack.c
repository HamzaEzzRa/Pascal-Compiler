#include <stdio.h>
#include <string.h>
#include "stack.h"

#define STACK_BUFFER_SIZE 100
#define stack_size(stack_ptr) ( (stack_ptr)->top - (stack_ptr)->base )

int n = 1;

void init_stack(Stack *stack_ptr) {
    stack_ptr->base = malloc(sizeof(void *)*STACK_BUFFER_SIZE*n);
    stack_ptr->top = stack_ptr->base;
}

int is_empty_stack(Stack *stack_ptr) {
    return stack_ptr->top == stack_ptr->base;
}

void stack_push(Stack *stack_ptr, void *data_ptr) {
    if (stack_size(stack_ptr) >= STACK_BUFFER_SIZE) {
        Stack *new_ptr = realloc(stack_ptr, sizeof(void *)*STACK_BUFFER_SIZE*++n);
        if (new_ptr == NULL) {
            // Error ...
        }
        stack_ptr = new_ptr;

        *stack_ptr->top = data_ptr;
        stack_ptr->top++;
    }
}

void* stack_pop(Stack *stack_ptr) {
    if (is_empty_stack(stack_ptr))
        return NULL;
    stack_ptr->top--;
    return *stack_ptr->top;
}
