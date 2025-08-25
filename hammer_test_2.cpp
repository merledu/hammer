// Simple C++ test equivalent to hammer_example.py
// Takes any ELF file as input

#include "hammer.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <your_program.elf>\n", argv[0]);
        return 1;
    }
    
    const std::string elf_path = argv[1];
    printf("Loading ELF: %s\n", elf_path.c_str());
    
    // Setup memory like Python example
    std::vector<mem_cfg_t> memory_layout;
    memory_layout.push_back(mem_cfg_t(reg_t(DRAM_BASE), reg_t(256 * 1024 * 1024)));
    
    std::vector<size_t> hart_ids{0};
    
    // Create Hammer with RV32IMC like Python
    Hammer sim("RV32IMC", "msu", "", hart_ids, memory_layout, elf_path, std::nullopt);
    
    printf("Initial PC: 0x%lx\n", (sim.get_PC(0) & 0xFFFFFFFF));
    
    // Step through instructions like Python example
    for (int i = 0; i < 300; ++i) {
        reg_t pc = sim.get_PC(0) & 0xFFFFFFFF;
        insn_bits_t insn_hex = sim.get_insn_hex(0, pc);
        std::string insn_str = sim.get_insn_string(0, pc);
        
        sim.single_step(0);
        
        printf("Step %02d: pc=0x%lx insn=0x%lx\n", i+1, pc, (unsigned long)insn_hex);
        printf("  %s\n", insn_str.c_str());
        
        // Check register writes
        auto reg_writes = sim.get_log_reg_writes(0);
        if (!reg_writes.empty()) {
            printf("=== REGISTER WRITES ===\n");
            for (const auto& [reg, value] : reg_writes) {
                printf("WRITE REG: %s, VALUE: 0x%lx\n", reg.c_str(), value);
            }
        }
        
        // Check memory writes  
        auto mem_writes = sim.get_log_mem_writes(0);
        if (!mem_writes.empty()) {
            printf("=== MEMORY WRITES ===\n");
            for (const auto& [addr, value, size] : mem_writes) {
                printf("WRITE ADDR: 0x%lx, VALUE: 0x%lx, SIZE: %zu\n", 
                       addr & 0xFFFFFFFF, value, (size_t)size);
            }
        }
        
        // Check target memory like Python
        auto memory_contents = sim.get_memory_at_VA<uint8_t>(0, 0x80002000, 4);
        uint32_t value_at_address = 0;
        if (memory_contents.has_value()) {
            auto& bytes = memory_contents.value();
            for (size_t j = 0; j < bytes.size(); ++j) {
                value_at_address |= (bytes[j] << (j * 8));
            }
        }
        
        if (value_at_address == 1) {
            printf("Reached target memory location, stopping simulation.\n");
            break;
        }
    }
    
    printf("Test completed!\n");
    return 0;
}
