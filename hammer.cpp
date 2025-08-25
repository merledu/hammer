// SPDX-FileCopyrightText: 2022 Rivos Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "hammer.h"

#include "fesvr/option_parser.h"
#include "riscv/cachesim.h"
#include "riscv/decode.h"

#include <vector>


// FIXME: This function exists in Spike as a static function. We shouldn't have to
// copy it out here but sim_t requires it as an argument.
static std::vector<std::pair<reg_t, abstract_mem_t*>> make_mems(const std::vector<mem_cfg_t> &layout) {
  std::vector<std::pair<reg_t, abstract_mem_t*>> mems;
  mems.reserve(layout.size());
  for (const auto &cfg : layout) {
    mems.push_back(std::make_pair(cfg.get_base(), new mem_t(cfg.get_size())));
  }
  return mems;
}

Hammer::Hammer(const char *isa, const char *privilege_levels, const char *vector_arch,
               std::vector<size_t> hart_ids, std::vector<mem_cfg_t> memory_layout,
               const std::string target_binary, const std::optional<uint64_t> start_pc) {
  // Expose these only if needed
  std::vector<std::pair<reg_t, abstract_device_t *>> plugin_devices;
  std::vector<std::pair<const device_factory_t*, std::vector<std::string>>> plugin_device_factories;

  debug_module_config_t dm_config = {.progbufsize = 2,
                                     .max_sba_data_width = 0,
                                     .require_authentication = false,
                                     .abstract_rti = 0,
                                     .support_hasel = true,
                                     .support_abstract_csr_access = true,
                                     .support_haltgroups = true,
                                     .support_impebreak = true};
  const char *log_path = "hammer.log";
  const char *dtb_file = nullptr;
  FILE *cmd_file = nullptr;

  // std::pair<reg_t, reg_t> initrd_bounds{0, 0};
  // const char *bootargs = nullptr;
  // bool real_time_clint = false;
  // bool misaligned = false;
  // bool explicit_hartids =false;

  // reg_t trigger_count = 4;

  // reg_t num_pmpregions = 16;

  // reg_t pmpgranularity   = (1 << PMP_SHIFT);

  // reg_t cache_blocksz = 64;

  // endianness_t endinaness = endianness_little;

  // cfg_t cfg = cfg_t(initrd_bounds, bootargs, isa, privilege_levels, vector_arch, misaligned, endinaness, num_pmpregions, memory_layout,
  //                   hart_ids, real_time_clint, trigger_count);

  cfg_t cfg; // Spike Commit 6023896 revised the cfg_t class
  // cfg.initrd_bounds    = initrd_bounds;
  // cfg.bootargs         = bootargs;
  cfg.isa              = isa;
  cfg.priv             = privilege_levels;
  cfg.misaligned       = true; 
  // cfg.endianness       = endinaness;
  // cfg.pmpregions       = num_pmpregions;
  // cfg.pmpgranularity   = pmpgranularity;
  cfg.mem_layout       = memory_layout;
  cfg.hartids          = hart_ids;
  // cfg.explicit_hartids = explicit_hartids;
  // cfg.real_time_clint  = real_time_clint;
  // cfg.trigger_count    = trigger_count;
  // cfg.cache_blocksz    = cache_blocksz;


  if (start_pc.has_value()) {
    cfg.start_pc = start_pc.value();
  }

  std::vector<std::pair<reg_t, abstract_mem_t*>> mems = make_mems(memory_layout);

  std::vector<std::string> htif_args;
  htif_args.push_back(target_binary);

  bool halted = false;
  bool dtb_enabled = true;
  bool socket_enabled = false;
  std::optional<unsigned long long> instructions = std::nullopt;

  // simulator = new sim_t(&cfg, halted, mems, plugin_devices, htif_args, dm_config, log_path,
  //                       dtb_enabled, dtb_file, socket_enabled, cmd_file);
  simulator = new sim_t(&cfg, halted, mems, plugin_device_factories, htif_args, dm_config, log_path,
                        dtb_enabled, dtb_file, socket_enabled, cmd_file,instructions);

  // Initializes everything
  bool enable_log=false;
  bool enable_commitlog=true;
  simulator->configure_log(enable_log,enable_commitlog);
  simulator->start();
}

Hammer::~Hammer() { delete simulator; }

bool Hammer::get_log_commits_enabled(uint8_t hart_id){
  processor_t *hart = simulator->get_core(hart_id);
  return hart->get_log_commits_enabled();
}

