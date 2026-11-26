#include "cli_input.h"
#include "uart_driver.h"
#include "events_enum.h"
#include "debug.h"

//////////////////////////////////////////////////////////////////////////////////////

#define CTRL_KEY(k) ((k) & 0x1f)

#define DEFAULT_STDIN      UART_KEYPAD

//////////////////////////////////////////////////////////////////////////////////////

static uint16_t input_message;
static char input_char;

//////////////////////////////////////////////////////////////////////////////////////

static uint16_t convert_key_to_event(int c)
{
    switch(c)
    {
        case CTRL_KEY('q'): return INPUT_EVENT_QUIT;
        case CTRL_KEY('s'): return INPUT_EVENT_SAVE;
        case CTRL_KEY('e'): return INPUT_EVENT_FLIP_UPWARDS;
        case CTRL_KEY('d'): return INPUT_EVENT_FLIP_DOWNWARDS;
        case CTRL_KEY('x'): return INPUT_EVENT_CUT_LINE;
        case CTRL_KEY('c'): return INPUT_EVENT_COPY_LINE;
        case CTRL_KEY('v'): return INPUT_EVENT_PASTE_LINE;
        case CTRL_KEY('f'): return INPUT_EVENT_SEARCH;
        case CTRL_KEY('z'): return INPUT_EVENT_UNDO;
        case CTRL_KEY('y'): return INPUT_EVENT_REDO;

        case CTRL_KEY('l'): return INPUT_EVENT_KEY_ESC;
        case CTRL_KEY('h'): return INPUT_EVENT_KEY_BACKSPACE;

        case 127: return INPUT_EVENT_KEY_BACKSPACE;
        case 13 : return INPUT_EVENT_KEY_ENTER;
        
        case '\t': return INPUT_EVENT_KEY_TAB;
        // default: return INPUT_EVENT_KEYPRESS;
    }

    return INPUT_EVENT_KEYPRESS;
}

static int get_key_press(uint8_t uart_index, uint8_t * input, uint8_t size)
{
    uint16_t count = 1000;
    int ret = 0;

    if (input == NULL)
    {
        return (int)uart_driver_is_rx_pending(uart_index);
    }

    while (count)
    {
        if (uart_driver_is_rx_pending(uart_index) == TRUE)
        {
            ret = 1;
            *input = uart_driver_read_data(uart_index);
            break;
        }

        count--;
    }

    return ret;
}

static void curr_iface(uint16_t message, char key)
{
    input_message = message;
    if(message == INPUT_EVENT_KEYPRESS)
    {
        //debug_printf("%d\n", key);
        input_char = key;    
    }
}

uint16_t cli_input_get_event(char * keycode)
{
    uint16_t ret = input_message;
    input_message = INPUT_EVENT_NONE;
    *keycode = input_char;
    return ret;
}

void cli_input_init(void)
{
    uart_driver_init(DEFAULT_STDIN, 19200);
    input_message = INPUT_EVENT_NONE;
}

void cli_input_process(void) 
{
    int nread = 0;
    char c;

    if( get_key_press( DEFAULT_STDIN, NULL, 0 ) == 0 )
    {
        return;
    }

    nread = get_key_press(DEFAULT_STDIN, (uint8_t *)&c, 1);
    if( nread == 0 )
    {
        return;
    }

    // Check escape sequences, if first byte
    // is an escape character then...
    if (c == '\x1b') {
        char seq[3];

        if (get_key_press(DEFAULT_STDIN, (uint8_t *)&seq[0], 1) != 1 ||
            get_key_press(DEFAULT_STDIN, (uint8_t *)&seq[1], 1) != 1) {
            curr_iface(INPUT_EVENT_KEY_ESC, '\x1b');
        } else if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (get_key_press(DEFAULT_STDIN, (uint8_t *)&seq[2], 1) != 1)
                    curr_iface(INPUT_EVENT_KEY_ESC, '\x1b');
                else if (seq[2] == '~') {
                    switch (seq[1]) {
                        // Home and End keys may be sent in many ways depending on the OS
                        // \x1b[1~, \x1b[7~, \x1b[4~, \x1b[8~
                        case '1':
                        case '7':
                            curr_iface(INPUT_EVENT_KEY_HOME, ' ');
                            break;
                        case '4':
                        case '8':
                            curr_iface(INPUT_EVENT_KEY_END, ' ');
                            break;
                        // Del key is sent as \x1b[3~
                        case '3':
                            curr_iface(INPUT_EVENT_KEY_DELETE, ' ');
                            break;
                        // Page Up and Page Down send '\x1b', '[', '5' or '6' and '~'.
                        case '5': 
                            curr_iface(INPUT_EVENT_KEY_PAGEUP, ' ');
                            break;
                        case '6': 
                            curr_iface(INPUT_EVENT_KEY_PAGEDOWN, ' ');
                            break;
                    }
                }
            } else {
                switch (seq[1]) {
                    // Arrow keys send multiple bytes starting with '\x1b', '[''
                    // and followed by an 'A', 'B', 'C' or 'D' depending on which
                    // arrow is pressed.
                    case 'A': 
                        curr_iface(INPUT_EVENT_KEY_UP, ' ');
                        break;
                    case 'B': 
                        curr_iface(INPUT_EVENT_KEY_DOWN, ' ');
                        break;
                    case 'C': 
                        curr_iface(INPUT_EVENT_KEY_RIGHT, ' ');
                        break;
                    case 'D': 
                        curr_iface(INPUT_EVENT_KEY_LEFT, ' ');
                        break;
                    // Home key can also be sent as \x1b[H
                    case 'H': 
                        curr_iface(INPUT_EVENT_KEY_HOME, ' ');
                        break;
                    // End key can also be sent as \x1b[F
                    case 'F': 
                        curr_iface(INPUT_EVENT_KEY_END, ' ');
                        break;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                // Yes, Home key can ALSO be sent as \x1bOH
                case 'H': 
                    curr_iface(INPUT_EVENT_KEY_HOME, ' ');
                    break;
                // And... End key as \x1bOF
                case 'F': 
                    curr_iface(INPUT_EVENT_KEY_END, ' ');
                    break;
            }
        } else {
            curr_iface(INPUT_EVENT_KEY_ESC, '\x1b');
        }
    } else {
        uint16_t event = convert_key_to_event(c);
        curr_iface(event, c);
    }
}
