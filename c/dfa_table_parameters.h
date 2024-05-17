#ifndef GEN_PARAMS_H
#define GEN_PARAMS_H

#include <inttypes.h>

typedef uint32_t state_num_t;

#define STATES_MAX_NUM (UINT32_MAX - 1)

typedef uint32_t target_num_t;

#define TARGETS_MAX_NUM (UINT32_MAX - 1)

typedef uint64_t table_size_t;

#define TABLE_MAX_SIZE UINT64_MAX

typedef uint8_t alphabet_t;

#define ALPHABET_MAX_SIZE (UINT8_MAX)

//TODO extend this structure to provide more info about regex syntax error

typedef struct{
    int err_type;
    unsigned int err_ind;
    char *err_offset;
}compiler_error;

#define ERROR_UNEXPECTED_SYMBOL     0
#define ERROR_STATES_OVERFLOW       1
#define ERROR_NO_MEMORY             2

#endif
