#include <SDL2/SDL.h>
#include "keyboard_matrix_emulator.h"
#include "ym2149_emulator.h"
#include "m68681_emulator.h"

/*
 * Keyboard matrix layout (16 rows x 8 columns).  The coordinates below MUST
 * match the firmware decode table in board_004/hardware/keyboard_matrix_driver.c
 * so the emulated scan and the firmware translation agree.
 *
 *   row = (IOA >> 3) & 0x0F   (4-to-16 decoder)
 *   col =  IOA       & 0x07   (8-to-1 mux)
 *
 * MC68681 IP5 reads 1 when the selected cell is pressed.
 */
#define KB_IP5_BIT              (1u << 5)

#define KB_ROWS                 16u
#define KB_COLS                 8u

/*
 * Minimum time (ms) a cell is held pressed after the host SDL_KEYUP arrives.
 *
 * The firmware scans one matrix row per timer tick, so a full 16-row scan
 * takes tens of milliseconds and, with debounce, a key must be seen pressed on
 * two consecutive full scans.  A quick physical tap on the host can be shorter
 * than that window, so the cell would be released before the firmware ever
 * samples it.  Latching the release for this many milliseconds guarantees the
 * firmware has enough scan passes to sense the key.
 */
#define KB_RELEASE_HOLD_MS      120u

/* Modifier cells live on row 15 (shared by both sides). */
#define KB_MOD_ROW              15u
#define KB_MOD_SHIFT_COL        0u
#define KB_MOD_CTRL_COL         1u
#define KB_MOD_ALT_COL          2u
#define KB_MOD_CAPS_COL         3u

typedef struct kb_entry_t_
{
    int sym;
    uint8_t row;
    uint8_t col;
} kb_entry_t;

static const kb_entry_t kb_table[] =
{
    /* Row 0 */
    { SDLK_ESCAPE,       0, 0 },
    { SDLK_1,            0, 1 },
    { SDLK_2,            0, 2 },
    { SDLK_3,            0, 3 },
    { SDLK_4,            0, 4 },
    { SDLK_5,            0, 5 },
    { SDLK_6,            0, 6 },
    { SDLK_7,            0, 7 },

    /* Row 1 */
    { SDLK_8,            1, 0 },
    { SDLK_9,            1, 1 },
    { SDLK_0,            1, 2 },
    { SDLK_MINUS,        1, 3 },
    { SDLK_EQUALS,       1, 4 },
    { SDLK_BACKSPACE,    1, 5 },
    { SDLK_TAB,          1, 6 },
    { SDLK_q,            1, 7 },

    /* Row 2 */
    { SDLK_w,            2, 0 },
    { SDLK_e,            2, 1 },
    { SDLK_r,            2, 2 },
    { SDLK_t,            2, 3 },
    { SDLK_y,            2, 4 },
    { SDLK_u,            2, 5 },
    { SDLK_i,            2, 6 },
    { SDLK_o,            2, 7 },

    /* Row 3 */
    { SDLK_p,            3, 0 },
    { SDLK_LEFTBRACKET,  3, 1 },
    { SDLK_RIGHTBRACKET, 3, 2 },
    { SDLK_BACKSLASH,    3, 3 },
    { SDLK_RETURN,       3, 4 },
    { SDLK_a,            3, 5 },
    { SDLK_s,            3, 6 },
    { SDLK_d,            3, 7 },

    /* Row 4 */
    { SDLK_f,            4, 0 },
    { SDLK_g,            4, 1 },
    { SDLK_h,            4, 2 },
    { SDLK_j,            4, 3 },
    { SDLK_k,            4, 4 },
    { SDLK_l,            4, 5 },
    { SDLK_SEMICOLON,    4, 6 },
    { SDLK_QUOTE,        4, 7 },

    /* Row 5 */
    { SDLK_z,            5, 0 },
    { SDLK_x,            5, 1 },
    { SDLK_c,            5, 2 },
    { SDLK_v,            5, 3 },
    { SDLK_b,            5, 4 },
    { SDLK_n,            5, 5 },
    { SDLK_m,            5, 6 },
    { SDLK_COMMA,        5, 7 },

    /* Row 6 */
    { SDLK_PERIOD,       6, 0 },
    { SDLK_SLASH,        6, 1 },
    { SDLK_SPACE,        6, 2 },
    { SDLK_BACKQUOTE,    6, 3 },
    { SDLK_UP,           6, 4 },
    { SDLK_DOWN,         6, 5 },
    { SDLK_RIGHT,        6, 6 },
    { SDLK_LEFT,         6, 7 },

    /* Row 7 */
    { SDLK_HOME,         7, 0 },
    { SDLK_END,          7, 1 },
    { SDLK_DELETE,       7, 2 },
    { SDLK_PAGEUP,       7, 3 },
    { SDLK_PAGEDOWN,     7, 4 },
    { SDLK_INSERT,       7, 5 },
    { SDLK_F1,           7, 6 },
    { SDLK_F2,           7, 7 },

    /* Row 8 */
    { SDLK_F3,           8, 0 },
    { SDLK_F4,           8, 1 },
    { SDLK_F5,           8, 2 },
    { SDLK_F6,           8, 3 },
    { SDLK_F7,           8, 4 },
    { SDLK_F8,           8, 5 },
    { SDLK_F9,           8, 6 },
    { SDLK_F10,          8, 7 },

    /* Row 9 */
    { SDLK_F11,          9, 0 },
    { SDLK_F12,          9, 1 },

    /* Row 15: modifiers */
    { SDLK_LSHIFT,       KB_MOD_ROW, KB_MOD_SHIFT_COL },
    { SDLK_RSHIFT,       KB_MOD_ROW, KB_MOD_SHIFT_COL },
    { SDLK_LCTRL,        KB_MOD_ROW, KB_MOD_CTRL_COL  },
    { SDLK_RCTRL,        KB_MOD_ROW, KB_MOD_CTRL_COL  },
    { SDLK_LALT,         KB_MOD_ROW, KB_MOD_ALT_COL   },
    { SDLK_RALT,         KB_MOD_ROW, KB_MOD_ALT_COL   },
    { SDLK_CAPSLOCK,     KB_MOD_ROW, KB_MOD_CAPS_COL  },
};

