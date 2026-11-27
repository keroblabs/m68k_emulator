#include "vdp9918_vt100.h"
#include "vdp9918_driver.h"
#include "vdp9918_types.h"

//////////////////////////////////////////////////////////////////////////////////////

#define IS_DIGIT(a)         ((a >= '0') && (a <= '9'))

#define TEXT_COLS           TMS9918_TEXT_COLS
#define TEXT_ROWS           TMS9918_TEXT_ROWS

#define CURSOR_CHAR         VDP_CURSOR_GLYPH

#define NEG_16              ((uint16_t)-1)

//////////////////////////////////////////////////////////////////////////////////////

enum
{
    VT100_MODE_IDLE,
    VT100_MODE_ESCAPE,
    VT100_MODE_BRACKET,
    VT100_MODE_DIGITS,
};

//////////////////////////////////////////////////////////////////////////////////////

/* Shadow copy of what is currently on the 40x24 character grid. The VDP VRAM
 * is write-only from the CPU side in practice, so we keep a local mirror to be
 * able to scroll, delete and restore characters under the cursor. */
static char screen[TEXT_ROWS][TEXT_COLS];

static uint16_t char_posx;
static uint16_t char_posy;

static bool_t cursor_enabled;
static bool_t scroll_enabled;
static bool_t is_set_reset;
static bool_t cursor_drawn;

static const char vt100_cmds[] = "mKJHlhABCD~";

//////////////////////////////////////////////////////////////////////////////////////

static void put_cell(uint16_t x, uint16_t y, char c)
{
    if ((x >= TEXT_COLS) || (y >= TEXT_ROWS))
    {
        return;
    }

    if (c < ' ')
    {
        c = ' ';
    }

    screen[y][x] = c;
    vdp9918_text_char_out(c, x, y, 0, 0, FALSE);
}

static void redraw_all(void)
{
    uint16_t x;
    uint16_t y;

    for (y = 0; y < TEXT_ROWS; y++)
    {
        for (x = 0; x < TEXT_COLS; x++)
        {
            vdp9918_text_char_out(screen[y][x], x, y, 0, 0, FALSE);
        }
    }
}

static void clear_screen(void)
{
    uint16_t x;
    uint16_t y;

    for (y = 0; y < TEXT_ROWS; y++)
    {
        for (x = 0; x < TEXT_COLS; x++)
        {
            screen[y][x] = ' ';
        }
    }

    vdp9918_text_clear_screen();

    char_posx = 0;
    char_posy = 0;
    cursor_drawn = FALSE;
}

static void clear_line(uint16_t y)
{
    uint16_t x;

    if (y >= TEXT_ROWS)
    {
        return;
    }

    for (x = 0; x < TEXT_COLS; x++)
    {
        screen[y][x] = ' ';
    }

    vdp9918_text_clear_line(0, y);
}

static void hide_cursor(void)
{
    if (cursor_drawn == TRUE)
    {
        vdp9918_text_char_out(screen[char_posy][char_posx], char_posx, char_posy, 0, 0, FALSE);
        cursor_drawn = FALSE;
    }
}

static void show_cursor(void)
{
    if ((cursor_enabled == TRUE) && (cursor_drawn == FALSE))
    {
        vdp9918_text_char_out(CURSOR_CHAR, char_posx, char_posy, 0, 0, FALSE);
        cursor_drawn = TRUE;
    }
}

static void scroll_up(void)
{
    uint16_t y;
    uint16_t x;

    for (y = 1; y < TEXT_ROWS; y++)
    {
        memcpy(screen[y - 1], screen[y], TEXT_COLS);
    }

    for (x = 0; x < TEXT_COLS; x++)
    {
        screen[TEXT_ROWS - 1][x] = ' ';
    }

    redraw_all();
}

static void do_new_line(void)
{
    char_posx = 0;
    char_posy++;

    if (char_posy >= TEXT_ROWS)
    {
        char_posy = TEXT_ROWS - 1;
        scroll_up();
    }
}

static void delete_char_at_cursor(void)
{
    uint16_t x;

    for (x = char_posx; x < (TEXT_COLS - 1); x++)
    {
        put_cell(x, char_posy, screen[char_posy][x + 1]);
    }

    put_cell(TEXT_COLS - 1, char_posy, ' ');
}

static void navigate_right(void)
{
    if (char_posx < (TEXT_COLS - 1))
    {
        char_posx++;
    }
}

static void navigate_left(bool_t is_delete)
{
    if (char_posx > 0)
    {
        char_posx--;

        if (is_delete == TRUE)
        {
            delete_char_at_cursor();
        }
    }
}

static void vt100_J_command(uint16_t param)
{
    switch (param)
    {
        case NEG_16:
        case 2:
            clear_screen();
            break;

        default:
            break;
    }
}

