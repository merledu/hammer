# Documentation on Hammer & Modifying it for Our Use Case

**Date:** July 31, 2025

**Latest Hammer Commit:** 6d9fac96407149a1a533cdd3f63f6bbd96614ffa

Hammer is an infrastructure to drive Spike (the RISC-V ISA Simulator) in co-simulation mode, designed by Rivos. It provides both C++ and Python interfaces to interact with Spike, enabling detailed instruction-level simulation, tracing, and analysis for RISC-V software and hardware development. Hammer is especially useful for co-simulation, debugging, and automated test environments.

The source code is available here: [https://github.com/rivosinc/hammer](https://github.com/rivosinc/hammer)

Hammer leverages `pybind` to provide a Python interface, using Spike’s executables and shared libraries to enable seamless integration between Python and C++. This allows users to script, automate, and analyze RISC-V simulation runs directly from Python, while still having access to the full power of Spike's C++ backend.

`meson` is the build infrastructure used to build the Hammer source code and execute it. This ensures a modern, fast, and reproducible build process.

The main sections in this repo are:

- `hammer.h`  — Contains the Hammer class definition and API
- `hammer.cpp` — Implementation of the Hammer class and core logic
- `hammer_enums.h` — Enumerations for register types, privilege levels, etc.
- `hammer_pybind.cpp` — Python bindings using pybind11

---

### Features

- Single-step and batch instruction execution
- Python and C++ APIs for scripting and automation
- Support for custom extensions and modifications

---

### Build & Installation

Hammer uses `meson` as its build infrastructure.
The build steps can be found at the Hammer repository.
Refer to it [here](https://github.com/rivosinc/hammer/blob/main/README.md)

### Command to Run Tests

To run the tests:

```sh
meson test -C builddir
```

This command compiles the test input files into ELF binaries using a RISC-V toolchain and executes them through the Meson test framework.

## Common Build Errors

## NOTE

The command `meson test -C builddir` has **not been successfully tested** in this environment.

> **This is because it requires:**
>
> - A RISC-V GCC toolchain that supports the `rv64gcv` ISA (including vector instructions).
> - Properly formatted linker scripts that avoid using unsupported syntax (e.g., `#` for comments).
>
> Without these prerequisites, test compilation and execution will fail due to assembler or linker errors.

---

## Usage

The current functionalities exposed through the interface include:

- Instantiating a Hammer object in `Python` which initializes the Spike simulator and preloads the ELF File.

  ```python
  import builddir.hammer as pyhammer
  hammer = pyhammer.Hammer("RV64GCV", "MSU", "vlen:512,elen:64", hart_ids, [mem_layout], elf_name, None)
  ```

  - Another example in `Python`:

  ```python
  import builddir.hammer as pyhammer
  mem_cfg = pyhammer.mem_cfg_t(pyhammer.DramBase, 256 * 1024 * 1024)
  hammer = pyhammer.Hammer("RV32IMC", "msu", "", hart_ids, [mem_cfg], target_binary, None)
  ```
- Instantiating a Hammer object in `C++` which initializes the Spike simulator and preloads the ELF File.

  ```cpp
  Hammer(const char *isa, const char *privilege_levels, const char *vector_arch,
         std::vector<size_t> hart_ids, std::vector<mem_cfg_t> memory_layout,
         const std::string target_binary, const std::optional<uint64_t> start_pc = std::nullopt)
  ```

  - Example snippet in `C++`:

  ```cpp
  std::vector<mem_cfg_t> memory_layout;
  // This is what Spike sets it to
  memory_layout.push_back(mem_cfg_t(reg_t(DRAM_BASE), reg_t(2048) << 20));

  std::vector<size_t> hart_ids{0};
  Hammer hammer = Hammer("RV64GCV", "MSU", "vlen:512,elen:32", hart_ids, memory_layout,
                         target_binary, std::nullopt);
  ```

Some more example snippets in `C++` can be found in `hammer/tests` and `hammer/pytests` for `Python`.

### Exposed Functionalities

- **get_gpr / set_gpr**: Read or write the value of a general-purpose register (GPR) for a specific hardware thread (hart).
- **get_fpr**: Retrieve the value of a floating-point register (FPR) for a given hart.
- **get_PC / set_PC**: Get or set the program counter (PC) for a specified hart.
  **NOTE:** The `get_PC` returns pc address which points to the next instruction to be executed, not the current instruction that has been executed.
- **get_csr**: Access the value of a control and status register (CSR) for a particular hart.
- **single_step**: Execute a single instruction on the specified hart, advancing its state by one step.
- **get_flen**: Obtain the floating-point register width (FLEN) for a hart.
- **get_vlen / get_elen**: Retrieve the vector register length (VLEN) and element length (ELEN) for a hart.
- **get_vector_reg**: Fetch the contents of a vector register for a given hart.
- **get_memory_at_VA**: Read a specified number of bytes from virtual memory at a given address for a hart, returning the data as a vector of the requested type.
- **set_memory_at_VA**: Write a vector of values to virtual memory at a specified address for a hart, storing the data as the given type.

### Additional Python Interface

  The Hammer Python module, exposed via `pybind11`, also provides:

- **Enums for Platform and CSR Registers**:The module exposes the `PlatformDefines` and `CsrDefines` enums, allowing you to refer to platform constants and a comprehensive set of RISC-V Control and Status Register (CSR) identifiers directly from Python. For example:

  ```python
  from builddir import hammer as pyhammer
  csr_val = pyhammer.CsrDefines.MSTATUS_CSR
  ```
- **Memory Configuration and Memory Objects**:The classes `mem_cfg_t` and `mem_t` are available in Python for configuring memory regions and representing memory, respectively.

  ```python
  mem_cfg = pyhammer.mem_cfg_t(base_addr, size)
  memory = pyhammer.mem_t(size)
  ```
- **hello_world**:
  A simple method for testing the Python binding, which returns a greeting string from the Hammer backend.

  ```python
  print(hammer.hello_world())
  ```

## Modifying Hammer for Our Use Case

With recent updates in Spike, several internal functions have changed their interface. As a result, our wrapper code around Spike had to be updated to reflect these **API changes**. This ensures compatibility and proper exposure of required functionality to Hammer.

The curent changes are done to Hammer with Spike commit `113ef64f6724b769ef9298e6a4a9802f7671021f` (https://github.com/riscv-software-src/riscv-isa-sim/commit/113ef64f6724b769ef9298e6a4a9802f7671021f) as of August 1, 2025.

With the recent updates in Spike, Hammer has been modified to accommodate these changes.

# Modifications:

## 1. What’s Added in This Fork ?

| Area                                | Change                                                                                                                                                       | Why it matters                                                                                                                                                                                                                                                                                               |
| ----------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Instruction introspection** | `get_insn*` helpers (word, hex, length,string) & `get_opcode`, `get_rs1_addr`, `get_rs2_addr`, `get_rs3_addr`, `get_rd_addr`, `get_csr_addr`  | To get more information about the instruction decoded                                                                                                                                                                                                                                                        |
| **RVC support**               | `get_rvc_opcode`, `get_rvc_rs*`, `get_rvc_rd_addr`                                                                                                     | Same as above but for 16‑bit compressed instructions                                                                                                                                                                                                                                                        |
| **Commit‑log access**        | `get_log_commits_enabled`, `get_log_reg_writes`, `get_log_mem_reads`, `get_log_mem_writes`                                                           | `<ul><li>`Enables spike to log the instructions `</li><li>`Access register logs to extract register write data and export to interface `</li><li>`Access memory read and write logs to extract memory accesses and export to interface `</li><li>`Enable debugging and trace analysis `</li></ul>` |
| **CSR access**                | `get_csr` now returns `std::optional<reg_t>`                                                                                                             | `<ul><li>`Avoids crashes on unimplemented CSRs instead of throwing access errors `</li><li>`Flexibility from interface side to check if a CSR instruction was executed or not `</li><li>`Provides a consistent interface for accessing CSR values `</li></ul>`                                       |
| **Helper**                    | private `get_insn_fetch()`                                                                                                                                 | Centralises MMU fetch so helpers stay small                                                                                                                                                                                                                                                                  |
| **Default logging**           | Spike log path now defaults to**`ham.log`** (Can be customised and compiled)                                                                               | Easy side‑by‑side core vs. cosim inspection                                                                                                                                                                                                                                                              |
| **Build system**              | *Meson* tweaks: `-D_GNU_SOURCE`, relaxed unused‑* warnings, removed hard‑wired test dirs                                                               | Builds cleanly with recent Clang/GCC and Spike headers                                                                                                                                                                                                                                                       |
| **Examples**                  | `hammer_example.py`, `hammer_run.py`                                                                                                                     | End‑to‑end demo: load ELF, single‑step, print decoded fields                                                                                                                                                                                                                                              |

Files touched:

```
README.md            (this file)
hammer.h             hammer.cpp
hammer_pybind.cpp    meson.build
native-file.txt      hammer_example.py
hammer_run.py        tests/test000.cpp
ham.log              (new default log)
```

**NOTE:** Since the Spike's API has changed, the existing tests in `tests/*` and `pytests/*` may need to be updated to reflect these changes. But to test the updated API , you can run `hammer_example.py` or `hammer_run.py` to see the new features in action.

---

## 2 · Extended C++ API

```cpp
 // New public methods on class Hammer
bool                         get_log_commits_enabled(uint8_t hart);
insn_t                       get_insn(uint8_t hart, reg_t pc);
int                          get_insn_length(uint8_t hart, reg_t pc);
uint64_t                     get_insn_hex(uint8_t hart, reg_t pc);
std::string 		     get_insn_string(uint8_t hart_id,reg_t pc);

uint64_t                     get_opcode(uint8_t hart, reg_t pc);
uint64_t                     get_rs1_addr(uint8_t hart, reg_t pc);
uint64_t                     get_rs2_addr(uint8_t hart, reg_t pc);
uint64_t                     get_rs3_addr(uint8_t hart, reg_t pc);
uint64_t                     get_rd_addr(uint8_t hart, reg_t pc);
uint64_t                     get_csr_addr(uint8_t hart, reg_t pc);

uint64_t                     get_rvc_opcode(uint8_t hart, reg_t pc);
uint64_t                     get_rvc_rs1_addr(uint8_t hart, reg_t pc);
uint64_t                     get_rvc_rs2_addr(uint8_t hart, reg_t pc);
uint64_t                     get_rvc_rd_addr(uint8_t hart, reg_t pc);

std::vector<std::pair<std::string,uint64_t>> get_log_reg_writes(uint8_t hart); // returns [register string name,data]
commit_log_mem_t             get_log_mem_reads(uint8_t hart); // returns vector of [addr,data,size]
commit_log_mem_t             get_log_mem_writes(uint8_t hart); //returns vector of [addr,data,size]
std::optional<reg_t>         get_csr(uint8_t hart, uint32_t csr_id);
```

 **Observations:** When using `get_log_mem_reads()` , the returned `commit_log_mem_t` length is atleast 4 in all instruction executions. The address it read in my case was the address next to `<kernel_stack_end>`. The reason for this is not very clear, but it seems to be a quirk of the Spike simulator. But if the length is more than 4 , the first instructions that it reads will be the actual memory read instruction, so you can take all the vector pairs except the last four in the returned vector.

```python
        mem_reads=sim.get_log_mem_reads(0)
        if mem_reads and len(mem_reads) > 4:
            print("=== MEMORY READS ===")
            for addr, value, size in mem_reads :
                addr &= 0xFFFFFFFF  # Mask to 32 bits
                print(f"READ ADDRESS: {addr:#x}, VALUE: {value:#x}, SIZE: {size} bytes")
                break
```

---

## 3 · Python Bindings

All new C++ methods are exported 1‑to‑1 via Pybind11.

```python
import hammer, pathlib

elf = pathlib.Path("./ibex_load_instr_test_0.o")
mem = hammer.mem_cfg_t(0x8000_0000, 256 * 1024 * 1024)

sim = hammer.Hammer("RV32IMC", "msu", "", [0], [mem], str(elf), None)

for _ in range(10):
    pc  = sim.get_PC(0)
    raw = sim.get_insn_hex(0, pc)
    opc = sim.get_opcode(0, pc)
    print(f"PC={pc:08x}  insn={raw:08x}  opcode={opc:03x}")
    sim.single_step(0)
```

---

## 4 · Building

Edit ~/.bashrc and add

```bash
export SPIKE_HOME=$HOME/hammer-deps/spike-install      # or whatever you used
```

to set the Spike install directory globally.

Then run the following commands in your terminal

### 1. Build and install Spike (exact commit below)

Skip this step if you edited `~/.bashrc` to set `SPIKE_HOME`.

```bash
export SPIKE_HOME=$HOME/hammer-deps/spike-install      # or whatever you used

git clone https://github.com/riscv-software-src/riscv-isa-sim
cd riscv-isa-sim && git checkout 113ef64f
mkdir build && cd build && ../configure --prefix=$SPIKE_HOME 
make -j$(nproc)
make install
```

### 2. Build Hammer

```bash
python3 -m pip install pybind11
meson setup builddir   --buildtype=release -Dspike_install_dir=$SPIKE_HOME
meson compile -C builddir
```

> **Tip:** `LD_LIBRARY_PATH=$SPIKE_HOME/lib` must include the Spike libraries when you import `hammer` in Python.

Add the following to your `~/.bashrc` to set the `LD_LIBRARY_PATH` and `builddir/` into PYTHONPATH globally:

```bash
export LD_LIBRARY_PATH=$SPIKE_HOME/lib:$LD_LIBRARY_PATH
export PYTHONPATH=path-to-hammer/builddir:$PYTHONPATH
```

Then run:

```bash
source ~/.bashrc
```

**Note:** If using 2 or more virtual environments(to build Hammer and to import Hammer elsewhere), make sure they both run on same Python version and in the virtual environment where you want to import Hammer, run the following command:

```bash
# Create a virtual environment in the directory where you want to import Hammer
python3 -m venv .venv

# Add Hammer to PYTHONPATH automatically when activating
echo 'export PYTHONPATH=path-to-hammer/hammer/builddir:$PYTHONPATH' > .venv/bin/activate.d/hammer.sh
chmod +x .venv/bin/activate.d/hammer.sh

source .venv/bin/activate
```

Now you can import `hammer` in Python and use it as described in the examples above.

---

## 5 · Running the Example

```bash
python3 hammer_example.py ./ibex_load_instr_test_0.o
```

Expect a short trace of `PC`, raw instruction, decoded opcode, plus an updated `ham.log` commit log.

To run a cpp test 

```cpp
g++ -std=c++20 -I$SPIKE_HOME/include -I$SPIKE_HOME/include/softfloat -I $SPIKE_HOME/include/fesvr -I. hammer_test_1.cpp <path-to-hammer>/builddir/libhammer.a -L$SPIKE_HOME/lib -lriscv -o my_hammer_test

 ./my_hammer_test ./ibex_load_instr_test_0.o 
```

---

## 6 · Known Limitations

* nly tested on **RV32IMC**; RV64 helpers compile but lack CI.
* Memory helpers show the last address/data pair seen by the MMU this cycle; concurrent load‑store contention on multi‑harts is **not** exposed yet.
* CSR access returns `None` if Spike was compiled without that CSR – handle it.

---
