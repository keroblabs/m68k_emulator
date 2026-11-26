#include "keypad_driver.h"
#include "timer.h"
#include "buffer.h"
#include "memory_map.h"

///////////////////////////////////////////////////////////////////////////

#define KEYPAD_WAIT_MS      10
#define KEYPAD_DEBOUCE_MS   10
#define KEYPAD_BUFFER_SIZE  16

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

///////////////////////////////////////////////////////////////////////////

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

    EVENT_KEY_ASCII = 1000,

    EVENT_KEY_SHIFT_L = 2000,
    EVENT_KEY_SHIFT_R,
    EVENT_KEY_CAPS,
    EVENT_KEY_CTRL_L,
    EVENT_KEY_CTRL_R,
    EVENT_KEY_ALT,
    EVENT_KEY_ALTGR,
    EVENT_KEY_MENU,
    EVENT_KEY_WIN,
    EVENT_KEY_FN,
    EVENT_KEY_F1,
    EVENT_KEY_F2,
    EVENT_KEY_F3,
    EVENT_KEY_F4,
    EVENT_KEY_F5,
    EVENT_KEY_F6,
    EVENT_KEY_F7,
    EVENT_KEY_F8,
    EVENT_KEY_F9,
    EVENT_KEY_F10,
    EVENT_KEY_F11,
    EVENT_KEY_F12,
    EVENT_KEY_PTRSC,
    EVENT_KEY_PAUSE,
    EVENT_KEY_INS,
};

enum
{
    STATE_IDLE,
    STATE_SEND_COL,
    STATE_WAIT,
    STATE_GET_ROW,
    STATE_CHECK_ROW,
};

typedef struct key_to_vt100_t_
{
    uint16_t event;
    const char * vt100;

} key_to_vt100_t;

typedef struct row_to_key_t_
{
    uint32_t code;
    uint32_t code_shift;
    uint32_t code_fn;

} row_to_key_t;

typedef struct col_key_t_
{
    row_to_key_t row_key[16];
    
} col_key_t;

///////////////////////////////////////////////////////////////////////////

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

