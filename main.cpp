#include <cstdio>
#include "stack.hpp"

//вынести main, long long canary без зависимости от типа, %printf спецификатор при смене типа, указывать тип стека при создании

int main() {
    stack_t stack1;
    stackCtor(stack1);

    for (stackElementType i = 0; i < 10; ++i) {
        stackPush(&stack1, i);
        printf("%lld %lld\n", i, stack1.hash);
    }

    stackElementType value;
    for (stackElementType i = 0; i < 10; ++i) {
        stackPop(&stack1, &value);
        printf("%lld\n", value);
    }

    // сломать можно так:
    //stack1.size = 2;

    stackDtor(&stack1);
    return 0;
}