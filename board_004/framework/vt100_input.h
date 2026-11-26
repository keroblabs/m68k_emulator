#ifndef _vt100_ctrl_h_
#define _vt100_ctrl_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

#define SHIFT_FLAG      0x01
#define CAPS_FLAG       0x02
#define CTRL_FLAG       0x04
#define ALT_FLAG		0x08
#define ALTGR_FLAG		0x10
#define WIN_FLAG		0x20
#define CLOSEAPL_FLAG   0x40
#define OPENAPL_FLAG    0x80

#define KEY_DOWN        0x01
#define KEY_UP          0x00

enum
{
    EVENT_KEY_ESC = 256,
    EVENT_KEY_TAB,
    EVENT_KEY_ENTER,
    EVENT_KEY_BACKSPACE,
    EVENT_KEY_DELETE,

    EVENT_KEY_PAGEUP,
    EVENT_KEY_PAGEDOWN,

    EVENT_KEY_HOME,
    EVENT_KEY_END,

    EVENT_KEY_UP,
    EVENT_KEY_DOWN,
    EVENT_KEY_LEFT,
    EVENT_KEY_RIGHT,
};

//////////////////////////////////////////////////////////////////////////////////////

void cli_ctrl_process(void);

void cli_ctrl_init(void);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _vt100_ctrl_h_
