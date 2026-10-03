#include "console_local.h"
#include "comm_host_interface.h"

#ifdef UNIX
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#include <fcntl.h>
#include <errno.h>

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_STDOUT      STDIN_FILENO
#define DEFAULT_STDIN       STDOUT_FILENO

int  _kbhit(void);
int _getch(void);

#define LOCAL_HANDLER       4096

//////////////////////////////////////////////////////////////////////////////////////

static int c_pending;
static bool_t has_pending = FALSE;
static bool_t local_set = FALSE;

static struct termios termios;

//////////////////////////////////////////////////////////////////////////////////////

void interface_disable_raw_mode(void) 
{
    tcgetattr(0, &termios);
    termios.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &termios);
}

void interface_set_raw_mode(void) 
{
    tcgetattr(0, &termios);
    termios.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
    tcsetattr(0, TCSANOW, &termios);
}

int  _kbhit(void)
{
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    return byteswaiting > 0;
}

int _getch(void)
{
    return getchar();
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
    struct winsize ws;

    // Getting window size thanks to ioctl into the given
    // winsize struct.
    if (ioctl(DEFAULT_STDOUT, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) 
    {
        console_force_exit("Failed to get window size");
    } 
    else 
    {
        *screen_cols = ws.ws_col;
        *screen_rows = ws.ws_row;
    }
    return 0;
}

bool_t console_local_check_input(void)
{
    char c;
    int nread;
    nread = read(DEFAULT_STDIN, &c, 1);

    if (nread == 1)
    {
        has_pending = TRUE;
        c_pending = c;
    }
    else if (nread == -1 && errno != EAGAIN)
    {
        //console_force_exit("Error reading input");    
        return FALSE;
    }

    return nread ? TRUE : FALSE;
}

int console_local_read_key(void) 
{
    int nread;
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
        char seq[3];

        if (read(DEFAULT_STDIN, &seq[0], 1) != 1 ||
            read(DEFAULT_STDIN, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(DEFAULT_STDIN, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        // Home and End keys may be sent in many ways depending on the OS
                        // \x1b[1~, \x1b[7~, \x1b[4~, \x1b[8~
                        case '1':
                        case '7':
                            return HOME_KEY;
                        case '4':
                        case '8':
                            return END_KEY;
                        // Del key is sent as \x1b[3~
                        case '3':
                            return DEL_KEY;
                        // Page Up and Page Down send '\x1b', '[', '5' or '6' and '~'.
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                    }
                }
            } else {
                switch (seq[1]) {
                    // Arrow keys send multiple bytes starting with '\x1b', '[''
                    // and followed by an 'A', 'B', 'C' or 'D' depending on which
                    // arrow is pressed.
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    // Home key can also be sent as \x1b[H
                    case 'H': return HOME_KEY;
                    // End key can also be sent as \x1b[F
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                // Yes, Home key can ALSO be sent as \x1bOH
                case 'H': return HOME_KEY;
                // And... End key as \x1bOF
                case 'F': return END_KEY;
            }
        }
    } else if (c == 127) {
        return '\b';
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
}
#endif
