#include "SDL2/SDL.h"
#include "types.h"
#include "sdl_lib_export.h"
#include "sdl_window.h"

//////////////////////////////////////////////////////////////////////////////////////

typedef struct sdl_user_data_t_
{
    struct sdl_user_data_t_ * p_next;
    uint32_t id;
    void * p_data;
    
} sdl_user_data_t;

typedef struct gui_instane_t_
{
    SDL_Window   * window;
    SDL_Renderer * renderer;
    SDL_Surface  * surface;
    SDL_Texture  * texture;

    void * p_data;

    sdl_user_data_t * user_data_list;

    event_mouse_t mouse_handler;
    event_delta_t delta_handler;
    event_key_t   key_handler;
    do_events_t   do_events;

    uint32_t id;

    uint16_t width;
    uint16_t height;

    uint32_t vsync_ms;

    struct gui_instane_t_ * p_next;

} gui_instance_t;

//////////////////////////////////////////////////////////////////////////////////////

static gui_instance_t * first_instance;
static gui_instance_t * last_instance;

//////////////////////////////////////////////////////////////////////////////////////

static gui_instance_t * find_window( uint32_t id )
{
    gui_instance_t * search_instance = first_instance;

    while( search_instance != NULL )
    {
        if( search_instance->id == id )
        {
            return search_instance;
        }

        search_instance = search_instance->p_next;
    }

    return NULL;
}

static void dummy_background( uint32_t window_id )
{

}

void sdl_send_key_event( uint32_t window_id, uint8_t key_code, uint16_t control_key, uint8_t state )
{
    gui_instance_t * instance = find_window( window_id );

    if( ( instance != NULL ) && ( instance->key_handler != NULL ) )
    {
        instance->key_handler( window_id, key_code, control_key, state );
    }
}

void sdl_send_mouse_event( uint32_t window_id, uint16_t posx, uint16_t posy, uint16_t posz )
{
    gui_instance_t * instance = find_window( window_id );

    if( ( instance != NULL ) && ( instance->mouse_handler != NULL ) )
    {
        instance->mouse_handler( window_id, posx, posy, posz );
    }
}

void sdl_send_mouse_delta_event( uint32_t window_id, int32_t deltax, int32_t deltay, uint16_t posz )
{
    gui_instance_t * instance = find_window( window_id );

    if( ( instance != NULL ) && ( instance->delta_handler != NULL ) )
    {
        instance->delta_handler( window_id, deltax, deltay, posz );
    }
}

void sdl_window_set_mouse_event( uint32_t window_id, event_mouse_t handler )
{
    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        instance->mouse_handler = handler;
    }
}

void sdl_window_set_mouse_delta_event( uint32_t window_id, event_delta_t handler )
{
    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        instance->delta_handler = handler;
    }
}

void sdl_window_set_key_event( uint32_t window_id, event_key_t handler )
{
    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        instance->key_handler = handler;
    }
}

do_events_t sdl_window_set_background_event( uint32_t window_id, do_events_t handler )
{
    do_events_t ret = NULL;
    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        ret = instance->do_events;
        instance->do_events = handler;
    }

    return ret;
}

uint16_t sdl_window_get_width( uint32_t window_id )
{
    uint16_t ret = 0;

    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        ret = instance->width;
    }

    return ret;
}

uint16_t sdl_window_get_height( uint32_t window_id )
{
    uint16_t ret = 0;

    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        ret = instance->height;
    }

    return ret;
}

void * sdl_windows_malloc( uint32_t window_id, uint32_t data_id, uint32_t size )
{
    void * ret = NULL;

    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        sdl_user_data_t * user_data = ( sdl_user_data_t * )malloc( sizeof( sdl_user_data_t ) );
        if( user_data == NULL )
        {
            return NULL;
        }

        user_data->p_data = malloc( size );
        if( user_data->p_data == NULL )
        {
            free( user_data );
            return NULL;
        }

        user_data->id = data_id;
        user_data->p_next = instance->user_data_list;
        instance->user_data_list = user_data;

        ret = user_data->p_data;
    }

    return ret;
}

void * sdl_windows_get_heap_data( uint32_t window_id, uint32_t data_id )
{
    void * ret = NULL;

    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        sdl_user_data_t * search_data = instance->user_data_list;

        while( search_data != NULL )
        {
            if( search_data->id == data_id )
            {
                ret = search_data->p_data;
                break;
            }

            search_data = search_data->p_next;
        }
    }

    return ret;
}

void * sdl_window_get_user_data( uint32_t window_id )
{
    void * ret = NULL;

    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        ret = instance->p_data;
    }

    return ret;
}

void sdl_window_set_user_data( uint32_t window_id, void * p_data )
{
    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        instance->p_data = p_data;
    }
}

