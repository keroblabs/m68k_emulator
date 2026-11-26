#include "keyboard_matrix_driver.h"
#include "ym2149_driver.h"
#include "m68681_driver.h"
#include "timer.h"
#include "buffer.h"

/*
 * Keyboard matrix decode table (16 rows x 8 columns).  The coordinates MUST
 * match the emulator host adapter in
 * emulator/peripherals/keyboard_matrix_emulator.c.
 *
 *   row = (IOA >> 3) & 0x0F   (4-to-16 decoder)
 *   col =  IOA       & 0x07   (8-to-1 mux)
 */

#define KB_ROWS                 16u
#define KB_COLS                 8u

/*
 * Debounce control.  Define KB_USE_DEBOUNCE to 0 (e.g. -DKB_USE_DEBOUNCE=0) to
 * accept each full scan immediately for the lowest possible input latency; a
 * single clean scan then registers the key.  With debounce enabled (default) a
 * key must read pressed in two consecutive full scans before it is accepted.
 */
#ifndef KB_USE_DEBOUNCE
#define KB_USE_DEBOUNCE         0
#endif

/*
 * One row is scanned per timer tick, so a full 16-row scan takes
 * KB_SCAN_PERIOD_MS * 16.  With debounce enabled a key must read pressed in two
 * consecutive full scans to be accepted, giving a debounce window of
 * (period * 16 * 2).  At 2 ms/row that is a 32 ms full scan and ~64 ms
 * debounce, which keeps typing responsive while still rejecting single-scan
 * glitches.
 */
#define KB_SCAN_PERIOD_MS       10u
#define KB_KEYPAD_DEVICE        M68681_DEVICE_0

/* Modifier cells on row 15 (shared with the emulator layout). */
#define KB_MOD_ROW              15u
#define KB_MOD_SHIFT_MASK       (1u << 0)
#define KB_MOD_CTRL_MASK        (1u << 1)
#define KB_MOD_ALT_MASK         (1u << 2)
#define KB_MOD_CAPS_MASK        (1u << 3)

#define KB_RX_BUFFER_SIZE       32u

typedef struct kb_key_t_
{
    uint8_t row;
    uint8_t col;
    uint8_t normal;      /* ASCII when no modifier; 0 for pure sequence keys. */
    uint8_t shifted;     /* ASCII when Shift is held. */
    const char * seq;    /* VT100 sequence for special keys, NULL otherwise. */
} kb_key_t;

