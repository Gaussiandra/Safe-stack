#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdio>
#include "stack.hpp"

void initStructFields(stack_t *stack) {
    assert(stack);

    stack->capacity = stack->size = stack->dataHash = stack->structHash = 0;
    stack->data = nullptr;
    stack->leftCanary = stack->rightCanary = nullptr;
}

void stackCtor_(stack_t *stack) {
    assert(stack);
    initStructFields(stack);
    ASSERT_STACK_IS_VERIFIED(stack);
}

void stackDtor(stack_t *stack) {
    ASSERT_STACK_IS_VERIFIED(stack);

    if (stack->leftCanary != nullptr) {
        if (DEBUG_LEVEL == DebugLevels::EXPENSIVE) {
            *stack->leftCanary  = (stackCanaryType) DELETED_FROM_MEMORY;
            *stack->rightCanary = (stackCanaryType) DELETED_FROM_MEMORY;
            for (size_t i = 0; i < stack->capacity; ++i) {
                stack->data[i] = DELETED_FROM_MEMORY;
            }
        }

        free(stack->leftCanary);
        initStructFields(stack);
    }
}

ErrorCodes validateStack(stack_t *stack) {
    if (DEBUG_LEVEL == DebugLevels::DISABLE) {
        return ErrorCodes::OKAY;
    }

    if (!stack) {
        return ErrorCodes::WRONG_STACK_PTR;
    }

    if (stack->data       == nullptr && // тут будет простой баг ||
        stack->size       == 0       &&
        stack->capacity   == 0       &&
        stack->dataHash   == 0       &&
        stack->structHash == 0) {
        return ErrorCodes::OKAY;
    }

    if (!stack->name)                                return ErrorCodes::CONSTRUCTOR_WASNT_CALLED;
    if (!stack->data)                                return ErrorCodes::DATA_NULLPTR;
    if (!stack->leftCanary)                          return ErrorCodes::LCANARY_DATA_NULLPTR;
    if (!stack->rightCanary)                         return ErrorCodes::RCANARY_DATA_NULLPTR;
    if (stack->leftStructCanary  != Poison::CANARY)  return ErrorCodes::LCANARY_STRUCT_WRONG_VALUE;
    if (stack->rightStructCanary != Poison::CANARY)  return ErrorCodes::RCANARY_STRUCT_WRONG_VALUE;
    if (stack->capacity          <  0)               return ErrorCodes::NEGATIVE_CAPACITY;
    if (stack->size              <  0)               return ErrorCodes::NEGATIVE_SIZE;
    if (stack->size              >  stack->capacity) return ErrorCodes::SIZE_BIGGER_THAN_CAPACITY;
    if (*stack->leftCanary       != Poison::CANARY)  return ErrorCodes::LCANARY_DATA_WRONG_VALUE;
    if (*stack->rightCanary      != Poison::CANARY)  return ErrorCodes::RCANARY_DATA_WRONG_VALUE;
    if (stack->leftCanary        != (stackCanaryType *) ((char *) stack->data - sizeof(stackCanaryType)))
        return ErrorCodes::LCANARY_WRONG_PTR;
    if (stack->rightCanary       != (stackCanaryType *) (stack->data + stack->capacity))
        return ErrorCodes::RCANARY_WRONG_PTR;

    if (DEBUG_LEVEL == DebugLevels::EXPENSIVE) {
        for (size_t i = stack->size; i < stack->capacity; ++i) {
            if (stack->data[i] != POPPED) {
                return ErrorCodes::FREE_SPACE_ISNT_POISONED;
            }
        }

        if (calcHash((char *) stack, getStructHashableSize(stack)) != stack->structHash) {
            return ErrorCodes::WRONG_STRUCT_HASH;
        }
        if (calcHash((char *) stack->leftCanary, getStackArraySize(stack->capacity)) != stack->dataHash) {
            return ErrorCodes::WRONG_DATA_HASH;
        }
    }


    return ErrorCodes::OKAY;
}

