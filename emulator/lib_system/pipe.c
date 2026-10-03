#include "pipe.h"

////////////////////////////////////////////////////////////////////////////////////////

#define PIPE_MALLOC     malloc
#define PIPE_FREE       free

////////////////////////////////////////////////////////////////////////////////////////

void pipe_init(pipe_data_t * pipe_data, uint16_t size)
{
    // erase memory
    memset(pipe_data, 0x00, sizeof(pipe_data_t));

	if (size >= MAX_PIPE_SIZE) size = MAX_PIPE_SIZE - 1;

    pipe_data->max_size = (uint8_t)size;
	pipe_data->count = 0;
}

uint8_t * pipe_get_at(pipe_data_t * pipe, uint16_t index, uint16_t * size)
{
    uint8_t * ret_value = NULL;
    uint16_t pipe_index = pipe->pointer_out + index;

    if (pipe_index >= pipe->max_size)
    {
        pipe_index -= pipe->max_size;
    }

    if (pipe_index >= pipe->pointer_in)
    {
        return NULL;
    }

    // get byte from buffer
	ret_value = pipe->pipe_list[pipe_index];
    
    if (size != NULL)
    {
        *size = pipe->pipe_size[pipe_index];    
    }

    return ret_value;
}

uint8_t * pipe_read(pipe_data_t * pipe, uint16_t * size)
{
	uint8_t * ret_value = NULL;

    if (pipe == NULL) return NULL;
    
    // buffer is empty
	if (pipe->pointer_in == pipe->pointer_out)
    {
        return NULL;
    }

    // get byte from buffer
	ret_value = pipe->pipe_list[pipe->pointer_out];
    
    if (size != NULL)
        *size =     pipe->pipe_size[pipe->pointer_out];

    // move out-pointer
	pipe->pointer_out++;
    pipe->count--;
	if (pipe->pointer_out == pipe->max_size) pipe->pointer_out = 0;
    
	return ret_value;
}

void pipe_write(pipe_data_t * pipe, uint8_t * buffer, uint16_t size)
{
    if (pipe == NULL) return;

    uint8_t * new_buffer = (uint8_t *)PIPE_MALLOC(size);
    if (new_buffer == NULL) return;
    memcpy(new_buffer, buffer, size);
    
    // insert data into the buffer
	pipe->pipe_list[pipe->pointer_in] = new_buffer;
	pipe->pipe_size[pipe->pointer_in] = size;
	pipe->pointer_in++;
    pipe->count++;
    
    // update pointers and control overflow
	if (pipe->pointer_in == pipe->max_size) pipe->pointer_in = 0;

    // overflow, delete buffer
	if (pipe->pointer_in == pipe->pointer_out)
    {
        PIPE_FREE(pipe->pipe_list[pipe->pointer_out]);
        pipe->pointer_out++;
        pipe->count--;
	}
    
	if (pipe->pointer_out == pipe->max_size) pipe->pointer_out = 0;
}

bool_t pipe_is_full(pipe_data_t * pipe)
{
    bool_t ret = FALSE;
    
    if (pipe == NULL) return TRUE;
    
    // input is just before output
	if (pipe->pointer_in == (pipe->pointer_out - 1)) ret = TRUE;

    // input is in the tail, and output is in the head
	else if ((pipe->pointer_in == (pipe->max_size - 1)) &&
             (pipe->pointer_out == 0)) ret = TRUE;
    
    // not full
    return ret;
}

bool_t pipe_is_empty(pipe_data_t * pipe)
{
    bool_t ret = FALSE;
    
    if (pipe == NULL) return TRUE;
	if (pipe->pointer_in == pipe->pointer_out) ret = TRUE;
    
    return ret;
}

uint8_t pipe_get_count(pipe_data_t * pipe)
{
    if (pipe == NULL) return TRUE;
    return pipe->count;
}
