#ifndef _keyboard_matrix_driver_h_
#define _keyboard_matrix_driver_h_

#include "types.h"

/*
 * Scanned keyboard matrix (16 rows x 8 columns).
 *
 * The matrix is driven through the YM2149 Port A and the first MC68681:
 *   - YM2149 IOA3..IOA6 -> 4-to-16 decoder selects one of 16 rows.
 *   - YM2149 IOA0..IOA2 -> 8-to-1 mux selects one of 8 columns.
 *   - MC68681 (device 0) IP5 returns the selected cell.
 *
 * keyboard_matrix_process() must be called frequently from the main loop; it
 * scans one row per tick (a full 16-row scan takes 16 ticks).  A keypress is
 * accepted only when two consecutive full scans agree (debounce).  Decoded
 * input events are queued and retrieved through keyboard_matrix_get_event().
 */
void keyboard_matrix_init(void);
void keyboard_matrix_process(void);

/*
 * Pops one decoded input event produced by the scan.  Special keys carry an
 * INPUT_EVENT_KEY_* code (key = 0); character keys carry INPUT_EVENT_KEYPRESS
 * with the decoded byte in key.  Returns FALSE when no event is queued.
 */
bool_t keyboard_matrix_get_event(uint16_t * event, char * key);

#endif /* _keyboard_matrix_driver_h_ */
