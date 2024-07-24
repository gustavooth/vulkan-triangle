#include "rexarray.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

rexarray _rexarray_create(u32 capacity, u32 stride) {
    u32 total_size = (REXARRAY_FIELD_LENGTH * sizeof(u32)) + (capacity * stride);
    u32* header = malloc(total_size);
    header[REXARRAY_CAPACITY] = capacity;
    header[REXARRAY_LENGTH] = 0;
    header[REXARRAY_STRIDE] = stride;
    return header + REXARRAY_DATA;
}

void rexarray_destroy(rexarray arr) {
    u32* header = (u32*)arr - REXARRAY_FIELD_LENGTH;
    u32 total_size = (REXARRAY_FIELD_LENGTH * sizeof(u32)) + (header[REXARRAY_CAPACITY] * header[REXARRAY_STRIDE]);
    free(header);
}

u32 _rexarray_field_get(rexarray arr, u32 field) {
    u32* header = (u32*)arr - REXARRAY_FIELD_LENGTH;
    return header[field];
}

void _rexarray_field_set(rexarray arr, u32 field, u32 value) {
    u32* header = (u32*)arr - REXARRAY_FIELD_LENGTH;
    header[field] = value;
}

rexarray _rexarray_extend(rexarray arr, u32 new_capacity) {
    u32* header = (u32*)arr - REXARRAY_FIELD_LENGTH;

    rexarray new_arr = _rexarray_create(new_capacity, header[REXARRAY_STRIDE]);
    u32* new_header = (u32*)new_arr - REXARRAY_FIELD_LENGTH;

    memcpy(new_arr, arr, header[REXARRAY_LENGTH] * header[REXARRAY_STRIDE]);
    new_header[REXARRAY_LENGTH] = header[REXARRAY_LENGTH];

    rexarray_destroy(arr);

    return new_arr;
}

rexarray _rexarray_push(rexarray arr, void* value_ptr) {
    u32* header = (u32*)arr - REXARRAY_FIELD_LENGTH;

    if(header[REXARRAY_CAPACITY] == header[REXARRAY_LENGTH]) {
        arr = _rexarray_extend(arr, header[REXARRAY_CAPACITY] + REXARRAY_DEFAULT_EXTEND);
        header = (u32*)arr - REXARRAY_FIELD_LENGTH;
    }

    memcpy((void*)((u64)arr + header[REXARRAY_STRIDE] * header[REXARRAY_LENGTH]), value_ptr, header[REXARRAY_STRIDE]);
    header[REXARRAY_LENGTH] += 1;

    return arr;
}

void rexarray_pop(rexarray arr) {
    u32* header = (u32*)arr - REXARRAY_FIELD_LENGTH;
    memset((void*)((u64)arr + header[REXARRAY_STRIDE] * (header[REXARRAY_LENGTH] - 1)), 0, header[REXARRAY_STRIDE]);
    header[REXARRAY_LENGTH] -= 1;
}

void rexarray_pop_at(rexarray arr, u32 index) {
    u32* header = (u32*)arr - REXARRAY_FIELD_LENGTH;

    u64 addr = (u64)arr + (header[REXARRAY_STRIDE] * (index + 1));
    u32 size = (header[REXARRAY_LENGTH] - (index + 1)) * header[REXARRAY_STRIDE];
    
    void* buffer = malloc(size);
    memcpy(buffer, (void*)addr, size);
    memcpy((void*)(addr - header[REXARRAY_STRIDE]), buffer, size);
    free(buffer);

    header[REXARRAY_LENGTH] -= 1;
}