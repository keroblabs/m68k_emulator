#ifndef __POOL_DATA_H__
#define __POOL_DATA_H__

////////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

////////////////////////////////////////////////////////////////////////////////////////

#define MAX_PIPE_SIZE               32
#define POOL_ELEMENT_SIZEOF(t)      (sizeof(t) + sizeof(pool_element_t))

////////////////////////////////////////////////////////////////////////////////////////

typedef struct pool_element_t_
{
    struct pool_element_t_ * p_next;
    uint8_t data[];

} pool_element_t;

typedef struct
{
    uint16_t element_size;
    uint16_t element_count;

    pool_element_t * free_pool;
    pool_element_t * active_pool;
    
} pool_data_t;

typedef bool_t (*pool_search_hdl)( void * element, void * p_data );

////////////////////////////////////////////////////////////////////////////////////////

void     pool_create( pool_data_t * pool_data, uint16_t element_size, void * buffer, uint16_t buffer_size );
void *   pool_malloc( pool_data_t * pool_data );
void *   pool_locate( pool_data_t * pool_data, void * p_data, pool_search_hdl search_hdl );
void     pool_free( pool_data_t * pool_data, void * element );

uint32_t pool_get_count( pool_data_t * pool_data );

////////////////////////////////////////////////////////////////////////////////////////

#endif // __POOL_DATA_H__

