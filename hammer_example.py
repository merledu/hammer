#!/usr/bin/env python3
"""
Quick sanity script for the Hammer ⇆ Spike wrapper.

Usage:
    python hammer_example.py path/to/program.elf

Make sure:
  * $LD_LIBRARY_PATH contains <SPIKE_PREFIX>/lib
  * The `hammer` extension (built with Meson) is discoverable in PYTHONPATH or installed.
"""

import sys
import hammer

def main():
    if len(sys.argv) < 2:
        print("Usage: python hammer_example.py <prog.elf>")
        sys.exit(1)

    elf = sys.argv[1]
    print(f"Loading ELF: {elf}")

    mem_cfg = hammer.mem_cfg_t(0x80000000, 256 * 1024 * 1024)

    sim = hammer.Hammer(
        "RV32IMC",          # arg0: isa
        "msu",              # arg1: privilege_levels  
        "",                 # arg2: vector_arch
        [0],                # arg3: hart_ids
        [mem_cfg],          # arg4: memory_layout (sequence of mem_cfg_t)
        elf,                # arg5: target_binary
        None                # arg6: start_pc (optional)
    )

    print("Initial PC:", hex(sim.get_PC(0)))

    # Step through first 10 instructions
    for i in range(20):

        pc  = sim.get_PC(0)
        insn_hex = sim.get_insn_hex(0, pc)
        sim.single_step(0)
        a0  = sim.get_gpr(0, 10)   # x10/a0
        val_a = sim.get_memory_at_VA(0, 0x8000bc48, 4, 1)
        val = 0
        if val_a is not None:
            for byte_value in val_a:
                val |= (byte_value << (val_a.index(byte_value) * 8))
        
        print(f"Step {i+1:02d}: pc={pc:#x}  a0={a0:#x} insn={insn_hex:#x}  ")
        memory_contents = sim.get_memory_at_VA(0, 0x80002000, 4, 1)
        value_at_address = 0
        x = 0
        if memory_contents is not None:
            for byte_value in memory_contents:
                value_at_address |= (byte_value << (x * 8))
                x += 1
        if value_at_address==1:
            print("Reached target memory location, stopping simulation.")
            break

if __name__ == "__main__":
    main()
