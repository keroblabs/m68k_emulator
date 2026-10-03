#include "buffer.h"

////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////

void buffer_init(buffer_data_t * buffer_data, uint8_t * buffer_ptr, uint16_t size)
{
    // init buffer
    buffer_data->buffer = buffer_ptr;
    buffer_data->size = size;
    buffer_data->pointer_in = 0;
    buffer_data->pointer_out = 0;
    buffer_data->mutex = tthread_mutex_init();
}

void buffer_reset(buffer_data_t * buffer_data)
{
	if (buffer_data == NULL) return;
    buffer_data->pointer_in = 0;
    buffer_data->pointer_out = 0;
}

void buffer_set_buffer(buffer_data_t * buffer_data, uint8_t * buffer_ptr)
{
    buffer_data->buffer = buffer_ptr;
}

////////////////////////////////////////////////////////////////////////////////

static uint8_t buffer_read_internal(buffer_data_t * buffer_data)
{
	uint8_t ret_value = 0;

	if (buffer_data == NULL) return 0;

    // buffer is empty
	if (buffer_data->pointer_in == buffer_data->pointer_out)
    {
        return 0;
    }

    // get byte from buffer
	ret_value = buffer_data->buffer[buffer_data->pointer_out];

    // move out-pointer
	buffer_data->pointer_out++;
	if (buffer_data->pointer_out == buffer_data->size) buffer_data->pointer_out = 0;
    
	return ret_value;
}

static void buffer_write_internal(buffer_data_t * buffer_data, uint8_t in_data)
{	
	if (buffer_data == NULL) return;
	
    // insert data into the buffer
	buffer_data->buffer[buffer_data->pointer_in] = in_data;
	buffer_data->pointer_in++;
    
    // update pointers and control overflow
	if (buffer_data->pointer_in == buffer_data->size) buffer_data->pointer_in = 0;
	if (buffer_data->pointer_in == buffer_data->pointer_out) buffer_data->pointer_out++;
	if (buffer_data->pointer_out == buffer_data->size) buffer_data->pointer_out = 0;
}

void buffer_write(buffer_data_t * buffer_data, uint8_t in_data)
{
    tthread_mutex_lock(buffer_data->mutex);
    buffer_write_internal(buffer_data, in_data);
    tthread_mutex_unlock(buffer_data->mutex);
}

uint8_t buffer_read(buffer_data_t * buffer_data)
{
    uint8_t ret = 0;

    tthread_mutex_lock(buffer_data->mutex);
    ret = buffer_read_internal(buffer_data);
    tthread_mutex_unlock(buffer_data->mutex);

    return ret;
}

void buffer_write_pointer(buffer_data_t * buffer_data, void * p_pointer)
{
    uint8_t * p_value = (uint8_t *)&p_pointer;
    buffer_write_buffer(buffer_data, p_value, sizeof(void *));
}

void * buffer_read_pointer(buffer_data_t * buffer_data)
{
    void * ret = NULL;

    if (buffer_is_empty(buffer_data) == FALSE)
    {
        buffer_read_buffer(buffer_data, (uint8_t *)&ret, sizeof(void *));
    }

    return ret;
}

void buffer_write_buffer(buffer_data_t * buffer_data, uint8_t * in_data, uint16_t size)
{
	uint16_t counter;
	
    tthread_mutex_lock(buffer_data->mutex);
    
	for (counter = 0; counter < size; counter++)
	{
		buffer_write_internal(buffer_data, in_data[counter]);
	}
    
    tthread_mutex_unlock(buffer_data->mutex);
}

uint16_t buffer_read_buffer(buffer_data_t * buffer_data, uint8_t * out_data, uint16_t size)
{
	uint16_t counter;
	
    tthread_mutex_lock(buffer_data->mutex);
    
	for (counter = 0; counter < size; counter++)
	{
		// buffer is empty
		if (buffer_data->pointer_in == buffer_data->pointer_out) break;
	
		out_data[counter] = buffer_read_internal(buffer_data);
	}
	
    tthread_mutex_unlock(buffer_data->mutex);
    
	return counter;
}

uint16_t buffer_get_size(buffer_data_t * buffer_data)
{
    uint16_t size;

	if (buffer_data == NULL) return 0;
    
    tthread_mutex_lock(buffer_data->mutex);
    
    if (buffer_data->pointer_in >= buffer_data->pointer_out)
    {
        // in and out at same side
        size = buffer_data->pointer_in - buffer_data->pointer_out;
    }
    else
    {
        // in turned around
        size = (buffer_data->size - buffer_data->pointer_out) + buffer_data->pointer_in;
    }

    tthread_mutex_unlock(buffer_data->mutex);
    
    return size;
}

uint16_t buffer_get_available_size(buffer_data_t * buffer_data)
{
    return buffer_data->size - buffer_get_size(buffer_data);
}

bool_t buffer_is_full(buffer_data_t * buffer_data)
{        
    bool_t ret = FALSE;
    
	if (buffer_data == NULL) return FALSE;
	
    tthread_mutex_lock(buffer_data->mutex);
    
    // input is just before output
	if (buffer_data->pointer_in == (buffer_data->pointer_out - 1)) ret = TRUE;

    // input is in the tail, and output is in the head
	else if ((buffer_data->pointer_in == (buffer_data->size - 1)) &&
             (buffer_data->pointer_out == 0)) ret = TRUE;

    tthread_mutex_unlock(buffer_data->mutex);

    return ret;
}

bool_t buffer_is_empty(buffer_data_t * buffer_data)
{
    bool_t ret = FALSE;
    
	if (buffer_data == NULL) return TRUE;

    tthread_mutex_lock(buffer_data->mutex);

	if (buffer_data->pointer_in == buffer_data->pointer_out) ret = TRUE;

    tthread_mutex_unlock(buffer_data->mutex);
    
    return ret;
}

