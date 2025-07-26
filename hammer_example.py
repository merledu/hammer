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
    for i in range(350):
        sim.single_step(0)
        pc  = sim.get_PC(0)
        a0  = sim.get_gpr(0, 10)   # x10/a0
        print(f"Step {i+1:02d}: pc={pc:#018x}  a0={a0:#018x}")

if __name__ == "__main__":
    main()
