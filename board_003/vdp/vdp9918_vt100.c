#include "vdp9918_vt100.h"
#include "vdp9918_driver.h"
#include "vdp9918_types.h"
//#include "vga_driver.h"
//#include "draw.h"

//////////////////////////////////////////////////////////////////////////////////////

#define IS_DIGIT(a)         ((a >= '0') && (a <= '9'))

#define UNPRINTABLE_COLOUR  TMS_COLOUR_MED_RED
#define TEXT_COLOUR         TMS_COLOUR_WHITE
#define BACKGROUND_COLOUR   TMS_COLOUR_BLACK
#define CURSOR_COLOUR       TMS_COLOUR_LT_GREEN

#define MAX_LINE_WIDTH      100

#define NEG_16              ((uint16_t)-1)

//#define GR_DRAW_CHAR        vdp9918_text_char_out

//////////////////////////////////////////////////////////////////////////////////////

enum
{
    VT100_MODE_IDLE,
    VT100_MODE_ESCAPE,
    VT100_MODE_BRACKET,
    VT100_MODE_DIGITS,
};

typedef struct temp_lcd_char_t_
{
    uint8_t fore_colour;
    uint8_t back_colour;
    char data;

} temp_lcd_char_t;

//////////////////////////////////////////////////////////////////////////////////////

static uint16_t char_posx;
static uint16_t char_posy;

static uint16_t char_width;
static uint16_t char_height;

static uint16_t char_x_count;
static uint16_t char_y_count;

//static uint16_t lcd_posx;
//static uint16_t lcd_posy;

static uint16_t lcd_width;
static uint16_t lcd_height;

static bool_t cursor_enabled;
static bool_t scroll_enabled;
static bool_t is_set_reset = FALSE;

static uint8_t vt100_colour_foreground;
static uint8_t vt100_colour_background;

static temp_lcd_char_t temp_lcd_line[MAX_LINE_WIDTH];

//static bool_t char_colour;

// \x1b[%dm - set colour
// \x1b[7m - text inverse
// \x1b[m  - normal colour
// \x1b[K  - clear line (?)
// \x1b[2J - clear screen

// \x1b[H      - cursor at up corner
// \x1b[%d;%dH - set cursor
// \x1b[?25l   - hide cursor
// \x1b[?25h   - show cursoe

static const char vt100_cmds[] = "mKJHlhABCD~";

//////////////////////////////////////////////////////////////////////////////////////

static void clear_char_lcd_line(void)
{
    uint16_t counter;

    for (counter = 0; counter < MAX_LINE_WIDTH; counter++)
    {
        temp_lcd_line[counter].data = ' ';
        temp_lcd_line[counter].fore_colour = TEXT_COLOUR;
        temp_lcd_line[counter].back_colour = BACKGROUND_COLOUR;
    }

    vdp9918_text_clear_line(0, char_posy);
}

static void show_char_lcd_line(char c_in, uint8_t fore_colour, uint8_t back_colour)
{
    if (char_posx < char_x_count)
    {
        vdp9918_text_char_out(c_in, char_posx, char_posy, fore_colour, back_colour, FALSE);

        temp_lcd_line[char_posx].data = c_in;
        temp_lcd_line[char_posx].fore_colour = fore_colour;
        temp_lcd_line[char_posx].back_colour = back_colour;
    }
}

static void update_char_lcd_line(void)
{
    uint16_t counter;
    uint16_t posx = 0;

    for (counter = 0; counter < MAX_LINE_WIDTH; counter++)
    {
        if (posx >= lcd_width)
        {
            break;
        }

        vdp9918_text_char_out(temp_lcd_line[ counter ].data, 
                              posx, char_posy, 
                              temp_lcd_line[ counter ].fore_colour, temp_lcd_line[ counter ].back_colour, FALSE);

        posx += char_width;
    }
}

static void clear_curr_line(void)
{
    lcd_posx = 0;
    clear_char_lcd_line();
}

static void do_new_line(void)
{
    lcd_posy += char_height;
    char_posy++;

    if (lcd_posy >= lcd_height)
    {
        lcd_posy = lcd_height - char_height;
        char_posy--;

        vga_do_scroll(char_height);
    }

    char_posx = 0;

    clear_curr_line();
}

static void delete_char_at_cursor(void)
{
    memcpy(&temp_lcd_line[char_posx], &temp_lcd_line[char_posx + 1], sizeof(temp_lcd_char_t) * (MAX_LINE_WIDTH - char_posx - 1));
    update_char_lcd_line();
}

static void navigate_right(void)
{
    if (lcd_posx < (lcd_width - 1))
    {
        if (cursor_enabled == TRUE)
        {
            show_char_lcd_line(temp_lcd_line[ char_posx ].data, vt100_colour_foreground, vt100_colour_background);
        }

        lcd_posx += char_width;
        char_posx++;

        if (cursor_enabled == TRUE)
        {
            show_char_lcd_line(temp_lcd_line[ char_posx ].data, BACKGROUND_COLOUR, CURSOR_COLOUR);
        }
    }    
}

static void navigate_left(bool_t is_delete)
{
    if (lcd_posx > 0)
    {
        if (cursor_enabled == TRUE)
        {
            show_char_lcd_line(temp_lcd_line[ char_posx ].data, vt100_colour_foreground, vt100_colour_background);
        }

        lcd_posx -= char_width;
        char_posx--;

        if (is_delete == TRUE)
        {
            delete_char_at_cursor();
        }

        if (cursor_enabled == TRUE)
        {
            show_char_lcd_line(temp_lcd_line[ char_posx ].data, BACKGROUND_COLOUR, CURSOR_COLOUR);
        }
    }
}

