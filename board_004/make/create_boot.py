def generate_irq_handlers():
    """
    Generates a C file with a list of interrupt handler functions.
    
    The functions are named irq_n<n> where n goes from 0 to 7.
    They call intr_vector_list[<m>](), where m goes from 24 to 31.
    """
    
    filename = "../boot/irq_boot.s"
    
    with open(filename, 'w') as f:
        f.write("    .align 2\n")
        f.write("    .section .vectors, \"a\"\n\n")

        f.write("    .long 0x8FFFE\n")
        f.write("    .long _start\n")
        f.write("    .long exception_bus_error\n")
        f.write("    .long exception_addr_error\n")
        f.write("    .long exception_illegal_inst\n")
        f.write("    .long exception_div_zero\n")
        f.write("    .long exception_chk\n")
        f.write("    .long exception_trapv\n")
        f.write("    .long exception_priv_violation\n")
        f.write("    .long 0\n")
        f.write("    .long exception_unimp_inst\n")
        f.write("    .long exception_unimp_inst\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long exception_uninit_int_vector\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")

        for n in range(8):
            f.write(f"    .long irq_n{n}_asm\n")
        f.write("\n")
        
        for n in range(16):
            f.write(f"    .long trap_n{n}_asm\n")
        f.write("\n")

        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")
        f.write("    .long 0\n")

        for n in range(16):
            f.write(f"    .long user_n{n}_asm\n")
        f.write("\n")

        for n in range(176):
            f.write(f"    .long user_interrupt\n")
        f.write("\n")

    print(f"Successfully generated {filename}")

if __name__ == "__main__":
    generate_irq_handlers()
