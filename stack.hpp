#include <cstdint>
#include "utils.hpp"

#define DEBUG_LEVEL_DISABLE   0
#define DEBUG_LEVEL_FAST      1
#define DEBUG_LEVEL_EXPENSIVE 2
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_EXPENSIVE

#if (CURRENT_DEBUG_LEVEL != DEBUG_LEVEL_DISABLE)
#define ON_DEBUG(code) code
#else
#define ON_DEBUG(code)
#endif

#define LOCATION __FILE__, __FUNCTION__, __LINE__
#define ASSERT_STACK_IS_VERIFIED(stack) {                                                                              \
    ErrorCodes validationStatus = validateStack(stack);                                                                \
    if (validationStatus != ErrorCodes::OKAY) {                                                                        \
        fprintf(OUTPUT_STREAM, "FILE = %s\nFUNC = %s\nLINE = %d\n", LOCATION);                                         \
        stackDump(stack, validationStatus);                                                                            \
        assert(false && "Fatal error. Dump was created");                                                              \
    }                                                                                                                  \
}

#define DECLARE_CODE_(code) code,
enum class ErrorCodes {
    #include "ErrorCodes.hpp"
};
#undef DECLARE_CODE_

// Should be set with respect to stackElementType and stackCanaryType
enum Poison {
    POPPED = 0xF2EE,
    #if CURRENT_DEBUG_LEVEL != DEBUG_LEVEL_DISABLE
        DELETED_FROM_MEMORY,
        CANARY = 0xDEADBEEF
    #endif
};

typedef int stackElementType;
typedef long long stackCanaryType;
struct stack_t {
    ON_DEBUG(stackCanaryType leftStructCanary = Poison::CANARY);

    size_t size     = 0,
           capacity = 0;
    stackElementType *data = nullptr;

    ON_DEBUG(stackCanaryType *leftCanary  = nullptr);
    ON_DEBUG(stackCanaryType *rightCanary = nullptr);
    ON_DEBUG(const char *name = nullptr);
    ON_DEBUG(stackCanaryType rightStructCanary = Poison::CANARY);

    #if CURRENT_DEBUG_LEVEL == DEBUG_LEVEL_EXPENSIVE
        hashType structHash = 0,
                 dataHash   = 0;
    #endif

    #define stackCtor(stackName) {                                                                                 \
        stackCtor_(&stackName);                                                                                    \
        ON_DEBUG(stackName.name = #stackName);                                                                     \
    }
};

const double EXPAND_COEF = 1.5;
const double SHRINK_COEF = 0.65;
const double CHECK_HYSTERESIS_COEF = 0.5;
static FILE *OUTPUT_STREAM = stderr;
ON_DEBUG(const size_t STRUCT_HASHABLE_SIZE = sizeof(stack_t) - 2*sizeof(hashType));

void stackCtor_(stack_t *stack);
void stackDtor(stack_t *stack);
ErrorCodes validateStack(stack_t *stack);
ErrorCodes setDataPointers(stack_t *stack, stackCanaryType *array, size_t capacity);
ErrorCodes stackChangeCapacity(stack_t *stack, size_t capacity, bool isExpandingMode);
ErrorCodes stackPush(stack_t *stack, stackElementType value);
ErrorCodes stackPop(stack_t *stack, stackElementType *poppedValue);
size_t getStackArraySize(size_t capacity);
void stackDump(stack_t *stack, ErrorCodes validationStatus, FILE *out = OUTPUT_STREAM);
const char* getErrorCodeName(ErrorCodes errorValue);
