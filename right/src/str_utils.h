#ifndef SRC_STR_UTILS_H_
#define SRC_STR_UTILS_H_

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Functions:

    int32_t ParseInt32(const char *a, const char *aEnd);
    bool StrLessOrEqual(const char* a, const char* aEnd, const char* b, const char* bEnd);
    bool StrEqual(const char* a, const char* aEnd, const char* b, const char* bEnd);
    const char* FindChar(char c, const char* str, const char* strEnd);
    bool TokenMatches(const char *a, const char *aEnd, const char *b);
    bool TokenMatches2(const char *a, const char *aEnd, const char *b, const char *bEnd);
    uint8_t TokLen(const char *a, const char *aEnd);
    const char* NextTok(const char* cmd, const char *cmdEnd);
    const char* TokEnd(const char* cmd, const char *cmdEnd);

#endif /* SRC_STR_UTILS_H_ */
