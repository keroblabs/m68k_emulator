#ifndef _console_local_h_
#define _console_local_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

enum editor_key 
{
    BACKSPACE = 0x7f, // 127
    ARROW_LEFT = 0x3e8, // 1000, large value out of the range of a char.
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DEL_KEY
};

//////////////////////////////////////////////////////////////////////////////////////

#define CTRL_KEY(k) ((k) & 0x1f)

//////////////////////////////////////////////////////////////////////////////////////

void console_local_output(const char * data, uint32_t size);

int console_local_get_windows_size(int* screen_rows, int* screen_cols);

int console_local_read_key(void);

bool_t console_local_check_input(void);

uint32_t console_local_wait_accept(void);

uint32_t console_local_prepare(void);

void console_local_init(void);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _console_local_h_
