#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdio>
#include "stack.hpp"

void stackDtor(stack_t *stack) {
    ASSERT_STACK_IS_VERIFIED(stack);

    if (stack->leftCanary != nullptr) {
        if (DEBUG_LEVEL == DebugLevels::EXHAUSTIVE) {
            *stack->leftCanary  = (stackElementType) DELETED_FROM_MEMORY;
            *stack->rightCanary = (stackElementType) DELETED_FROM_MEMORY;
            for (size_t i = 0; i < stack->capacity; ++i) {
                stack->data[i] = DELETED_FROM_MEMORY;
            }
        }

        free(stack->leftCanary);

        stack->capacity = stack->size = stack->hash = 0;
        stack->leftCanary = stack->data = stack->rightCanary = nullptr;
    }
}

ErrorCodes validateStack(stack_t *stack) {
    if (DEBUG_LEVEL == DebugLevels::DISABLE) {
        return ErrorCodes::OKAY;
    }

    if (stack->data     == nullptr &&
        stack->size     == 0       &&
        stack->capacity == 0       &&
        stack->hash     == 0) {
        return ErrorCodes::OKAY;
    }

    if (!strlen(stack->name))                                 return ErrorCodes::CONSTRUCTOR_WASNT_CALLED;
    if (!stack->data)                                         return ErrorCodes::DATA_NULLPTR;
    if (!stack->leftCanary)                                   return ErrorCodes::LCANARY_NULLPTR;
    if (!stack->rightCanary)                                  return ErrorCodes::RCANARY_NULLPTR;
    if (stack->capacity     <  0)                             return ErrorCodes::NEGATIVE_CAPACITY;
    if (stack->size         <  0)                             return ErrorCodes::NEGATIVE_SIZE;
    if (stack->size         >  stack->capacity)               return ErrorCodes::SIZE_BIGGER_THAN_CAPACITY;
    if (stack->leftCanary   != stack->data - 1)               return ErrorCodes::LCANARY_WRONG_PTR;
    if (stack->rightCanary  != stack->data + stack->capacity) return ErrorCodes::RCANARY_WRONG_PTR;
    if (*stack->leftCanary  != Poison::CANARY)                return ErrorCodes::LCANARY_WRONG_VALUE;
    if (*stack->rightCanary != Poison::CANARY)                return ErrorCodes::RCANARY_WRONG_VALUE;

    if (DEBUG_LEVEL == DebugLevels::EXHAUSTIVE) {
        for (size_t i = stack->size; i < stack->capacity; ++i) {
            if (stack->data[i] != POPPED) {
                return ErrorCodes::FREE_SPACE_ISNT_POISONED;
            }
        }

        if (calcStackHash(stack) != stack->hash) {
            return ErrorCodes::WRONG_HASH;
        }
    }


    return ErrorCodes::OKAY;
}

ErrorCodes setDataPointers(stack_t *stack, stackElementType *leftCanary, size_t capacity) {
    //если стеком попользоваться, а потом изменить размер на 0, то он будет = 2 из-за канареек, норм?
    stack->leftCanary = leftCanary;
    stack->data = leftCanary + 1;
    stack->rightCanary = stack->data + capacity;

    *stack->leftCanary  = Poison::CANARY;
    *stack->rightCanary = Poison::CANARY;
    stack->capacity = capacity;

    if (DEBUG_LEVEL == DebugLevels::EXHAUSTIVE) {
        // заменить на мемсет?
        for (size_t i = stack->size; i < stack->capacity; ++i) {
            stack->data[i] = POPPED;
        }

        stack->hash = calcStackHash(stack);
    }

    ASSERT_STACK_IS_VERIFIED(stack);
    return ErrorCodes::OKAY;
}

