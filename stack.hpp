#define OUTPUT_STREAM stderr

#define ASSERT_STACK_IS_VERIFIED(stack) {                                                                              \
    ErrorCodes validationStatus = validateStack(stack);                                                                \
    if (validationStatus != ErrorCodes::OKAY) {                                                                        \
        fprintf(OUTPUT_STREAM, "FILE = %s\n", __FILE__);                                                               \
        fprintf(OUTPUT_STREAM, "FUNC = %s\n", __FUNCTION__);                                                           \
        fprintf(OUTPUT_STREAM, "Line = %d\n", __LINE__);                                                               \
        stackDump(stack, validationStatus);                                                                            \
        assert(false);                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
}                                                                                                                      \

#if (!defined(POISON_FIRST_VALUE_))
    #define POISON_FIRST_VALUE_ -987654103
#endif

// можно ли в enum class сделать?
enum Poison {
    POPPED = POISON_FIRST_VALUE_,
    DELETED_FROM_MEMORY,
    CANARY = 0xDEADBEEF, // а если уменьшить тип?
};
#undef POISON_FIRST_VALUE_

#define DECLARE_CODE_(code) code,
enum class ErrorCodes {
    #include "ErrorCodes.h"
};
#undef DECLARE_CODE_

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
    const char *name = "";
    long long hash = 0;
    #define stackCtor(stackName) stackName.name = #stackName;
};

void stackDtor(stack_t *stack);
ErrorCodes validateStack(stack_t *stack);
ErrorCodes setDataPointers(stack_t *stack, stackElementType *leftCanary, size_t capacity);
ErrorCodes stackChangeCapacity(stack_t *stack, size_t capacity);
ErrorCodes stackPush(stack_t *stack, stackElementType value);
ErrorCodes stackPop(stack_t *stack, stackElementType *poppedValue);
long long calcHash(const char *dataPointer, size_t nBytes);
long long calcStackHash(stack_t *stack);
void stackDump(stack_t *stack, ErrorCodes validationStatus, FILE *out = OUTPUT_STREAM);
const char* getErrorCodeName(ErrorCodes errorValue);