static uint8_t set_vt100_colour(uint8_t vt_100_colour)
{
    uint8_t colour;

    switch(vt_100_colour)
    {
        case 30: colour = TMS_COLOUR_BLACK; break; //  APP_RGB(0,0,0); break;
        case 31: colour = TMS_COLOUR_MED_RED; break; //  APP_RGB(205,0,0); break;
        case 32: colour = TMS_COLOUR_LT_GREEN; break; //  APP_RGB(0,205,0); break;
        case 33: colour = TMS_COLOUR_LT_YELLOW; break; //  APP_RGB(252,127,0); break;
        case 34: colour = TMS_COLOUR_DK_BLUE; break; //  APP_RGB(0,0,238); break;
        case 35: colour = TMS_COLOUR_CYAN; break; //  APP_RGB(156,0,156); break;
        case 36: colour = TMS_COLOUR_DK_BLUE; break; //  APP_RGB(0,205,205); break;
        case 37: colour = TMS_COLOUR_WHITE; break; //  APP_RGB(229,229,229); break;

        default: colour = TMS_COLOUR_MED_RED; break;
    }

    return colour;
}

static void vt100_J_command(uint16_t param)
{
    switch (param)
    {
        case NEG_16:
        case 2:
            // clear the screen
            draw_clear_screen();
            break;

        case 0:
            // TODO; erase from cursor down
            break;

        case 1:
            // TODO: erase from cursor up
            break;
    }
}

static void vt100_K_command(uint16_t param)
{
    switch (param)
    {
        case NEG_16:
        case 2:
            // clear current line
            clear_curr_line();
            break;

        case 0:
            // TODO; clear line from cursor to end
            break;

        case 1:
            // TODO: clear line from beginning to cursor
            break;
    }
}

static void vt100_m_command(uint16_t param)
{
    if (param == 0)
    {
        vt100_colour_foreground = TEXT_COLOUR;
        vt100_colour_background = BACKGROUND_COLOUR;
    }
    else if (param == 7)
    {
        vt100_colour_foreground = BACKGROUND_COLOUR;
        vt100_colour_background = TEXT_COLOUR;
    }
    else
    {
        vt100_colour_foreground = set_vt100_colour(param);
        vt100_colour_background = BACKGROUND_COLOUR;
    }
}

static void vt100_handle_keys(char command)
{
    switch (command)
    {
        case 'A':
            break;

        case 'B':
            break;

        case 'C':
            // right key
            navigate_right();
            break;

        case 'D':
            // left key
            navigate_left(FALSE);
            break;

        case '~':
            // delete key
            delete_char_at_cursor();
            break;
    }
}

static void set_reset_option(uint16_t index, bool_t state)
{
    switch(index)
    {
        case 25:
            // cursor
            cursor_enabled = state;
            break;

        case 9:
            break;

        case 47:
            // raw mode
            scroll_enabled = state;
            break;
    }
}

static void ascii_ctrl_write(char c_in)
{
    if (c_in == '\n')
    {
        if (cursor_enabled == TRUE)
        {
            show_char_lcd_line(temp_lcd_line[ char_posx ].data, vt100_colour_foreground, vt100_colour_background);
        }

        if (scroll_enabled == TRUE)
        {
            do_new_line();
        }
    }
    else
    {
        if (c_in < ' ')
        {
            show_char_lcd_line('?', BACKGROUND_COLOUR, UNPRINTABLE_COLOUR);
        }
        else
        {
            show_char_lcd_line(c_in, vt100_colour_foreground, vt100_colour_background);
        }

        lcd_posx += char_width;
        char_posx++;

        if (lcd_posx >= lcd_width)
        {
            do_new_line();
        }
    }

    if (cursor_enabled == TRUE)
    {
        show_char_lcd_line(temp_lcd_line[ char_posx ].data, BACKGROUND_COLOUR, CURSOR_COLOUR);
    }
}

void vdp9918_vt100_write(char * data)
{
    uint8_t mode = VT100_MODE_IDLE;
    uint8_t prev_mode;

    char * p_data = data;

    uint16_t param0;
    uint16_t param1;
    uint16_t * p_param = &param0;

    bool_t exec_vt100 = FALSE;

    while (*p_data != 0)
    {
        if ((mode != VT100_MODE_IDLE ) && (IS_DIGIT(*p_data)))
        {
            prev_mode = mode;
            mode = VT100_MODE_DIGITS;
        }

        switch(mode)
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
                    *p_param = (*p_param * 10) + (*p_data & 0x0F);
                }
                else
                {
                    p_param--;
                    mode = prev_mode;
                }         
                break;       
        }

        if (exec_vt100 == TRUE)
        {
            switch(*p_data)
            {
                case 'm':
                    vt100_m_command(param0);
                    break;

                case 'H':
                    if (param1 != NEG_16)
                    {
                        char_posx = param1;
                        char_posy = param0;
                    }
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

                case 'A': // up
                case 'B': // down
                case 'C': // right
                case 'D': // left
                    vt100_handle_keys(*p_data);
                    break;

                case '~':
                    if (param0 == 3)
                    {
                        vt100_handle_keys(*p_data);
                    }
                    break;
            }
        }

        p_data++;
    }
}

void vdp9918_vt100_init(void)
{
    char_width = draw_get_text_width("?");
    char_height = draw_get_text_height("?");

    lcd_width = vga_get_width();
    lcd_height = vga_get_height();

    char_posx = 0;
    char_posy = 0;

    lcd_posx = 0;
    lcd_posy = 0;

    cursor_enabled = TRUE;
    scroll_enabled = TRUE;

    vt100_colour_foreground = TEXT_COLOUR;
    vt100_colour_background = BACKGROUND_COLOUR;

    clear_char_lcd_line();
    draw_clear_screen();
}

