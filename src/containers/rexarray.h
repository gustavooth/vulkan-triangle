#pragma once

#include "defines.h"

/** 
 * Memory layout
 * u64 capacity = number elements that can be held
 * u64 length = number of elements currently contained
 * u64 stride = size of each element in bytes
 * data = array of elements
 */
typedef void* rexarray;

enum {
    REXARRAY_CAPACITY,
    REXARRAY_LENGTH,
    REXARRAY_STRIDE,

    REXARRAY_DATA,
    REXARRAY_FIELD_LENGTH = 3,
};

rexarray _rexarray_create(u32 capacity, u32 stride);
void rexarray_destroy(rexarray arr);

u32 _rexarray_field_get(rexarray arr, u32 field);
void _rexarray_field_set(rexarray arr, u32 field, u32 value);
rexarray _rexarray_extend(rexarray arr, u32 new_capacity);

rexarray _rexarray_push(rexarray arr, void* value_ptr);

/**
 * Remove the last item from the array.
 * @param rexarray Dynamic array.
 */
void rexarray_pop(rexarray arr);

/**
 * Remove the item at the chosen index from the array.
 * @param rexarray Dynamic array.
 * @param index Item index.
 */
void rexarray_pop_at(rexarray arr, u32 index);

#define REXARRAY_DEFAULT_EXTEND 3

/**
 * @brief Create a dynamic array
 * @param type Variable type.
 * @returns Dynamic array (rexarray).
 * @note Returns a direct pointer to the data.
 */
#define REXARRAY(type) _rexarray_create(REXARRAY_DEFAULT_EXTEND, sizeof(type))

/**
 * Create a dynamic array
 * @param type Variable type.
 * @returns Dynamic array (rexarray).
 * @note Returns a direct pointer to the data.
 */
#define rexarray_create(type) _rexarray_create(REXARRAY_DEFAULT_EXTEND, sizeof(type))

/**
 * Create a dynamic array with the chosen capacity.
 * @param type Variable type.
 * @param size Array Capacity.
 * @returns Dynamic array (rexarray).
 * @note Returns a direct pointer to the data.
 */
#define rexarray_reserve(type, size) _rexarray_create(size, sizeof(type))

/**
 * Adds a copy of the value to the end of the array.
 * @param rexarray Dynamic array.
 * @param value_ptr Value pointer.
 */
#define rexarray_push(rexarray, value_ptr) {        \
    rexarray = _rexarray_push(rexarray, value_ptr); \
}

/**
 * This function returns the number of items in the array.
 * @param rexarray Dynamic array.
 * @returns Number of items in the array.
 */
#define rexarray_len(rexarray) _rexarray_field_get(rexarray, REXARRAY_LENGTH)

/**
 * This function returns the size of each item in the array.
 * @param rexarray Dynamic array.
 * @returns Size of each item in the array.
 */
#define rexarray_stride(rexarray) _rexarray_field_get(rexarray, REXARRAY_STRIDE)

/**
 * This function returns the capacity of the array.
 * @param rexarray Dynamic array.
 * @returns Array Capacity.
 */
#define rexarray_capacity(rexarray) _rexarray_field_get(rexarray, REXARRAY_CAPACITY)