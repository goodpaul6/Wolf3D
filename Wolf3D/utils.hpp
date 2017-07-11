#pragma once 

#include <stdio.h>
#include <stdlib.h>

#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

#define CRASH(...) do { \
    fprintf(stderr, "(%s:%d) ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    exit(1); \
} while(0)
