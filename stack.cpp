#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "utils.hpp"
#include "stack.hpp"

void stackCtor_(stack_t *stack) {
    assert(stack);

    stack->capacity = stack->size = 0;
    stack->data = nullptr;

    ON_DEBUG(stack->leftStructCanary = stack->rightStructCanary = Poison::CANARY);
    ON_DEBUG(stack->leftCanary = stack->rightCanary = nullptr);

    #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_EXPENSIVE
        stack->structHash = calcHash((char *) stack, STRUCT_HASHABLE_SIZE);
        stack->dataHash = 0;
    #endif

    ASSERT_STACK_IS_VERIFIED(stack);
}

void stackDtor(stack_t *stack) {
    ASSERT_STACK_IS_VERIFIED(stack);

    #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_DISABLE
        if (stack->data != nullptr) {
            free(stack->data);
        }
    #else
        if (stack->leftCanary != nullptr) {
            #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_EXPENSIVE
                *stack->leftCanary  = (stackCanaryType) DELETED_FROM_MEMORY;
                *stack->rightCanary = (stackCanaryType) DELETED_FROM_MEMORY;
                for (size_t i = 0; i < stack->capacity; ++i) {
                    stack->data[i] = DELETED_FROM_MEMORY;
                }
            #endif

            free(stack->leftCanary);
            memset(stack, 0, sizeof(*stack));
        }
    #endif
}

ErrorCodes validateStack(stack_t *stack) {
    #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_DISABLE
        return ErrorCodes::OKAY;
    #else
        if (!stack) {
            return ErrorCodes::WRONG_STACK_PTR;
        }

        if (stack->data       == nullptr &&
            stack->size       == 0       &&
            stack->capacity   == 0) {
            #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_EXPENSIVE
                if (stack->dataHash   == 0) {
                    return ErrorCodes::OKAY;
                }
            #else
                return ErrorCodes::OKAY;
            #endif
        }

        if (!stack->name)                                return ErrorCodes::CONSTRUCTOR_WASNT_CALLED;
        if (!stack->data)                                return ErrorCodes::DATA_NULLPTR;
        if (!stack->leftCanary)                          return ErrorCodes::LCANARY_DATA_NULLPTR;
        if (!stack->rightCanary)                         return ErrorCodes::RCANARY_DATA_NULLPTR;
        if (stack->capacity          <  0)               return ErrorCodes::NEGATIVE_CAPACITY;
        if (stack->size              <  0)               return ErrorCodes::NEGATIVE_SIZE;
        if (stack->size              >  stack->capacity) return ErrorCodes::SIZE_BIGGER_THAN_CAPACITY;
        if (stack->rightStructCanary != Poison::CANARY)  return ErrorCodes::RCANARY_STRUCT_WRONG_VALUE;
        if (stack->leftStructCanary  != Poison::CANARY)  return ErrorCodes::LCANARY_STRUCT_WRONG_VALUE;
        if (*stack->leftCanary       != Poison::CANARY)  return ErrorCodes::LCANARY_DATA_WRONG_VALUE;
        if (*stack->rightCanary      != Poison::CANARY)  return ErrorCodes::RCANARY_DATA_WRONG_VALUE;
        if (stack->leftCanary        != (stackCanaryType *) ((char *) stack->data - sizeof(stackCanaryType)))
            return ErrorCodes::LCANARY_WRONG_PTR;
        if (stack->rightCanary       != (stackCanaryType *) (stack->data + stack->capacity))
            return ErrorCodes::RCANARY_WRONG_PTR;

        #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_EXPENSIVE
            for (size_t i = stack->size; i < stack->capacity; ++i) {
                if (stack->data[i] != POPPED) {
                    return ErrorCodes::FREE_SPACE_ISNT_POISONED;
                }
            }

            if (calcHash((char *) stack, STRUCT_HASHABLE_SIZE) != stack->structHash) {
                return ErrorCodes::WRONG_STRUCT_HASH;
            }
            if (calcHash((char *) stack->leftCanary, getStackArraySize(stack->capacity)) != stack->dataHash) {
                return ErrorCodes::WRONG_DATA_HASH;
            }
        #endif
        return ErrorCodes::OKAY;
    #endif
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
        newCapacity = (size_t) fmax((double) stack->capacity * SHRINK_COEF, 1);
    }

    if (inputStackStatus == ErrorCodes::OKAY ||
        stack->data == nullptr) {
    #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_DISABLE
        newPtr = (stackCanaryType *) recalloc(stack->data, getStackArraySize(newCapacity), 1);
    #else
        newPtr = (stackCanaryType *) recalloc(stack->leftCanary, getStackArraySize(newCapacity), 1);
    #endif
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

#if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_EXPENSIVE
    #define SET_HASH_VALUES(stack) {                                                                                   \
        stack->structHash = calcHash((char *) stack, STRUCT_HASHABLE_SIZE);                                            \
        stack->dataHash = calcHash((char *) stack->leftCanary, getStackArraySize(stack->capacity));                    \
    }
