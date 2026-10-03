#include <unistd.h>
#include "uart_driver.h"

int _write(int file, const char *ptr, int len) {
    // Check if the file descriptor is stdout or stderr
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        return len;
    }
    return -1; // Indicate error for other file descriptors
}

// Minimal _sbrk_r for heap management
extern char _heap_end; // Symbol provided by your linker script
static char *heap_end = &_heap_end;

void *_sbrk_r(int incr) {
    char *prev_heap_end = heap_end;
    heap_end += incr;
    return (void *)prev_heap_end;
}
