// SPDX-FileCopyrightText: 2022 Rivos Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "hammer.h"

/**
 * Simple test similar to hammer_example.py - takes any ELF as input
 */
void test_hammer_simple(const std::string& elf_path) {
    printf("=== SIMPLE HAMMER TEST ===\n");
    printf("Loading ELF: %s\n", elf_path.c_str());
    
    // Configure memory layout - using RV32IMC like Python example
    std::vector<mem_cfg_t> memory_layout;
    memory_layout.push_back(mem_cfg_t(reg_t(DRAM_BASE), reg_t(256 * 1024 * 1024)));
    
    std::vector<size_t> hart_ids{0};
    
    // Create Hammer instance with RV32IMC like Python example
    Hammer sim("RV32IMC", "msu", "", hart_ids, memory_layout, elf_path, std::nullopt);
    
    printf("Initial PC: 0x%lx\n", (sim.get_PC(0) & 0xFFFFFFFF));
    
    // Step through first 300 instructions like Python example
    for (int i = 0; i < 300; ++i) {
        reg_t pc = sim.get_PC(0) & 0xFFFFFFFF;  // RV32IMC, mask to 32 bits
        insn_bits_t insn_hex = sim.get_insn_hex(0, pc);
        std::string insn_str = sim.get_insn_string(0, pc);
        
        // Get instruction details (matching Python example)
        uint64_t rs1 = sim.get_rs1_addr(0, pc);
        uint64_t rs2 = sim.get_rs2_addr(0, pc);
        uint64_t rs3 = sim.get_rs3_addr(0, pc);
        uint64_t rd = sim.get_rd_addr(0, pc);
        uint64_t csr_addr = sim.get_csr_addr(0, pc);
        
        auto csr_val = sim.get_csr(0, csr_addr);
        int length = sim.get_insn_length(0, pc);
        bool enable = sim.get_log_commits_enabled(0);
        
        // Execute the instruction
        sim.single_step(0);
        
        reg_t a0 = sim.get_gpr(0, 10);  // x10/a0
        
        // Check memory at specific address like Python example
        auto val_a = sim.get_memory_at_VA<uint8_t>(0, 0x8000bc48, 4);
        uint32_t val = 0;
        if (val_a.has_value()) {
            auto& bytes = val_a.value();
            for (size_t j = 0; j < bytes.size(); ++j) {
                val |= (bytes[j] << (j * 8));
            }
        }
        
        printf("Step %02d: pc=0x%lx insn=0x%lx\n", 
               i+1, pc, (uint64_t)insn_hex);
        printf("  insn_str: %s\n", insn_str.c_str());
        
        // Log register writes
        auto reg_writes = sim.get_log_reg_writes(0);
        if (!reg_writes.empty()) {
            printf("=== REGISTER WRITES ===\n");
            for (const auto& reg_write : reg_writes) {
                printf("WRITE REG: %s, VALUE: 0x%lx\n", 
                       reg_write.first.c_str(), reg_write.second);
            }
        }
        
        // Log memory reads
        auto mem_reads = sim.get_log_mem_reads(0);
        if (!mem_reads.empty() && mem_reads.size() > 4) {
            printf("=== MEMORY READS ===\n");
            auto first_read = mem_reads[0];  // Just first one like Python
            uint64_t addr = std::get<0>(first_read) & 0xFFFFFFFF;  // Mask to 32 bits
            uint64_t value = std::get<1>(first_read);
            size_t size = std::get<2>(first_read);
            printf("READ ADDRESS: 0x%lx, VALUE: 0x%lx, SIZE: %zu bytes\n", 
                   addr, value, size);
        }
        
        // Log memory writes
        auto mem_writes = sim.get_log_mem_writes(0);
        if (!mem_writes.empty()) {
            printf("=== MEMORY WRITES ===\n");
            for (const auto& mem_write : mem_writes) {
                uint64_t addr = std::get<0>(mem_write) & 0xFFFFFFFF;  // Mask to 32 bits
                uint64_t value = std::get<1>(mem_write);
                size_t size = std::get<2>(mem_write);
                printf("WRITE ADDRESS: 0x%lx, VALUE: 0x%lx, SIZE: %zu bytes\n", 
                       addr, value, size);
            }
        }
        
        // Check target memory location like Python example
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
    
    printf("Simple test completed.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <your_program.elf>\n", argv[0]);
        printf("Example: %s /path/to/your/program.elf\n", argv[0]);
        return 1;
    }
    
    const std::string elf_path = argv[1];
    
    try {
        // Run simple test with your ELF
        test_hammer_simple(elf_path);
        
        printf("\nTest completed successfully!\n");
        
    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
