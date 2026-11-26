#include "pipe.h"
//#include "tima_libc.h"
//#include "debug.h"

//#ifdef USE_MULTITHREAD
//#include "t_threads.h"
//#endif

////////////////////////////////////////////////////////////////////////////////////////

#define PIPE_HEAP_SIZE 8192

#if 0 // def _OBJECT_HANDLER
#include "heap.h"
#define PIPE_MALLOC     heap_malloc
#define PIPE_FREE       heap_free
#else
//#define PIPE_MALLOC     MMALLOC
//#define PIPE_FREE       MFREE
#define PIPE_MALLOC     malloc
#define PIPE_FREE       free
#endif

////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////

uint8_t * pipe_alloc_buffer(uint16_t size)
{
    uint8_t * ptr;
    ptr = (uint8_t *)PIPE_MALLOC(size);
    return(ptr);
}

void pipe_release_buffer(uint8_t * buffer)
{
    PIPE_FREE(buffer);
}

void pipe_init(pipe_data_t * pipe_data, uint16_t size)
{
    // erase memory
    memset(pipe_data, 0x00, sizeof(pipe_data_t));

	if (size >= MAX_PIPE_SIZE) size = MAX_PIPE_SIZE - 1;

	pipe_data->total_data = 0;
	pipe_data->data_index = 0;

    pipe_data->max_size = (uint8_t)size;
}

uint16_t pipe_get_buffer_size(pipe_data_t * pipe)
{
	uint16_t size = 0;
	
    if (pipe == NULL) return(0);
    
    DI();

    // buffer is empty
	if (pipe->pointer_in == pipe->pointer_out) size = 0;
	else size = pipe->pipe_size[pipe->pointer_out];

    EI();

    return size;
}

uint32_t pipe_get_total_size(pipe_data_t * pipe)
{
    return pipe->total_data;
}

uint16_t pipe_read_buffer(pipe_data_t * pipe, uint8_t * buffer, uint16_t max_size)
{
	// uint16_t element_index;
	uint16_t copy_index;
	uint16_t copy_size;
	uint16_t size_cnt;
	uint16_t size = 0;
    uint8_t * buf_tmp;
	
    if (pipe == NULL) return(0);

    DI();

    // buffer is empty
	if (pipe->pointer_in == pipe->pointer_out)
    {
        EI();
        return(0);
    }

	if (max_size > pipe->total_data)
	{
		max_size = pipe->total_data;
	}

	size_cnt = max_size;
	copy_index = 0;

	while (size_cnt)
	{
		// get the pipe element
		size = pipe->pipe_size[pipe->pointer_out];
	    buf_tmp = pipe->pipe_list[pipe->pointer_out];

	    size -= pipe->data_index;

	    // check how much we need to copy
	    copy_size = size_cnt;
	    if (copy_size > size) copy_size = size;

	    // copy all we can
	    memcpy(&buffer[copy_index], &buf_tmp[pipe->data_index], copy_size);

	    // move indexes
	    copy_index += copy_size;
	    size_cnt -= copy_size;
	    pipe->total_data -= copy_size;

	    // if we copied the whole element
	    if (copy_size == size)
	    {
	    	// release what was read
	        pipe_release_buffer(buf_tmp);

	        // move out-pointer
	    	pipe->pointer_out++;
	    	if (pipe->pointer_out == pipe->max_size) pipe->pointer_out = 0;
	    	pipe->data_index = 0;
	    }
	    else
	    {
	    	// we still have data to copy
	    	pipe->data_index += copy_size;
	    }
	}

    EI();
    
	return(copy_index);
}

void pipe_clear(pipe_data_t * pipe)
{
    while (pipe->pointer_in != pipe->pointer_out)
    {
        pipe_release_buffer(pipe->pipe_list[pipe->pointer_out]);

        pipe->total_data -= pipe->pipe_size[pipe->pointer_out];
        pipe->data_index = 0;

        // move out-pointer
        pipe->pointer_out++;
        if (pipe->pointer_out == pipe->max_size) pipe->pointer_out = 0;
    }
}

