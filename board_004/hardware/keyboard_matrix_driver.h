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
 * scans one row every 10 ms (a full 16-row scan therefore takes 160 ms).  A
 * keypress is accepted only when two consecutive full scans agree (debounce).
 * Decoded VT100 bytes are queued and retrieved through the rx API below.
 */
void keyboard_matrix_init(void);
void keyboard_matrix_process(void);

bool_t keyboard_matrix_rx_pending(void);
uint8_t keyboard_matrix_read(void);

#endif /* _keyboard_matrix_driver_h_ */