#else
    #define SET_HASH_VALUES(stack)
#endif

ErrorCodes setDataPointers(stack_t *stack, stackCanaryType *array, size_t capacity) {
    assert(stack);

#if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_DISABLE
    stack->data = (stackElementType *) array;
#else
    stack->leftCanary = array;
        stack->data = (stackElementType *) (array + 1);
        stack->rightCanary = (stackCanaryType *) (stack->data + capacity);

        *stack->leftCanary  = Poison::CANARY;
        *stack->rightCanary = Poison::CANARY;
#endif
    stack->capacity = capacity;

#if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_EXPENSIVE
    for (size_t i = stack->size; i < stack->capacity; ++i) {
            stack->data[i] = POPPED;
        }

    SET_HASH_VALUES(stack);
#endif

    ASSERT_STACK_IS_VERIFIED(stack);
    return ErrorCodes::OKAY;
}

ErrorCodes stackPush(stack_t *stack, stackElementType value) {
    ASSERT_STACK_IS_VERIFIED(stack);

    if (stack->size == stack->capacity) {
        stackChangeCapacity(stack, stack->capacity, true);
    }
    stack->data[stack->size++] = value;

    SET_HASH_VALUES(stack);

    ASSERT_STACK_IS_VERIFIED(stack);
    return ErrorCodes::OKAY;
}

ErrorCodes stackPop(stack_t *stack, stackElementType *poppedValue) {
    ASSERT_STACK_IS_VERIFIED(stack);

    if (stack->size == 0) {
        return ErrorCodes::POPPING_FROM_EMPTY_STACK;
    }

    *poppedValue = stack->data[--stack->size];
    #if CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_FAST
        stack->data[stack->size] = Poison::POPPED;

        SET_HASH_VALUES(stack);
    #endif

    stackChangeCapacity(stack, stack->capacity, false);

    ASSERT_STACK_IS_VERIFIED(stack);
    return ErrorCodes::OKAY;
}

size_t getStackArraySize(size_t capacity) {
    return capacity * sizeof(stackElementType) ON_DEBUG(+ 2*sizeof(stackCanaryType));
}

void stackDump(stack_t *stack, ErrorCodes validationStatus, FILE *out) {
    assert(stack);

    ON_DEBUG(fprintf(out, "Stack \"%s\" from %p\n", stack->name, stack));
    ON_DEBUG(fprintf(out, "Left data canary    = %lld on %p\n", *stack->leftCanary, stack->leftCanary));
    ON_DEBUG(fprintf(out, "Right data canary   = %lld on %p\n", *stack->rightCanary, stack->rightCanary));
    ON_DEBUG(fprintf(out, "Left struct canary  = %lld on %p\n", stack->leftStructCanary, &stack->leftStructCanary));
    ON_DEBUG(fprintf(out, "Right struct canary = %lld on %p\n", stack->rightStructCanary, &stack->rightStructCanary));
    #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_EXPENSIVE
        fprintf(out, "Data hash   = %lld\n", stack->dataHash);
        fprintf(out, "Struct hash = %lld\n", stack->structHash);
    #endif

    fprintf(out, "Data from %p\n", stack->data);
    fprintf(out, "Status = %s\n", getErrorCodeName(validationStatus));
    fprintf(out, "Capacity = %zu, size = %zu\n", stack->capacity, stack->size);

    #if CURRENT_DEBUG_LEVEL != DEBUG_LEVEL_DISABLE
        if (*stack->leftCanary == Poison::CANARY) {
            fprintf(out, "[-1] CANARY\n");
        }
        else {
            fprintf(out, "[-1] %lld\n", *stack->leftCanary);
        }
    #endif

    for (size_t i = 0; i < stack->capacity; ++i) {
        if (stack->data[i] == POPPED) {
            fprintf(out, "[%zu] POPPED\n", i);
        }
        else {
            fprintf(out, "[%zu] %lld\n", i, stack->data[i]);
        }
    }

    #if CURRENT_DEBUG_LEVEL != DEBUG_LEVEL_DISABLE
        if (*stack->rightCanary == Poison::CANARY) {
            fprintf(out, "[%zu] CANARY\n", stack->capacity);
        }
        else {
            fprintf(out, "[%zu] %lld\n", stack->capacity, *stack->rightCanary);
        }
    #endif
}

#define DECLARE_CODE_(code) #code,
const char* getErrorCodeName(ErrorCodes errorValue) {
    unsigned long value = (unsigned long) errorValue;
    const char* value2Name[] = {
        #include "ErrorCodes.hpp"
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