uint8_t * pipe_read(pipe_data_t * pipe, uint16_t * size)
{
	uint8_t * ret_value = NULL;

    if (pipe == NULL) return(NULL);

    DI();
    
    // buffer is empty
	if (pipe->pointer_in == pipe->pointer_out)
    {
        EI();
        return(NULL);
    }

    // get byte from buffer
	ret_value = pipe->pipe_list[pipe->pointer_out];
    
    if (size != NULL)
    {
        *size =     pipe->pipe_size[pipe->pointer_out];
    }

    pipe->total_data -= pipe->pipe_size[pipe->pointer_out];
    pipe->data_index = 0;

    // move out-pointer
	pipe->pointer_out++;
	if (pipe->pointer_out == pipe->max_size) pipe->pointer_out = 0;
    
    EI();
    
	return(ret_value);
}

uint8_t * pipe_get(pipe_data_t * pipe, uint16_t * size)
{
    uint8_t * ret_value = NULL;
    
    if (pipe == NULL) return(NULL);
    
    DI();
    
    // buffer is empty
    if (pipe->pointer_in == pipe->pointer_out)
    {
        EI();
        return(NULL);
    }
    
    // get byte from buffer
    ret_value = pipe->pipe_list[pipe->pointer_out];
    
    if (size != NULL)
        *size =     pipe->pipe_size[pipe->pointer_out];
    
    EI();
    
    return(ret_value);
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

void pipe_send_buffer(pipe_data_t * pipe, uint8_t * buffer, uint16_t size)
{
    uint8_t * buf_tmp = pipe_alloc_buffer(size);
    if (buf_tmp == NULL) return;
    
    memcpy(buf_tmp, buffer, size);
    pipe_write(pipe, buf_tmp, size);
}

void pipe_write(pipe_data_t * pipe, uint8_t * buffer, uint16_t size)
{
    if (pipe == NULL) return;

    DI();
    
    // insert data into the buffer
	pipe->pipe_list[pipe->pointer_in] = buffer;
	pipe->pipe_size[pipe->pointer_in] = size;
	pipe->pointer_in++;
    pipe->total_data += size;
    
    // update pointers and control overflow
	if (pipe->pointer_in == pipe->max_size) pipe->pointer_in = 0;

    // overflow, delete buffer
	if (pipe->pointer_in == pipe->pointer_out)
    {
        pipe_release_buffer(pipe->pipe_list[pipe->pointer_out]);
        pipe->pointer_out++;
	}
    
	if (pipe->pointer_out == pipe->max_size) pipe->pointer_out = 0;
    
    EI();
}

bool_t pipe_is_full(pipe_data_t * pipe)
{
    bool_t ret = FALSE;
    
    if (pipe == NULL) return(TRUE);
    
    DI();
    
    // input is just before output
	if (pipe->pointer_in == (pipe->pointer_out - 1)) ret = TRUE ;

    // input is in the tail, and output is in the head
	else if ((pipe->pointer_in == (pipe->max_size - 1)) &&
             (pipe->pointer_out == 0)) ret = TRUE;

    EI();
    
    // not full
    return(ret);
}

bool_t pipe_is_empty(pipe_data_t * pipe)
{
    bool_t ret = FALSE;
    
    if (pipe == NULL) return(TRUE);
    
    DI();
    
	if (pipe->pointer_in == pipe->pointer_out) ret = TRUE;
    
    EI();
    
    return(ret);
}

uint16_t pipe_get_used(pipe_data_t * pipe)
{
    uint16_t size;

    if (pipe == NULL) return(0);
    
    DI();
    
    if (pipe->pointer_in >= pipe->pointer_out)
    {
        // in and out at same side
        size = pipe->pointer_in - pipe->pointer_out;
    }
    else
    {
        // in turned around
        size = ((pipe->max_size - 1) - pipe->pointer_out) + pipe->pointer_in;
    }
    
    EI();
    
    return(size);
}

