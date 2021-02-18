#ifndef STACK_H
#define STACK_H

typedef struct {
    void **base;
    void **top;
} Stack;

void init_stack(Stack *stack_ptr);
int empty_stack(Stack *stack_ptr);
void stack_push(Stack *stack_ptr, void *data_ptr);
void* stack_pop(Stack *stack_ptr);

#endif