ErrorCodes stackChangeCapacity(stack_t *stack, size_t capacity) {
    ErrorCodes inputStackStatus = validateStack(stack);
    stackElementType *newPtr = nullptr;

    if (inputStackStatus == ErrorCodes::OKAY ||
        stack->data == nullptr) {
        newPtr = (stackElementType *) realloc(stack->leftCanary,(capacity + 2) * sizeof(stackElementType));
    }
    else {
        ASSERT_STACK_IS_VERIFIED(stack);
    }

    if (newPtr != nullptr) {
        setDataPointers(stack, newPtr, capacity);
        ASSERT_STACK_IS_VERIFIED(stack);
        return ErrorCodes::OKAY;
    }
    return ErrorCodes::ALLOCATION_ERROR;
}

ErrorCodes stackPush(stack_t *stack, stackElementType value) {
    ASSERT_STACK_IS_VERIFIED(stack);

    if (stack->size == stack->capacity) {
        stackChangeCapacity(stack, (size_t) ceil((double) stack->capacity * EXPAND_COEF) + 1);
    }
    stack->data[stack->size++] = value;

    if (DEBUG_LEVEL == DebugLevels::EXHAUSTIVE) {
        stack->hash = calcStackHash(stack);
    }

    ASSERT_STACK_IS_VERIFIED(stack);
    return ErrorCodes::OKAY;
}

ErrorCodes stackPop(stack_t *stack, stackElementType *poppedValue) {
    ASSERT_STACK_IS_VERIFIED(stack);

    if (stack->size == 0) {
        return ErrorCodes::POPPING_FROM_EMPTY_STACK;
    }

    *poppedValue = stack->data[--stack->size];
    if (DEBUG_LEVEL >= DebugLevels::FAST) {
        stack->data[stack->size] = Poison::POPPED;

        if (DEBUG_LEVEL == DebugLevels::EXHAUSTIVE) {
            stack->hash = calcStackHash(stack);
        }
    }

    if (stack->size <= (size_t) ((double) stack->capacity * CHECK_SHRINK_COEF)) {
        stackChangeCapacity(stack, (size_t) ((double) stack->capacity * SHRINK_COEF));
    }

    ASSERT_STACK_IS_VERIFIED(stack);
    return ErrorCodes::OKAY;
}

long long calcHash(const char *dataPointer, size_t nBytes) {
    long long hash = *dataPointer;
    for (size_t i = 0; i < nBytes; ++i) {
        // тут ub почему-то
        hash ^= dataPointer[i] << (i % 64);
    }

    return hash;
}

long long calcStackHash(stack_t *stack) {
    // корректно если размер канарейки совпадает с типом стека
    return calcHash((char *)stack->leftCanary,
                    sizeof(stackElementType) * (stack->capacity + 2));
}

void stackDump(stack_t *stack, ErrorCodes validationStatus, FILE *out) {
    fprintf(out, "Stack \"%s\" from %p\n", stack->name, stack);
    fprintf(out, "Status = %s\n", getErrorCodeName(validationStatus));
    fprintf(out, "Capacity = %zu, size = %zu\n", stack->capacity, stack->size);
    // как вывести тип stackElementType?

    if (*stack->leftCanary == Poison::CANARY) {
        fprintf(out, "[-1] CANARY\n");
    }
    else {
        fprintf(out, "[-1] %lld\n", *stack->leftCanary);
    }

    for (size_t i = 0; i < stack->capacity; ++i) {
        if (stack->data[i] == POPPED) {
            fprintf(out, "[%zu] POPPED\n", i);
        }
        else {
            fprintf(out, "[%zu] %lld\n", i, stack->data[i]);
        }
    }

    if (*stack->rightCanary == Poison::CANARY) {
        fprintf(out, "[%zu] CANARY\n", stack->capacity);
    }
    else {
        fprintf(out, "[%zu] %lld\n", stack->capacity, *stack->rightCanary);
    }
}

#define DECLARE_CODE_(code) #code,
const char* getErrorCodeName(ErrorCodes errorValue) {
    unsigned long value = (unsigned long) errorValue;
    const char* value2Name[] = {
        #include "ErrorCodes.h"
    };
    size_t arrSize = sizeof(value2Name) / sizeof(value2Name[0]);

    if (value >= 0 && value < arrSize) {
        return value2Name[value];
    }
    else {
        return "UNKNOWN";
    }
}
#undef DECLARE_CODE_