uint32_t sdl_create_window( const char * name, uint16_t width, uint16_t height )
{
    Uint32 r_mask, g_mask, b_mask, a_mask;
    int bpp;

    SDL_PixelFormatEnumToMasks( SDL_PIXEL_FORMAT, &bpp, &r_mask, &g_mask, &b_mask, &a_mask );

    gui_instance_t * instance = malloc( sizeof( gui_instance_t ) );
    if( instance == NULL )
    {
        printf( "SDL: failed to allocate instance\n" );
        return NEG_U32;
    }

    memset( instance, 0x00, sizeof( gui_instance_t ) );
    instance->width = width;
    instance->height = height;
    instance->mouse_handler = NULL;
    instance->delta_handler = NULL;
    instance->key_handler = NULL;
    instance->do_events = dummy_background;

    instance->window = SDL_CreateWindow( name,
                                         SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED,
                                         width, height, 0 );

    instance->id = SDL_GetWindowID( instance->window );

    if( instance->window == NULL )
    {
        printf( "SDL: failed to create window\n" );
        free( instance );
        return NEG_U32;
    }

    instance->renderer = SDL_CreateRenderer( instance->window, -1, SDL_RENDERER_ACCELERATED );
    if( instance->renderer == NULL )
    {
        printf( "SDL: failed to create render\n" );
        free( instance );
        return NEG_U32;
    }

    instance->surface = SDL_CreateRGBSurface( 0, width, height, bpp, r_mask, g_mask, b_mask, a_mask);
    if( instance->surface == NULL )
    {
        printf( "SDL: failed to create surface\n" );
        return NEG_U32;
    }

    instance->texture = SDL_CreateTexture( instance->renderer, SDL_PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING, width, height);
    if( instance->texture == NULL )
    {
        printf( "SDL: failed to create texture\n" );
        return NEG_U32;
    }

    instance->vsync_ms = 0; // SDL_GetTicks() + VSYNC_PERIOD_MS;

    //Fill the surface white
    SDL_FillRect( instance->surface, NULL, SDL_MapRGB( instance->surface->format, 0, 0, 0 ) );

    //Update the surface
    SDL_UpdateWindowSurface( instance->window );

    if( first_instance == NULL )
    {
        first_instance = instance;
    }
    else
    {
        last_instance->p_next = instance;
    }

    last_instance = instance;

    return instance->id;
}

void sdl_window_set_refresh_time( uint32_t window_id, uint32_t time )
{
    gui_instance_t * instance = find_window( window_id );
    instance->vsync_ms = SDL_GetTicks() + time;    
}

void * sdl_get_buffer( uint32_t window_id )
{
    PIXEL_TYPE_T * ret = NULL;
    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        ret = ( PIXEL_TYPE_T * )instance->surface->pixels;
    }

    return ret;
}

void sdl_destroy_window( uint32_t window_id )
{
    gui_instance_t * instance = find_window( window_id );

    if( instance != NULL )
    {
        SDL_DestroyTexture( instance->texture );
        SDL_DestroyRenderer( instance->renderer );
        SDL_DestroyWindow( instance->window );
    }
}

void sdl_terminate( void )
{
    gui_instance_t * search_instance = first_instance;

    while( search_instance != NULL )
    {
        sdl_destroy_window( search_instance->id );
        search_instance = search_instance->p_next;
    }
}

void sdl_window_process( void )
{
    gui_instance_t * search_instance = first_instance;

    while( search_instance != NULL )
    {
        if( search_instance->window != NULL )
        {
            if( ( search_instance->vsync_ms != 0 ) && ( SDL_GetTicks() >= search_instance->vsync_ms ) )
            {
                search_instance->vsync_ms = SDL_GetTicks() + VSYNC_PERIOD_MS;

                SDL_UpdateTexture( search_instance->texture, NULL, search_instance->surface->pixels, search_instance->surface->pitch );
                SDL_RenderClear( search_instance->renderer );
                SDL_RenderCopy( search_instance->renderer, search_instance->texture, NULL, NULL );
                SDL_RenderPresent( search_instance->renderer );
            }

            if( search_instance->do_events != NULL )
            {
                search_instance->do_events( search_instance->id );
            }
        }

        search_instance = search_instance->p_next;
    }
}

void sdl_update_window( uint32_t window_id )
{
    gui_instance_t * instance = find_window( window_id );

    SDL_UpdateTexture( instance->texture, NULL, instance->surface->pixels, instance->surface->pitch );
    SDL_RenderClear( instance->renderer );
    SDL_RenderCopy( instance->renderer, instance->texture, NULL, NULL );
    SDL_RenderPresent( instance->renderer );
}
