#ifdef USE_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <fcntl.h>
#include <errno.h>
#include "console_local.h"
#include "comm_host_interface.h"

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_STDOUT      _fileno(stdout)
#define DEFAULT_STDIN       _fileno(stdin)

int  _kbhit(void);
int _getch(void);

#define LOCAL_HANDLER       4096

//////////////////////////////////////////////////////////////////////////////////////

static int c_pending;
static bool_t has_pending = FALSE;
static bool_t local_set = FALSE;

static DWORD old_console_ode;

//////////////////////////////////////////////////////////////////////////////////////

void interface_disable_raw_mode(void) 
{
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), old_console_ode);
}

void interface_set_raw_mode(void) 
{
    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &old_console_ode);

    // Disable input buffering and echo
    DWORD newMode = 0;
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), newMode);    
}

uint32_t console_local_wait_accept(void)
{
    if (local_set == FALSE)
    {
        local_set = TRUE;
        return LOCAL_HANDLER;
    }

    return INTERFACE_NOT_READY;
}

void console_local_init(void)
{
    interface_set_raw_mode();

    // At exit, restore the original state.
    atexit(interface_disable_raw_mode);
}

uint32_t console_local_prepare(void)
{
    return LOCAL_HANDLER;
}

void console_force_exit(const char* s) 
{
    perror(s);
    printf("\r\n");
    exit(1);
}

void console_local_output(const char * data, uint32_t size)
{
    write(DEFAULT_STDOUT, data, size);
}

int console_local_get_windows_size(int* screen_rows, int* screen_cols) 
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int ret;
    ret = GetConsoleScreenBufferInfo(GetStdHandle( STD_OUTPUT_HANDLE ),&csbi);
    if (ret)
    {
        console_force_exit("Failed to get window size");
    } 
    else 
    {
        *screen_cols = csbi.dwSize.X;
        *screen_rows = csbi.dwSize.Y;
    }
    return 0;
}

bool_t console_local_check_input(void)
{
    if (_kbhit())
    {
        c_pending = _getch();
        has_pending = TRUE;
        return TRUE;
    }

    return FALSE;
}

int console_local_read_key(void) 
{
    int c;

    if (has_pending == FALSE)
    {
        return 0;
    }
    else
    {
        c = c_pending;
        has_pending = FALSE;
    }

    if (c == '\x1b') {
        return '\x1b';
    } else if (c == 127) {
        return '\b';
    } else if (c == 13) {
        return 10;
    } else if (c == 0x08) {
        return '\b';
    } else if (c == 0xE0) {
        char seq[3];
        if (_kbhit())
        {
            seq[0] = _getch();
            switch(seq[0])
            {
                case 'H': return ARROW_UP;
                case 'P': return ARROW_DOWN;
                case 'M': return ARROW_RIGHT;
                case 'K': return ARROW_LEFT;
                case 'G': return HOME_KEY;
                case 'O': return END_KEY;
                case 'I': return PAGE_UP;
                case 'Q': return PAGE_DOWN;
            }
        }
    } else {
        return c;
    }

    return 0;
}
#endif
