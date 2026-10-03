#include "pool.h"

void pool_create( pool_data_t * pool_data, uint16_t element_size, void * buffer, uint16_t buffer_size )
{
    uint8_t * temp_buffer = ( uint8_t * )buffer;
    uint32_t temp_index = 0;

    pool_data->element_count = 0;
    pool_data->element_size = element_size;
    pool_data->active_pool = NULL;
    pool_data->free_pool = NULL;

    while( temp_index < buffer_size )
    {
        pool_element_t * element = ( pool_element_t * )&temp_buffer[ temp_index ];

        element->p_next = pool_data->free_pool;
        pool_data->free_pool = element;

        temp_index += ( element_size + sizeof( pool_element_t ) );
    }
}

void *pool_malloc( pool_data_t * pool_data )
{
    void * ret = NULL;

    if( pool_data->free_pool != NULL )
    {
        pool_element_t * element = pool_data->free_pool;
        pool_data->free_pool = element->p_next;

        element->p_next = pool_data->active_pool;
        pool_data->active_pool = element;
        pool_data->element_count++;
        
        ret = element->data;
    }

    return ret;
}

void *pool_locate( pool_data_t * pool_data, void * p_data, pool_search_hdl search_hdl )
{
    pool_element_t * search_element = pool_data->active_pool;
    void * ret = NULL;

    while( search_element && search_hdl )
    {
        if( search_hdl( search_element->data, p_data ) )
        {
            ret = search_element->data;
            break;
        }
        search_element = search_element->p_next;
    }

    return ret;
}

void pool_free( pool_data_t * pool_data, void * p_element )
{
    pool_element_t * element = ( pool_element_t * )( ( void * )( ( uint64_t )p_element - sizeof( pool_element_t ) ) );
    pool_element_t * search_element = pool_data->active_pool;
    pool_element_t * prev_element;

    while( search_element != NULL )
    {
        if( search_element == element )
        {
            if( prev_element == NULL )
            {
                pool_data->active_pool = element->p_next;
            }
            else
            {
                prev_element->p_next = element->p_next;
            }

            element->p_next = pool_data->free_pool;
            pool_data->free_pool = element;
            pool_data->element_count--;
            break;
        }

        search_element = search_element->p_next;
    }
}

uint32_t pool_get_count( pool_data_t * pool_data )
{
    return pool_data->element_count;
}
