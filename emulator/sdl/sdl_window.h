#ifndef SDL_WINDOW_H_
#define SDL_WINDOW_H_

//////////////////////////////////////////////////////////////////////////////////////

#ifdef TIMA_LINUX
#define SDL_PIXEL_FORMAT SDL_PIXELFORMAT_XRGB8888
#define PIXEL_TYPE_T     uint32_t
#else
#define SDL_PIXEL_FORMAT SDL_PIXELFORMAT_RGB565
#define PIXEL_TYPE_T     uint16_t
#endif

#define VSYNC_RATE_HZ		50
#define VSYNC_PERIOD_MS		( 1000 / VSYNC_RATE_HZ )

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

uint32_t sdl_create_window( const char * name, uint16_t width, uint16_t height );

void * sdl_get_buffer( uint32_t window_id );

void sdl_destroy_window( uint32_t window_id );

void sdl_terminate( void );

void sdl_window_process( void );

void sdl_update_window( uint32_t window_id );

void * sdl_window_get_user_data( uint32_t window_id );
void   sdl_window_set_user_data( uint32_t window_id, void * p_data );

void * sdl_windows_malloc( uint32_t window_id, uint32_t data_id, uint32_t size );
void * sdl_windows_get_heap_data( uint32_t window_id, uint32_t data_id );

void sdl_send_key_event( uint32_t window_id, uint8_t key_code, uint16_t control_key, uint8_t state );
void sdl_send_mouse_event( uint32_t window_id, uint16_t posx, uint16_t posy, uint16_t posz );
void sdl_send_mouse_delta_event( uint32_t window_id, int32_t deltax, int32_t deltay, uint16_t posz );

void sdl_window_set_mouse_delta_event( uint32_t window_id, event_delta_t handler );
void sdl_window_set_mouse_event( uint32_t window_id, event_mouse_t handler );
void sdl_window_set_key_event( uint32_t window_id, event_key_t handler );

do_events_t sdl_window_set_background_event( uint32_t window_id, do_events_t handler );

uint16_t sdl_window_get_width( uint32_t window_id );
uint16_t sdl_window_get_height( uint32_t window_id );

//////////////////////////////////////////////////////////////////////////////////////

#endif // SDL_WINDOW_H_