// Instruction metadata
insn_fetch_t Hammer::get_insn_fetch(uint8_t hart_id,reg_t pc){
  processor_t *hart = simulator->get_core(hart_id);
  mmu_t *mmu = hart->get_mmu();
  insn_fetch_t fetch = mmu->load_insn(pc);
  return fetch;
}

std::string Hammer::get_insn_string(uint8_t hart_id,reg_t pc) {
  processor_t *hart = simulator->get_core(hart_id);
  mmu_t *mmu = hart->get_mmu();
  insn_fetch_t fetch = mmu->load_insn(pc);
  insn_t insn = fetch.insn;
  const disassembler_t *disasm = hart->get_disassembler();
  std::string insn_str = disasm->disassemble(insn);
  return insn_str;
}

insn_t Hammer::get_insn(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn;
}
insn_bits_t Hammer::get_insn_hex(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.bits();
}
int Hammer::get_insn_length(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.length();
}
uint64_t Hammer::get_opcode(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.opcode();
}

uint64_t Hammer::get_rs1_addr(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.rs1();
}
uint64_t Hammer::get_rs2_addr(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.rs2();
}
uint64_t Hammer::get_rs3_addr(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.rs3();
}
uint64_t Hammer::get_rd_addr(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.rd();
}
uint64_t Hammer::get_csr_addr(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.csr();
}

// RVC instructions metadata
uint64_t Hammer::get_rvc_opcode(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.rvc_opcode();
}
uint64_t Hammer::get_rvc_rs1_addr(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.rvc_rs1();
}

uint64_t Hammer::get_rvc_rs2_addr(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.rvc_rs2();
}

uint64_t Hammer::get_rvc_rd_addr(uint8_t hart_id,reg_t pc){
  return get_insn_fetch(hart_id,pc).insn.rvc_rd();
}

std::vector<std::pair<std::string, uint64_t>> Hammer::get_log_reg_writes(uint8_t hart_id){
  processor_t *hart = simulator->get_core(hart_id);
  commit_log_reg_t reg = hart->get_state()->log_reg_write;
  int xlen = hart->get_state()->last_inst_xlen;
  int flen = hart->get_state()->last_inst_flen;
  bool show_vec = false;
  
  std::vector<std::pair<std::string, uint64_t>> result;
  
  for (auto item : reg) {
    if (item.first == 0)
      continue;

    char prefix = ' ';
    int size;
    int rd = item.first >> 4;
    bool is_vec = false;
    bool is_vreg = false;
    
    // Determine register type and size (same logic as Spike)
    switch (item.first & 0xf) {
    case 0:
      size = xlen;
      prefix = 'x';
      break;
    case 1:
      size = flen;
      prefix = 'f';
      break;
    case 2:
      size = hart->VU.get_vlen();
      prefix = 'v';
      is_vreg = true;
      break;
    case 3:
      is_vec = true;
      break;
    case 4:
      size = xlen;
      prefix = 'c';
      break;
    default:
      continue; // Skip unknown register types
    }
    // NOT RELEVANT FOR RV32IMC
    if (!show_vec && (is_vreg || is_vec)) {
        // log write 
        show_vec = true;
    }

    if (!is_vec) {
      // Create register name
      std::string reg_name = std::string(1, prefix) + std::to_string(rd);
      
      // Extract the integer value directly from the register data
      uint64_t int_value = 0;
      if (size <= 64) {
        // For 32-bit and 64-bit registers, use v[0]
        int_value = item.second.v[0];
        if (size == 32) {
          // For 32-bit registers, mask to 32 bits
          int_value &= 0xFFFFFFFF;
        }
      } else {
        // For larger registers (like 128-bit), combine v[0] and v[1]
        int_value = item.second.v[0] | (static_cast<uint64_t>(item.second.v[1]) << 32);
      }
      
      // std::cout<<"Reg"<<" "<<reg_name<<" "<<"Value:"<<std::hex<<"0x"<<int_value<<std::dec<<std::endl;
      result.emplace_back(reg_name, int_value);
    }
  }
  
  return result;
}
commit_log_mem_t Hammer::get_log_mem_reads(uint8_t hart_id){
    commit_log_mem_t result;
    processor_t *hart = simulator->get_core(hart_id);
    commit_log_mem_t load = hart->get_state()->log_mem_read;
    
    // if (!load.empty()) {
    //     std::cout << "=== MEMORY READS for PC: 0x" << std::hex << get_PC(hart_id) << std::dec << " ===" << std::endl;
    // }
    
    //address,data,size
    for (auto item : load) {
      // item is a tuple: <address, value, size>
      reg_t addr = std::get<0>(item);
      uint64_t value = std::get<1>(item);
      uint8_t size = std::get<2>(item);
      
      // std::cout<<std::hex<<"ADDR 0x"<<addr<<" value 0x"<<value<<" size "<<std::dec<<(int)size<<" bytes"<<std::endl;
      // Add to result as a tuple
      result.emplace_back(addr, value, size);
    }
    return result;

}

