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

    mem_cfg = hammer.mem_cfg_t(hammer.DramBase, 256 * 1024 * 1024)

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
    for i in range(300):
        hart=0

        pc  = sim.get_PC(0) & 0xFFFFFFFF  # RV32IMC, so we mask to 32 bits
        insn_hex = sim.get_insn_hex(0, pc)
        insn_str = sim.get_insn_string(0, pc)
        # print(dir(insn_str))
        rs1 = sim.get_rs1_addr(0, pc)
        rs2 = sim.get_rs2_addr(0, pc)
        rs3 = sim.get_rs3_addr(0, pc)
        rd  = sim.get_rd_addr(0, pc)
        csr = sim.get_csr_addr(0, pc)
        csr_val = sim.get_csr(0, csr)
        length = sim.get_insn_length(0, pc)
        enable = sim.get_log_commits_enabled(0)
        # Handle optional CSR value (None if invalid CSR)
        if csr_val is None:
            csr_val = "N/A"

        sim.single_step(0)
        

        a0  = sim.get_gpr(0, 10)   # x10/a0
        val_a = sim.get_memory_at_VA(0, 0x8000bc48, 4, 1)
        val = 0
        if val_a is not None:
            for byte_value in val_a:
                val |= (byte_value << (val_a.index(byte_value) * 8))
        
        print(f"Step {i+1:02d}: pc={pc:#x} insn={insn_hex:#x}")
        print(f"  insn_str: {insn_str}")
        reg_writes=sim.get_log_reg_writes(0)  # Dump the state of hart 0
        if reg_writes:
            print("=== REGISTER WRITES ===")
            for reg, value in reg_writes:
                print(f"WRITE REG: {reg}, VALUE: {value:#x}")

        mem_reads=sim.get_log_mem_reads(0)
        if mem_reads and len(mem_reads) > 4:
            print("=== MEMORY READS ===")
            for addr, value, size in mem_reads :
                addr &= 0xFFFFFFFF  # Mask to 32 bits
                print(f"READ ADDRESS: {addr:#x}, VALUE: {value:#x}, SIZE: {size} bytes")
                break
        mem_writes=sim.get_log_mem_writes(0)
        if mem_writes:
            print("=== MEMORY WRITES ===")
            for addr, value, size in mem_writes:
                addr &= 0xFFFFFFFF  # Mask to 32 bits
                print(f"WRITE ADDRESS: {addr:#x}, VALUE: {value:#x}, SIZE: {size} bytes")
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