static const kb_key_t kb_keys[] =
{
    /* Row 0 */
    { 0, 0, 0x1B, 0x1B, "\x1b" },
    { 0, 1, '1', '!', 0 },
    { 0, 2, '2', '@', 0 },
    { 0, 3, '3', '#', 0 },
    { 0, 4, '4', '$', 0 },
    { 0, 5, '5', '%', 0 },
    { 0, 6, '6', '^', 0 },
    { 0, 7, '7', '&', 0 },

    /* Row 1 */
    { 1, 0, '8', '*', 0 },
    { 1, 1, '9', '(', 0 },
    { 1, 2, '0', ')', 0 },
    { 1, 3, '-', '_', 0 },
    { 1, 4, '=', '+', 0 },
    { 1, 5, 0x7F, 0x7F, 0 },      /* Backspace */
    { 1, 6, '\t', '\t', 0 },      /* Tab */
    { 1, 7, 'q', 'Q', 0 },

    /* Row 2 */
    { 2, 0, 'w', 'W', 0 },
    { 2, 1, 'e', 'E', 0 },
    { 2, 2, 'r', 'R', 0 },
    { 2, 3, 't', 'T', 0 },
    { 2, 4, 'y', 'Y', 0 },
    { 2, 5, 'u', 'U', 0 },
    { 2, 6, 'i', 'I', 0 },
    { 2, 7, 'o', 'O', 0 },

    /* Row 3 */
    { 3, 0, 'p', 'P', 0 },
    { 3, 1, '[', '{', 0 },
    { 3, 2, ']', '}', 0 },
    { 3, 3, '\\', '|', 0 },
    { 3, 4, 0x0D, 0x0D, 0 },      /* Enter */
    { 3, 5, 'a', 'A', 0 },
    { 3, 6, 's', 'S', 0 },
    { 3, 7, 'd', 'D', 0 },

    /* Row 4 */
    { 4, 0, 'f', 'F', 0 },
    { 4, 1, 'g', 'G', 0 },
    { 4, 2, 'h', 'H', 0 },
    { 4, 3, 'j', 'J', 0 },
    { 4, 4, 'k', 'K', 0 },
    { 4, 5, 'l', 'L', 0 },
    { 4, 6, ';', ':', 0 },
    { 4, 7, '\'', '"', 0 },

    /* Row 5 */
    { 5, 0, 'z', 'Z', 0 },
    { 5, 1, 'x', 'X', 0 },
    { 5, 2, 'c', 'C', 0 },
    { 5, 3, 'v', 'V', 0 },
    { 5, 4, 'b', 'B', 0 },
    { 5, 5, 'n', 'N', 0 },
    { 5, 6, 'm', 'M', 0 },
    { 5, 7, ',', '<', 0 },

    /* Row 6 */
    { 6, 0, '.', '>', 0 },
    { 6, 1, '/', '?', 0 },
    { 6, 2, ' ', ' ', 0 },
    { 6, 3, '`', '~', 0 },
    { 6, 4, 0, 0, "\x1b[A" },     /* Up */
    { 6, 5, 0, 0, "\x1b[B" },     /* Down */
    { 6, 6, 0, 0, "\x1b[C" },     /* Right */
    { 6, 7, 0, 0, "\x1b[D" },     /* Left */

    /* Row 7 */
    { 7, 0, 0, 0, "\x1b[1~" },    /* Home */
    { 7, 1, 0, 0, "\x1b[4~" },    /* End */
    { 7, 2, 0, 0, "\x1b[3~" },    /* Delete */
    { 7, 3, 0, 0, "\x1b[5~" },    /* Page Up */
    { 7, 4, 0, 0, "\x1b[6~" },    /* Page Down */
    { 7, 5, 0, 0, "\x1b[2~" },    /* Insert */
    { 7, 6, 0, 0, "\x1bOP" },     /* F1 */
    { 7, 7, 0, 0, "\x1bOQ" },     /* F2 */

    /* Row 8 */
    { 8, 0, 0, 0, "\x1bOR" },     /* F3 */
    { 8, 1, 0, 0, "\x1bOS" },     /* F4 */
    { 8, 2, 0, 0, "\x1b[15~" },   /* F5 */
    { 8, 3, 0, 0, "\x1b[17~" },   /* F6 */
    { 8, 4, 0, 0, "\x1b[18~" },   /* F7 */
    { 8, 5, 0, 0, "\x1b[19~" },   /* F8 */
    { 8, 6, 0, 0, "\x1b[20~" },   /* F9 */
    { 8, 7, 0, 0, "\x1b[21~" },   /* F10 */

    /* Row 9 */
    { 9, 0, 0, 0, "\x1b[23~" },   /* F11 */
    { 9, 1, 0, 0, "\x1b[24~" },   /* F12 */
};

#define KB_KEY_COUNT    (sizeof(kb_keys) / sizeof(kb_keys[0]))

static uint8_t kb_line_16;
static uint8_t kb_scan[KB_ROWS];     /* Matrix being accumulated this pass.   */
static uint8_t kb_prev[KB_ROWS];     /* Previous completed pass (debounce).   */
static uint8_t kb_stable[KB_ROWS];   /* Last debounced (accepted) state.      */
static uint8_t kb_caps_lock;

static timer_data_t kb_timer;

static buffer_data_t kb_rx_buffer_data;
static uint8_t kb_rx_buffer[KB_RX_BUFFER_SIZE];

static void kb_output_select(uint8_t row, uint8_t col)
{
    /* YM2149 Port A: IOA3..IOA6 = row decoder, IOA0..IOA2 = column mux. */
    ym2149_port_write(YM2149_PORT_A, (uint8_t)(((row & 0x0Fu) << 3) | (col & 0x07u)));
}

static bool_t kb_input_sense(void)
{
    return ((m68681_read_input_port(KB_KEYPAD_DEVICE) & M68681_IP_BIT_5) != 0) ? TRUE : FALSE;
}

static const kb_key_t * kb_lookup(uint8_t row, uint8_t col)
{
    uint16_t i;

    for (i = 0; i < KB_KEY_COUNT; i++)
    {
        if ((kb_keys[i].row == row) && (kb_keys[i].col == col))
        {
            return &kb_keys[i];
        }
    }

    return NULL;
}