ErrorCodes setDataPointers(stack_t *stack, stackCanaryType *leftCanary, size_t capacity) {
    assert(stack);

    stack->leftCanary = leftCanary;
    stack->data = (stackElementType *) (leftCanary + 1);
    stack->rightCanary = (stackCanaryType *) (stack->data + capacity);

    *stack->leftCanary  = Poison::CANARY;
    *stack->rightCanary = Poison::CANARY;
    stack->capacity = capacity;

    if (DEBUG_LEVEL == DebugLevels::EXPENSIVE) {
        for (size_t i = stack->size; i < stack->capacity; ++i) {
            stack->data[i] = POPPED;
        }

        stack->structHash = calcHash((char *) stack,  getStructHashableSize(stack));
        stack->dataHash = calcHash((char *) stack->leftCanary, getStackArraySize(stack->capacity));
    }

    ASSERT_STACK_IS_VERIFIED(stack);
    return ErrorCodes::OKAY;
}

ErrorCodes stackChangeCapacity(stack_t *stack, size_t capacity, bool isExpandingMode) {
    assert(stack);

    ErrorCodes inputStackStatus = validateStack(stack);
    stackCanaryType *newPtr = nullptr;
    size_t newCapacity = stack->capacity;
    if (isExpandingMode) {
        newCapacity = (size_t) ceil((double) capacity * EXPAND_COEF) + 1;
    }
    else if (stack->size <= (size_t) ((double) stack->capacity * CHECK_HYSTERESIS_COEF)) {
        newCapacity = (size_t) ((double) stack->capacity * SHRINK_COEF);
    }

    if (inputStackStatus == ErrorCodes::OKAY ||
        stack->data == nullptr) {
        newPtr = (stackCanaryType *) realloc(stack->leftCanary, getStackArraySize(newCapacity));
    }
    else {
        ASSERT_STACK_IS_VERIFIED(stack);
    }

    if (newPtr != nullptr) {
        setDataPointers(stack, newPtr, newCapacity);
        ASSERT_STACK_IS_VERIFIED(stack);
        return ErrorCodes::OKAY;
    }
    return ErrorCodes::ALLOCATION_ERROR;
}

ErrorCodes stackPush(stack_t *stack, stackElementType value) {
    ASSERT_STACK_IS_VERIFIED(stack);

    if (stack->size == stack->capacity) {
        stackChangeCapacity(stack, stack->capacity, true);
    }
    stack->data[stack->size++] = value;

    if (DEBUG_LEVEL == DebugLevels::EXPENSIVE) {
        stack->structHash = calcHash((char *) stack,  getStructHashableSize(stack));
        stack->dataHash = calcHash((char *) stack->leftCanary, getStackArraySize(stack->capacity));
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

        if (DEBUG_LEVEL == DebugLevels::EXPENSIVE) {
            stack->structHash = calcHash((char *) stack, getStructHashableSize(stack));
            stack->dataHash = calcHash((char *) stack->leftCanary, getStackArraySize(stack->capacity));
        }
    }

    stackChangeCapacity(stack, stack->capacity, false);

    ASSERT_STACK_IS_VERIFIED(stack);
    return ErrorCodes::OKAY;
}

long long calcHash(const char *dataPointer, size_t nBytes) {
    assert(dataPointer);

    long long hash = *dataPointer;
    for (size_t i = 0; i < nBytes; ++i) {
        hash ^= dataPointer[i] << (i % 64);
    }

    return hash;
}

size_t getStructHashableSize(stack_t *stack) {
    assert(stack);
    return sizeof(*stack) - 2*sizeof(hashType);
}

size_t getStackArraySize(size_t capacity) {
    return capacity * sizeof(stackElementType) + 2*sizeof(stackCanaryType);
}

void stackDump(stack_t *stack, ErrorCodes validationStatus, FILE *out) {
    assert(stack);

    fprintf(out, "Stack \"%s\" from %p\n", stack->name, stack);
    fprintf(out, "Status = %s\n", getErrorCodeName(validationStatus));
    fprintf(out, "Capacity = %zu, size = %zu\n", stack->capacity, stack->size);

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
