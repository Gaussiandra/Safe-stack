#include <cstdio>
#include "stack.hpp"

int main() {
    stack_t stack1;
    stackCtor(stack1);

    for (stackElementType i = 0; i < 10; ++i) {
        stackPush(&stack1, i);
        //printf("%lld %lld\n", i, stack1.dataHash);
    }

    stackElementType value;
    for (stackElementType i = 0; i < 10; ++i) {
        stackPop(&stack1, &value);
        printf("%lld\n", value);
    }

    stackDtor(&stack1);
    return 0;
}