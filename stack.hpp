#define LOCATION __FILE__, __FUNCTION__, __LINE__
#define ASSERT_STACK_IS_VERIFIED(stack) {                                                                              \
    ErrorCodes validationStatus = validateStack(stack);                                                                \
    if (validationStatus != ErrorCodes::OKAY) {                                                                        \
        fprintf(OUTPUT_STREAM, "FILE = %s\nFUNC = %s\nLINE = %d\n", LOCATION);                                         \
        stackDump(stack, validationStatus);                                                                            \
        assert(false && "Fatal error. Dump was created");                                                              \
    }                                                                                                                  \
}                                                                                                                      \

#define DECLARE_CODE_(code) code,
enum class ErrorCodes {
    #include "ErrorCodes.h"
};
#undef DECLARE_CODE_

enum class DebugLevels {
    DISABLE,
    FAST,
    EXPENSIVE,
};

enum Poison {
    POPPED = 0xF2EE,
    DELETED_FROM_MEMORY,
    CANARY = 0xDEADBEEF,
};

// стоит помнять
#define OUTPUT_STREAM stderr
const DebugLevels DEBUG_LEVEL = DebugLevels::EXPENSIVE;
const double EXPAND_COEF = 1.5;
const double SHRINK_COEF = 0.65;
const double CHECK_HYSTERESIS_COEF = 0.5;

typedef int stackElementType;
typedef long long stackCanaryType;
struct stack_t {
    const stackCanaryType leftStructCanary = Poison::CANARY;
    size_t size     = 0,
           capacity = 0;
    stackElementType *data        = nullptr;
    stackCanaryType  *leftCanary  = nullptr,
                     *rightCanary = nullptr;
    const char *name = nullptr;
    long long hash = 0;
    const stackCanaryType rightStructCanary = Poison::CANARY;
#define stackCtor(stackName) stackName.name = #stackName;
};

void stackDtor(stack_t *stack);
ErrorCodes validateStack(stack_t *stack);
ErrorCodes setDataPointers(stack_t *stack, stackCanaryType *leftCanary, size_t capacity);
ErrorCodes stackChangeCapacity(stack_t *stack, size_t capacity, bool isExpandingMode);
ErrorCodes stackPush(stack_t *stack, stackElementType value);
ErrorCodes stackPop(stack_t *stack, stackElementType *poppedValue);
long long calcHash(const char *dataPointer, size_t nBytes);
size_t getStackDataSize(size_t capacity);
void stackDump(stack_t *stack, ErrorCodes validationStatus, FILE *out = OUTPUT_STREAM);
const char* getErrorCodeName(ErrorCodes errorValue);