commit_log_mem_t Hammer::get_log_mem_writes(uint8_t hart_id){
    commit_log_mem_t result;
    processor_t *hart = simulator->get_core(hart_id);
    commit_log_mem_t store = hart->get_state()->log_mem_write;
    
    // if (!store.empty()) {
    //     std::cout << "=== MEMORY WRITES ===" << std::endl;
    // }
    
    //address,data,size
    for (auto item : store) {
      // item is a tuple: <address, value, size>
      reg_t addr = std::get<0>(item);
      uint64_t value = std::get<1>(item);
      uint8_t size = std::get<2>(item);
      
      // std::cout<<std::hex<<"WRITE ADDR 0x"<<addr<<" value 0x"<<value<<" size "<<std::dec<<(int)size<<" bytes"<<std::endl;
      // Add to result as a tuple
      result.emplace_back(addr, value, size);
    }
    return result;
}



reg_t Hammer::get_gpr(uint8_t hart_id, uint8_t gpr_id) {
  assert(gpr_id < NXPR);

  processor_t *hart = simulator->get_core(hart_id);
  state_t *hart_state = hart->get_state();

  return hart_state->XPR[gpr_id];
}

void Hammer::set_gpr(uint8_t hart_id, uint8_t gpr_id, reg_t new_gpr_value) {
  assert(gpr_id < NXPR);

  processor_t *hart = simulator->get_core(hart_id);
  state_t *hart_state = hart->get_state();

  hart_state->XPR.write(gpr_id, new_gpr_value);
}

uint64_t Hammer::get_fpr(uint8_t hart_id, uint8_t fpr_id) {
  assert(fpr_id < NFPR);

  processor_t *hart = simulator->get_core(hart_id);
  state_t *hart_state = hart->get_state();
  freg_t fpr_value = hart_state->FPR[fpr_id];

  return fpr_value.v[0];
}

reg_t Hammer::get_PC(uint8_t hart_id) {
  processor_t *hart = simulator->get_core(hart_id);
  state_t *hart_state = hart->get_state();

  return hart_state->pc;
}

void Hammer::set_PC(uint8_t hart_id, reg_t new_pc_value) {
  processor_t *hart = simulator->get_core(hart_id);
  state_t *hart_state = hart->get_state();

  hart_state->pc = new_pc_value;
}

std::optional<reg_t> Hammer::get_csr(uint8_t hart_id, uint32_t csr_id) {
  processor_t *hart = simulator->get_core(hart_id);
  try {
    return hart->get_csr(csr_id);
  } catch (...) {
    // Return nullopt for invalid/non-existent CSRs
    return std::nullopt;
  }
}

void Hammer::single_step(uint8_t hart_id) {
  processor_t *hart = simulator->get_core(hart_id);
  hart->step(1);
}

uint32_t Hammer::get_flen(uint8_t hart_id) {
  processor_t *hart = simulator->get_core(hart_id);
  return hart->get_flen();
}

reg_t Hammer::get_vlen(uint8_t hart_id) {
  processor_t *hart = simulator->get_core(hart_id);
  return hart->VU.get_vlen();
}

reg_t Hammer::get_elen(uint8_t hart_id) {
  processor_t *hart = simulator->get_core(hart_id);
  return hart->VU.get_elen();
}

std::vector<uint64_t> Hammer::get_vector_reg(uint8_t hart_id, uint8_t vector_reg_id) {
  assert(vector_reg_id < NVPR);

  processor_t *hart = simulator->get_core(hart_id);
  uint32_t vlen = hart->VU.get_vlen();

  std::vector<uint64_t> vector_reg_value;

  for (uint32_t i = 0; i < (vlen / 64); ++i) {
    vector_reg_value.push_back(hart->VU.elt<uint64_t>(vector_reg_id, i));
  }

  return vector_reg_value;
}