static void vt100_K_command(uint16_t param)
{
    switch (param)
    {
        case NEG_16:
        case 0:
        case 2:
            clear_line(char_posy);
            char_posx = 0;
            break;

        default:
            break;
    }
}

static void vt100_handle_keys(char command)
{
    switch (command)
    {
        case 'C':
            navigate_right();
            break;

        case 'D':
            navigate_left(FALSE);
            break;

        case '~':
            delete_char_at_cursor();
            break;

        default:
            break;
    }
}

static void set_reset_option(uint16_t index, bool_t state)
{
    switch (index)
    {
        case 25:
            cursor_enabled = state;
            break;

        case 47:
            scroll_enabled = state;
            break;

        default:
            break;
    }
}

static void ascii_ctrl_write(char c_in)
{
    if (c_in == '\n')
    {
        if (scroll_enabled == TRUE)
        {
            do_new_line();
        }
    }
    else if (c_in == '\r')
    {
        char_posx = 0;
    }
    else
    {
        if (c_in < ' ')
        {
            put_cell(char_posx, char_posy, '?');
        }
        else
        {
            put_cell(char_posx, char_posy, c_in);
        }

        char_posx++;

        if (char_posx >= TEXT_COLS)
        {
            do_new_line();
        }
    }
}

void vdp9918_vt100_write(char * data)
{
    uint8_t mode = VT100_MODE_IDLE;
    uint8_t prev_mode = VT100_MODE_IDLE;

    char * p_data = data;

    uint16_t param0 = NEG_16;
    uint16_t param1 = NEG_16;
    uint16_t * p_param = &param0;

    bool_t exec_vt100 = FALSE;

    hide_cursor();

    while (*p_data != 0)
    {
        exec_vt100 = FALSE;

        if ((mode != VT100_MODE_IDLE) && (mode != VT100_MODE_DIGITS) && (IS_DIGIT(*p_data)))
        {
            prev_mode = mode;
            mode = VT100_MODE_DIGITS;
        }

        switch (mode)
        {
            case VT100_MODE_IDLE:
                if (*p_data == 0x1b)
                {
                    mode = VT100_MODE_ESCAPE;
                }
                else if (*p_data == 0x08)
                {
                    navigate_left(TRUE);
                }
                else
                {
                    ascii_ctrl_write(*p_data);
                }
                break;

            case VT100_MODE_ESCAPE:
                if (*p_data == '[')
                {
                    mode = VT100_MODE_BRACKET;
                    param0 = NEG_16;
                    param1 = NEG_16;
                    is_set_reset = FALSE;
                    p_param = &param0;
                }
                else
                {
                    mode = VT100_MODE_IDLE;
                }
                break;

            case VT100_MODE_BRACKET:
                if (*p_data == '?')
                {
                    is_set_reset = TRUE;
                }
                else if (*p_data == ';')
                {
                    p_param = &param1;
                }
                else if (strchr(vt100_cmds, *p_data) != NULL)
                {
                    exec_vt100 = TRUE;
                    mode = VT100_MODE_IDLE;
                }
                break;

            case VT100_MODE_DIGITS:
                if (IS_DIGIT(*p_data))
                {
                    if (*p_param == NEG_16)
                    {
                        *p_param = 0;
                    }
                    *p_param = (*p_param * 10) + (*p_data & 0x0F);
                }
                else
                {
                    mode = prev_mode;
                    continue;
                }
                break;
        }

        if (exec_vt100 == TRUE)
        {
            switch (*p_data)
            {
                case 'H':
                    char_posy = (param0 == NEG_16) ? 0 : param0;
                    char_posx = (param1 == NEG_16) ? 0 : param1;
                    if (char_posy >= TEXT_ROWS) char_posy = TEXT_ROWS - 1;
                    if (char_posx >= TEXT_COLS) char_posx = TEXT_COLS - 1;
                    break;

                case 'l':
                    if (is_set_reset == TRUE)
                    {
                        set_reset_option(param0, FALSE);
                    }
                    break;

                case 'h':
                    if (is_set_reset == TRUE)
                    {
                        set_reset_option(param0, TRUE);
                    }
                    break;

                case 'K':
                    vt100_K_command(param0);
                    break;

                case 'J':
                    vt100_J_command(param0);
                    break;

                case 'A':
                case 'B':
                case 'C':
                case 'D':
                    vt100_handle_keys(*p_data);
                    break;

                case '~':
                    if (param0 == 3)
                    {
                        vt100_handle_keys(*p_data);
                    }
                    break;

                case 'm':
                default:
                    break;
            }
        }

        p_data++;
    }

    show_cursor();
}

void vdp9918_vt100_init(void)
{
    vdp9918_text_mode_init();

    cursor_enabled = TRUE;
    scroll_enabled = TRUE;
    is_set_reset = FALSE;
    cursor_drawn = FALSE;

    clear_screen();
    show_cursor();
}
