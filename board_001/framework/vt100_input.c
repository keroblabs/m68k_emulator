#include "vt100_input.h"

typedef struct key_to_vt100_t_
{
    uint16_t event;
    const char * vt100;

} key_to_vt100_t;

static const key_to_vt100_t key_to_vt100[] = 
{
    { EVENT_KEY_HOME,       "\x1b[1~" },
    { EVENT_KEY_END,        "\x1b[4~" },
    { EVENT_KEY_DELETE,     "\x1b[3~" },
    { EVENT_KEY_PAGEUP,     "\x1b[5~" },
    { EVENT_KEY_PAGEDOWN,   "\x1b[6~" },

    { EVENT_KEY_UP,         "\x1b[A"  },
    { EVENT_KEY_DOWN,       "\x1b[B"  },
    { EVENT_KEY_RIGHT,      "\x1b[C"  },
    { EVENT_KEY_LEFT,       "\x1b[D"  },
    { 0,                    NULL      }
};
