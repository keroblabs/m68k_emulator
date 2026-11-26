def generate_irq_handlers():
    """
    Generates a C file with a list of interrupt handler functions.
    
    The functions are named irq_n<n> where n goes from 0 to 7.
    They call intr_vector_list[<m>](), where m goes from 24 to 31.
    """
    
    filename = "../boot/irq_vectors.s"
    
    with open(filename, 'w') as f:
        f.write("    .section .handlers_asm, \"ax\"\n\n")

        for n in range(8):
            f.write(f"    .global irq_n{n}_asm\n")
        f.write("\n")
        
        for n in range(8):
            f.write(f"    .extern irq_n{n}\n")
        f.write("\n")

        for n in range(16):
            f.write(f"    .global trap_n{n}_asm\n")
        f.write("\n")
        
        for n in range(16):
            f.write(f"    .extern trap_n{n}\n")
        f.write("\n")

        for n in range(192):
            f.write(f"    .global user_n{n}_asm\n")
        f.write("\n")
        
        for n in range(192):
            f.write(f"    .extern user_n{n}\n")
        f.write("\n")

        for n in range(8):
            f.write(f"irq_n{n}_asm:\n")
            f.write(f"    movem.l %d0-%d7,-(%a7)\n")
            f.write(f"    movem.l %a0-%a6,-(%a7)\n")
            f.write(f"    bsr irq_n{n}\n")
            f.write(f"    movem.l (%a7)+,%a0-%a6\n")
            f.write(f"    movem.l (%a7)+,%d0-%d7\n")
            f.write(f"    rte\n\n")
        f.write("\n")

        for n in range(16):
            f.write(f"trap_n{n}_asm:\n")
            f.write(f"    movem.l %d0-%d7,-(%a7)\n")
            f.write(f"    movem.l %a0-%a6,-(%a7)\n")
            f.write(f"    bsr trap_n{n}\n")
            f.write(f"    movem.l (%a7)+,%a0-%a6\n")
            f.write(f"    movem.l (%a7)+,%d0-%d7\n")
            f.write(f"    rte\n\n")
        f.write("\n")

        for n in range(16):
            f.write(f"user_n{n}_asm:\n")
            f.write(f"    movem.l %d0-%d7,-(%a7)\n")
            f.write(f"    movem.l %a0-%a6,-(%a7)\n")
            f.write(f"    bsr user_n{n}\n")
            f.write(f"    movem.l (%a7)+,%a0-%a6\n")
            f.write(f"    movem.l (%a7)+,%d0-%d7\n")
            f.write(f"    rte\n\n")
        f.write("\n")

    print(f"Successfully generated {filename}")

if __name__ == "__main__":
    generate_irq_handlers()