#define KB_TABLE_COUNT   (sizeof(kb_table) / sizeof(kb_table[0]))

/* One bit per column; a set bit means the cell is currently pressed. */
static uint8_t kb_matrix[KB_ROWS];

/*
 * Pending delayed release.  A set bit means the cell received an SDL_KEYUP but
 * is being held pressed until its release tick (below) elapses.
 */
static uint8_t kb_release_pending[KB_ROWS];
static uint32_t kb_release_tick[KB_ROWS][KB_COLS];

/* Last YM2149 Port A value latched by the firmware scan. */
static uint8_t kb_scan_select;

static void km_ym_port_write(void * p_data, uint8_t data)
{
    (void)p_data;
    kb_scan_select = data;
}

static uint8_t km_m68681_ip_read(void * p_data)
{
    uint8_t row = (uint8_t)((kb_scan_select >> 3) & 0x0Fu);
    uint8_t col = (uint8_t)(kb_scan_select & 0x07u);

    (void)p_data;

    if (((kb_matrix[row] >> col) & 0x01u) != 0)
    {
        return KB_IP5_BIT;
    }

    return 0;
}

void keyboard_matrix_emulator_init(void * p_ym2149, void * p_duart0)
{
    uint8_t row;

    for (row = 0; row < KB_ROWS; row++)
    {
        kb_matrix[row] = 0;
        kb_release_pending[row] = 0;
    }
    kb_scan_select = 0;

    /* YM2149 Port A is an output (scan select); sense arrives on MC68681 IP5. */
    ym2149_emulator_set_port_callbacks(p_ym2149, YM2149_EMULATOR_PORT_A,
                                       NULL, km_ym_port_write, NULL);
    m68681_emulator_set_port_callbacks(p_duart0, km_m68681_ip_read, NULL, NULL);
}

void keyboard_matrix_emulator_key_event(int sdl_sym, bool_t pressed)
{
    uint32_t i;

    for (i = 0; i < KB_TABLE_COUNT; i++)
    {
        if (kb_table[i].sym == sdl_sym)
        {
            uint8_t row = kb_table[i].row;
            uint8_t col = kb_table[i].col;
            uint8_t mask = (uint8_t)(1u << col);

            if (pressed == TRUE)
            {
                /* Press immediately and cancel any pending delayed release. */
                kb_matrix[row] |= mask;
                kb_release_pending[row] &= (uint8_t)~mask;
            }
            else
            {
                /*
                 * Defer the release so the firmware's periodic scan has time to
                 * sense the key.  The cell stays pressed until the hold time
                 * elapses in keyboard_matrix_emulator_process().
                 */
                kb_release_pending[row] |= mask;
                kb_release_tick[row][col] = SDL_GetTicks() + KB_RELEASE_HOLD_MS;
            }
            return;
        }
    }
}

void keyboard_matrix_emulator_process(void)
{
    uint32_t now = SDL_GetTicks();
    uint8_t row;

    for (row = 0; row < KB_ROWS; row++)
    {
        uint8_t pending = kb_release_pending[row];
        uint8_t col;

        if (pending == 0)
        {
            continue;
        }

        for (col = 0; col < KB_COLS; col++)
        {
            uint8_t mask = (uint8_t)(1u << col);

            if (((pending & mask) != 0) &&
                ((int32_t)(now - kb_release_tick[row][col]) >= 0))
            {
                kb_matrix[row] &= (uint8_t)~mask;
                kb_release_pending[row] &= (uint8_t)~mask;
            }
        }
    }
}

