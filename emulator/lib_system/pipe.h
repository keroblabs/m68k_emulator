#ifndef __PIPE_DATA_H__
#define __PIPE_DATA_H__

////////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

////////////////////////////////////////////////////////////////////////////////////////

#define MAX_PIPE_SIZE           32

////////////////////////////////////////////////////////////////////////////////////////

typedef void (*trig_pipe_t)(void);

typedef struct
{
    uint8_t pointer_in;
    uint8_t pointer_out;

    uint8_t max_size;
    uint8_t count;
    
    uint8_t * pipe_list[MAX_PIPE_SIZE];
    uint16_t  pipe_size[MAX_PIPE_SIZE];
    
} pipe_data_t;

////////////////////////////////////////////////////////////////////////////////////////

void        pipe_init(pipe_data_t * pipe_data, uint16_t size);

uint8_t   * pipe_read(pipe_data_t * pipe, uint16_t * size);
uint8_t   * pipe_get_at(pipe_data_t * pipe, uint16_t index, uint16_t * size);
void        pipe_write(pipe_data_t * pipe, uint8_t * buffer, uint16_t size);

bool_t      pipe_is_full(pipe_data_t * pipe);
bool_t      pipe_is_empty(pipe_data_t * pipe);

uint8_t     pipe_get_count(pipe_data_t * pipe);

////////////////////////////////////////////////////////////////////////////////////////

#endif // __PIPE_DATA_H__

