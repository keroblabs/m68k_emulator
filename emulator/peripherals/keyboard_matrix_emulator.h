#ifndef _keyboard_matrix_emulator_h_
#define _keyboard_matrix_emulator_h_

#include "types.h"

/*
 * Keyboard-matrix host adapter.
 *
 * Models a 16x8 key matrix wired through the YM2149 Port A and the first
 * MC68681:
 *   - YM2149 IOA3..IOA6 drive a 4-to-16 decoder selecting one of 16 rows.
 *   - YM2149 IOA0..IOA2 drive an 8-to-1 mux selecting one of 8 columns.
 *   - The selected cell is returned on MC68681 IP5 (device 0 input port).
 *
 * p_ym2149 is a YM2149 device handle; p_duart0 is the first MC68681 device
 * handle.  The adapter installs the YM2149 Port A write callback (scan select)
 * and the MC68681 input-port read callback (IP5 sense).
 */
void keyboard_matrix_emulator_init(void * p_ym2149, void * p_duart0);

/* Updates the matrix from an SDL keyboard event (SDL_Keycode sym). */
void keyboard_matrix_emulator_key_event(int sdl_sym, bool_t pressed);

/*
 * Services deferred key releases.  Must be called periodically (e.g. once per
 * SDL event-loop iteration) so keys latched past their SDL_KEYUP are released
 * once their hold time elapses.
 */
void keyboard_matrix_emulator_process(void);

#endif /* _keyboard_matrix_emulator_h_ */
