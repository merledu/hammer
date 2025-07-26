import hammer, pathlib, sys

# print(dir(hammer))
# elf = sys.argv[1]
elf = "/home/nitin/cocotb_simulation/top_tracing_simulation/ibex_arithmetic_basic_test_0.o"

# Build one mem_cfg_t object – (base, size) just like the C++ struct
mem_cfg = hammer.mem_cfg_t(0x8000_0000, 256 * 1024 * 1024)   # 256 MiB DRAM

# print(dir(hammer.Hammer))

#               0           1           2       3       4           5     [6]
sim = hammer.Hammer("RV32IMC", "msu",   "",   [0],  [mem_cfg],elf,None) 
#                ↑          ↑           ↑     ↑       ↑           ↑     ↑
#                isa   privilege_levels  vlen hartIDs  mem_layout  ELF  (start_pc optional)

# print("Initial PC:", hex(sim.get_PC(0)))
# for i in range(10):
#     sim.single_step(0)
#     print(f"{i+1:02}: pc={hex(sim.get_PC(0))}")