static const col_key_t matrix_to_key[] = 
{
    {   { EVENT_KEY_SHIFT_L,    0, 0 },
        { EVENT_KEY_CAPS,       0, 0 },
        { EVENT_KEY_CTRL_L,     0, 0 },
        { EVENT_KEY_ALT,        0, 0 },

        { EVENT_KEY_ALTGR,      0, 0 },
        { EVENT_KEY_WIN,        0, 0 },
        { EVENT_KEY_ESC,        0, 0 },
        { EVENT_KEY_TAB,        0, 0 },

        { EVENT_KEY_ENTER,      0, 0 },
        { EVENT_KEY_BACKSPACE,  0, 0 },
        { EVENT_KEY_DELETE,     0, 0 },
        { EVENT_KEY_PAGEDOWN,   0, 0 },

        { EVENT_KEY_PAGEUP,     0, 0 },
        { EVENT_KEY_END,        0, 0 },
        { EVENT_KEY_HOME,       0, 0 },
        { EVENT_KEY_FN,         0, 0 }, },

    {   { EVENT_KEY_UP,         0, 0 },
        { EVENT_KEY_DOWN,       0, 0 },
        { EVENT_KEY_RIGHT,      0, 0 },
        { EVENT_KEY_LEFT,       0, 0 }, 

        { EVENT_KEY_ASCII + '1', EVENT_KEY_ASCII + '!', 0 },
        { EVENT_KEY_ASCII + '2', EVENT_KEY_ASCII + '\"', 0 },
        { EVENT_KEY_ASCII + '3', EVENT_KEY_ASCII + '3', 0 },
        { EVENT_KEY_ASCII + '4', EVENT_KEY_ASCII + '$', 0 },

        { EVENT_KEY_ASCII + '5', EVENT_KEY_ASCII + '%', 0 },
        { EVENT_KEY_ASCII + '6', EVENT_KEY_ASCII + '^', 0 },
        { EVENT_KEY_ASCII + '7', EVENT_KEY_ASCII + '&', 0 },
        { EVENT_KEY_ASCII + '8', EVENT_KEY_ASCII + '*', 0 },

        { EVENT_KEY_ASCII + '9', EVENT_KEY_ASCII + '(', 0 },
        { EVENT_KEY_ASCII + '0', EVENT_KEY_ASCII + ')', 0 },
        { EVENT_KEY_ASCII + '-', EVENT_KEY_ASCII + '_', 0 },
        { EVENT_KEY_ASCII + '=', EVENT_KEY_ASCII + '+', 0 }, },

    {   { EVENT_KEY_ASCII + 'q', EVENT_KEY_ASCII + 'Q', 0 },
        { EVENT_KEY_ASCII + 'w', EVENT_KEY_ASCII + 'W', 0 },
        { EVENT_KEY_ASCII + 'e', EVENT_KEY_ASCII + 'E', 0 },
        { EVENT_KEY_ASCII + 'r', EVENT_KEY_ASCII + 'R', 0 },

        { EVENT_KEY_ASCII + 't', EVENT_KEY_ASCII + 'T', 0 },
        { EVENT_KEY_ASCII + 'y', EVENT_KEY_ASCII + 'Y', 0 },
        { EVENT_KEY_ASCII + 'u', EVENT_KEY_ASCII + 'U', 0 },
        { EVENT_KEY_ASCII + 'i', EVENT_KEY_ASCII + 'I', 0 },

        { EVENT_KEY_ASCII + 'o', EVENT_KEY_ASCII + 'O', 0 },
        { EVENT_KEY_ASCII + 'p', EVENT_KEY_ASCII + 'P', 0 },
        { EVENT_KEY_ASCII + '[', EVENT_KEY_ASCII + '{', 0 },
        { EVENT_KEY_ASCII + ']', EVENT_KEY_ASCII + '}', 0 },

        { EVENT_KEY_ASCII + '\'', EVENT_KEY_ASCII + '@', 0 },
        { EVENT_KEY_ASCII + ';', EVENT_KEY_ASCII + ':', 0 },
        { EVENT_KEY_ASCII + '#', EVENT_KEY_ASCII + '~', 0 },
        { EVENT_KEY_ASCII + '/', EVENT_KEY_ASCII + '?', 0 }, },

    { EVENT_KEY_ASCII + '1', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '1', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '`',  EVENT_KEY_ASCII + '`', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },
    { EVENT_KEY_ASCII + '\\', EVENT_KEY_ASCII + '!', 0 },

    { 0, 0, 0 },
};

///////////////////////////////////////////////////////////////////////////

static uint8_t state;
static uint8_t curr_col;
static uint8_t temp_row;

static uint8_t keypad_buffer_data[KEYPAD_BUFFER_SIZE];

static timer_data_t keypad_timer;
static buffer_data_t keypad_buffer;

static volatile uint8_t * gpio_col;
static volatile uint8_t * gpio_row;

///////////////////////////////////////////////////////////////////////////

void keypad_driver_init(void)
{
    state = STATE_IDLE;
    curr_col = 0;

    gpio_col = (volatile uint8_t * )GPIO_OUTPUT_ADDR;
    gpio_row = (volatile uint8_t * )GPIO_INPUT_ADDR;

    buffer_init(&keypad_buffer, keypad_buffer_data, KEYPAD_BUFFER_SIZE);
}

void keypad_driver_process(void)
{
    switch (state)
    {
        case STATE_IDLE:
            *gpio_col = curr_col;
            curr_col++;
            state = STATE_WAIT;
            timer_Start(&keypad_timer, KEYPAD_WAIT_MS);
            break;

        case STATE_WAIT:
            if (timer_Check(&keypad_timer) == TRUE)
            {
                state = STATE_GET_ROW;
                temp_row = *gpio_row;
                timer_Start(&keypad_timer, KEYPAD_DEBOUCE_MS);
            }
            break;

        case STATE_GET_ROW:
            if (timer_Check(&keypad_timer) == TRUE)
            {
                state = STATE_IDLE;
                if (temp_row == *gpio_row)
                {

                }
            }
            break;
    }
}
