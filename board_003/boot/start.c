#include "start.h"
#include "irq_handlers.h"

// External symbols defined by the linker script
extern uint32_t _bss_start;
extern uint32_t _bss_end;

// main function
extern int main(void);

// Declare some global variables in BSS
volatile int global_var;
static int static_var;

void initialize_bss() {
    uint32_t *bss_start = &_bss_start;
    uint32_t *bss_end = &_bss_end;

    // Zero out the BSS segment
    while (bss_start < bss_end) {
        *bss_start++ = 0;
    }
}

void panic(const char *err)
{
    while (1)
    {
    }
}

// Requires a GCC-compatible C compiler for the 68000

/*
 * set_interrupt_level() - Sets the 68000 interrupt priority mask.
 * @level: The desired interrupt level, a value from 0 to 7.
 *         Level 0 enables all maskable interrupts.
 *         Level 7 disables all maskable interrupts.
 *
 * This function uses inline assembly to write directly to the Status Register (SR).
 * The operation requires the CPU to be in supervisor mode.
 */
void set_interrupt_level(unsigned char level) {
    // Check that the level is within the valid range (0-7)
    if (level > 7) {
        level = 7;
    }

    // Use a temporary variable to hold the new SR value
    unsigned short new_sr;

    // Inline assembly to perform the register manipulation
    // The "cc" clobber informs the compiler that the condition codes are changed.
    __asm__ volatile (
        "move.w  %%sr, %0\n\t"   // Copy the current SR to the C variable 'new_sr'
        "andi.w  #0xF8FF, %0\n\t" // Clear the IPL bits (I2, I1, I0)
        "or.w    %1, %0\n\t"     // Set the new IPL bits from 'new_sr_mask'
        "move.w  %0, %%sr"       // Write the modified value back to the SR
        : "=d" (new_sr)          // Output: 'new_sr' is written by a data register
        : "d" ((unsigned short)level << 8) // Input: 'level' is placed in a data register, shifted
        : "cc"                   // Clobber: Condition codes are modified
    );
}

void __attribute((interrupt)) exception_unhandled()     { panic("Unhandled exception"); }
void __attribute((interrupt)) exception_bus_error()     { panic("Bus error"); }
void __attribute((interrupt)) exception_addr_error()    { panic("Address error"); }
void __attribute((interrupt)) exception_illegal_inst()  { panic("Illegal instruction"); }
void __attribute((interrupt)) exception_div_zero()      { panic("Divide by zero"); }
void __attribute((interrupt)) exception_chk()           { panic("Chk instruction out of bounds"); }
void __attribute((interrupt)) exception_trapv()         { panic("Trap v"); }
void __attribute((interrupt)) exception_priv_violation() { panic("Privilege violation"); }
void __attribute((interrupt)) exception_unimp_inst()    { panic("Unimplemented instruction"); }
void __attribute((interrupt)) exception_uninit_int_vector() { panic("Uninitialized interrupt vector"); }
void __attribute((interrupt)) exception_spurious_intr() { panic("Spurious interrupt"); }
void __attribute((interrupt)) autovector()              { panic("Autovector"); }
void __attribute((interrupt)) trap()                    { panic("Trap"); }
void __attribute((interrupt)) user_interrupt()          { panic("User interrupt"); }

void _start()
{
    global_var = 0;
    intr_handler_init();
    initialize_bss();
    set_interrupt_level(4);

    main();
}
