#pragma once

#include <inc/lib.h> // printf, exit


#define CHECK(condition, ...)                                                       \
    do {                                                                            \
        if (!(condition)) {                                                         \
            printf("failed check '%s' at %s:%d\n", #condition, __FILE__, __LINE__); \
            printf(__VA_ARGS__);                                                    \
            exit();                                                                 \
        }                                                                           \
    } while (0)
