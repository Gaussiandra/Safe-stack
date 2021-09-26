#define ASSERT_STACK_IS_VERIFIED(stack, stackName) {                    \
    if (validateStack(stack) != ErrorCodes::OKAY) {                     \
        printf("##stackName##<>")                                                                    \
    }\
                                                                        \
}                                                            \

#if (!defined(POISON_FIRST_VALUE))
    #define POISON_FIRST_VALUE -987654103
#endif

// можно ли в enum class сделать?
enum Poison {
    POPPED = POISON_FIRST_VALUE,
    DELETED_FROM_MEMORY,
    CANARY = 0xDEADBEEF, // а если уменьшить тип?
};

enum class ErrorCodes {
    OKAY,
    DATA_NULLPTR,
    NEGATIVE_CAPACITY,
    NEGATIVE_SIZE,
    SIZE_BIGGER_THAN_CAPACITY,
    LCANARY_NULLPTR,
    RCANARY_NULLPTR,
    LCANARY_WRONG_PTR,
    RCANARY_WRONG_PTR,
    LCANARY_WRONG_VALUE,
    RCANARY_WRONG_VALUE,

    FREE_SPACE_POISONED,
    ALLOCATION_ERROR,
    POPPING_FROM_EMPTY_STACK,
};

enum class DebugLevels {
    DISABLE,
    FAST,
    EXHAUSTIVE,
};

const DebugLevels DEBUG_LEVEL = DebugLevels::EXHAUSTIVE;
const double EXPAND_COEF = 1.5;
const double CHECK_SHRINK_COEF = 0.5;
const double SHRINK_COEF = 0.65;

typedef long long stackElementType;
struct stack_t {
    size_t size     = 0,
           capacity = 0;
    stackElementType *data        = nullptr,
                     *leftCanary  = nullptr,
                     *rightCanary = nullptr;
};

ErrorCodes validateStack(stack_t *stack);
ErrorCodes setDataPointers(stack_t *stack, stackElementType *leftCanary, size_t capacity);
ErrorCodes stackChangeCapacity(stack_t *stack, size_t capacity);
ErrorCodes stackPush(stack_t *stack, stackElementType value);
ErrorCodes stackPop(stack_t *stack, stackElementType *poppedValue);
void stackDtor(stack_t *stack);
