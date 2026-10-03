#ifndef _events_enum_h_
#define _events_enum_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

enum
{
    INPUT_EVENT_NONE = 0,

    INPUT_EVENT_KEY_ENTER = 0x1000,
    INPUT_EVENT_KEY_BACKSPACE,
    INPUT_EVENT_KEY_TAB,
    INPUT_EVENT_KEY_ESC,
    INPUT_EVENT_KEY_DELETE,
    INPUT_EVENT_KEY_PAGEUP,
    INPUT_EVENT_KEY_PAGEDOWN,
    INPUT_EVENT_KEY_HOME,
    INPUT_EVENT_KEY_END,
    INPUT_EVENT_KEY_UP,
    INPUT_EVENT_KEY_DOWN,
    INPUT_EVENT_KEY_LEFT,
    INPUT_EVENT_KEY_RIGHT,

    INPUT_EVENT_QUIT,
    INPUT_EVENT_SAVE,
    INPUT_EVENT_SEARCH,
    INPUT_EVENT_FLIP_UPWARDS,
    INPUT_EVENT_FLIP_DOWNWARDS,
    INPUT_EVENT_CUT_LINE,
    INPUT_EVENT_COPY_LINE,
    INPUT_EVENT_PASTE_LINE,
    INPUT_EVENT_UNDO,
    INPUT_EVENT_REDO,
    INPUT_EVENT_HISTORY,

    INPUT_EVENT_PROMPT,
    INPUT_EVENT_SHOW,
    INPUT_EVENT_KEYPRESS,
};

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

#endif // _events_enum_h_