static void kb_emit_key(const kb_key_t * key, uint8_t shift, uint8_t ctrl)
{
    if (key->seq != NULL)
    {
        const char * p = key->seq;
        while (*p != '\0')
        {
            buffer_write(&kb_rx_buffer_data, (uint8_t)*p);
            p++;
        }
        return;
    }

    uint8_t c = shift ? key->shifted : key->normal;

    /* Caps Lock inverts the case of alphabetic keys only. */
    if (kb_caps_lock)
    {
        if ((c >= 'a') && (c <= 'z'))
        {
            c = (uint8_t)(c - 0x20);
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            c = (uint8_t)(c + 0x20);
        }
    }

    /* Ctrl maps letters to their control code. */
    if (ctrl && (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))))
    {
        c = (uint8_t)(c & 0x1Fu);
    }

    buffer_write(&kb_rx_buffer_data, c);
}

static void kb_accept_scan(const uint8_t * debounced)
{
    uint8_t shift = ((debounced[KB_MOD_ROW] & KB_MOD_SHIFT_MASK) != 0) ? 1u : 0u;
    uint8_t ctrl  = ((debounced[KB_MOD_ROW] & KB_MOD_CTRL_MASK) != 0) ? 1u : 0u;
    uint8_t row;

    /* Caps Lock toggles on a fresh press of its cell. */
    if (((debounced[KB_MOD_ROW] & KB_MOD_CAPS_MASK) != 0) &&
        ((kb_stable[KB_MOD_ROW] & KB_MOD_CAPS_MASK) == 0))
    {
        kb_caps_lock ^= 1u;
    }

    for (row = 0; row < KB_ROWS; row++)
    {
        /* Newly pressed cells since the last accepted state. */
        uint8_t pressed = (uint8_t)(debounced[row] & ~kb_stable[row]);
        uint8_t col;

        if (row == KB_MOD_ROW)
        {
            continue;   /* Modifiers do not emit characters. */
        }

        if (pressed == 0)
        {
            continue;
        }

        for (col = 0; col < KB_COLS; col++)
        {
            if (((pressed >> col) & 0x01u) != 0)
            {
                const kb_key_t * key = kb_lookup(row, col);
                if (key != NULL)
                {
                    kb_emit_key(key, shift, ctrl);
                }
            }
        }
    }

    for (row = 0; row < KB_ROWS; row++)
    {
        kb_stable[row] = debounced[row];
    }
}

static void kb_scan_row(void)
{
    uint8_t read = 0;
    uint8_t i;

    for (i = 0; i < KB_COLS; i++)
    {
        kb_output_select(kb_line_16, i);
        if (kb_input_sense() == TRUE)
        {
            read |= (uint8_t)(1u << i);
        }
    }

    kb_scan[kb_line_16] = read;
    kb_line_16++;

    if (kb_line_16 >= KB_ROWS)
    {
        uint8_t debounced[KB_ROWS];
        uint8_t row;

        kb_line_16 = 0;

#if KB_USE_DEBOUNCE
        /*
         * Per-key debounce: a cell counts as pressed only when it reads
         * pressed in this scan AND the previous full scan.  Debouncing each
         * cell independently means a change on one key never blocks the
         * acceptance of another (important while typing or rolling keys).
         */
        for (row = 0; row < KB_ROWS; row++)
        {
            debounced[row] = (uint8_t)(kb_scan[row] & kb_prev[row]);
        }
#else
        /* No debounce: accept the raw scan immediately (lowest latency). */
        for (row = 0; row < KB_ROWS; row++)
        {
            debounced[row] = kb_scan[row];
        }
#endif

        kb_accept_scan(debounced);

        for (row = 0; row < KB_ROWS; row++)
        {
            kb_prev[row] = kb_scan[row];
        }
    }
}

void keyboard_matrix_init(void)
{
    uint8_t row;

    kb_line_16 = 0;
    kb_caps_lock = 0;

    for (row = 0; row < KB_ROWS; row++)
    {
        kb_scan[row] = 0;
        kb_prev[row] = 0;
        kb_stable[row] = 0;
    }

    buffer_init(&kb_rx_buffer_data, kb_rx_buffer, KB_RX_BUFFER_SIZE);

    /* YM2149 Port A drives the scan lines (output). */
    ym2149_init();
    ym2149_port_configure(YM2149_PORT_A, YM2149_PORT_OUTPUT);

    timer_Start(&kb_timer, KB_SCAN_PERIOD_MS);
}

void keyboard_matrix_process(void)
{
    if (timer_Check(&kb_timer) == TRUE)
    {
        timer_Reload(&kb_timer);
        kb_scan_row();
    }
}

bool_t keyboard_matrix_rx_pending(void)
{
    return (buffer_is_empty(&kb_rx_buffer_data) == FALSE) ? TRUE : FALSE;
}

uint8_t keyboard_matrix_read(void)
{
    return buffer_read(&kb_rx_buffer_data);
}
