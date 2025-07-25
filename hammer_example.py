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

    # Instantiate Spike via Hammer
    sim = hammer.Hammer(
        isa="RV32IMC",
        privilege_levels="msu",
        vector_arch="",
        hart_ids=[0],
        memory_layout=[(0x80000000, 256 * 1024 * 1024)],  # 256 MiB @ DRAM base
        target_binary=elf
    )

    print("Initial PC:", hex(sim.get_PC(0)))

    # Step through first 10 instructions
    for i in range(10):
        sim.single_step(0)
        pc  = sim.get_PC(0)
        a0  = sim.get_gpr(0, 10)   # x10/a0
        print(f"Step {i+1:02d}: pc={pc:#018x}  a0={a0:#018x}")

if __name__ == "__main__":
    main()
