#include "SDL2/SDL.h"
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "types.h"
#include "sdl_lib_export.h"
#include "sdl_window.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

#if 0
void sdl_mouse_init( void )
{
    mouse_trigg = 0;
    mouse_button = 0;
}

bool_t sdl_mouse_get_pos( uint16_t * posx, uint16_t * posy, uint16_t * posz )
{
    if( mouse_trigg == 1 )
    {
        mouse_trigg = 0;

        *posx = mouse_x;
        *posy = mouse_y;
        *posz = ( mouse_button ) ? 100 : 0;

        return TRUE;
    }

    return FALSE;
}
#endif

void sdl_mouse_event( uint32_t type, SDL_MouseButtonEvent event, SDL_MouseMotionEvent * motion )
{
    uint32_t mouse_x;
    uint32_t mouse_y;
    int32_t delta_mouse_x;
    int32_t delta_mouse_y;
    uint8_t mouse_button;
    bool_t mouse_trigg = 0;

    switch( type )
    {
        case SDL_MOUSEBUTTONDOWN:
            mouse_x = event.x;
            mouse_y = event.y;
            if( event.button == SDL_BUTTON_LEFT )
            {
                mouse_button = 1;
            }
            else if( event.button == SDL_BUTTON_RIGHT )
            {
                mouse_button = 2;
            }
            else break;
            mouse_trigg = 1;
            break;

        case SDL_MOUSEBUTTONUP:
            mouse_button = 0;
            mouse_x = event.x;
            mouse_y = event.y;
            mouse_trigg = 1;
            break;

        case SDL_MOUSEMOTION:
            // uncomment the line below to enable touchscreen mode
            // comment the line below to enable mouse mode
            //if( mouse_button == 0 ) break;

            mouse_x = event.x;
            mouse_y = event.y;
            delta_mouse_x = motion->xrel;
            delta_mouse_y = motion->yrel;
            mouse_trigg = 1;

            //printf( "%4d x %4d\n", mouse_x, mouse_y );
            //fflush(stdout);
            break;
    }

    if( mouse_trigg == TRUE )
    {
        sdl_send_mouse_event( event.windowID, ( uint16_t )mouse_x, ( uint16_t )mouse_y, mouse_button ? 100 : 0 );
        sdl_send_mouse_delta_event( event.windowID, delta_mouse_x, delta_mouse_y, mouse_button ? 100 : 0 );
    }
}

