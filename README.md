# Floating-Point Unit (FPU) with FMA

**Grundlagenpraktikum Rechnerarchitektur — Abschlussprojekt**  
Lehrstuhl für Design Automation, TU München

---

## Module Description

This project implements a **Floating-Point Unit (FPU)** as a SystemC module (`FLOATING_POINT_UNIT`) supporting configurable-precision arithmetic based on a subset of the IEEE 754 standard. The unit operates on 32-bit registers and processes one operation per clock cycle.

### Supported Operations

| Op Code | Name  | Result          |
|---------|-------|-----------------|
| 8       | FADD  | `r1 + r2`       |
| 9       | FSUB  | `r1 - r2`       |
| 10      | FMUL  | `r1 * r2`       |
| 13      | FMIN  | `min(r1, r2)`   |
| 14      | FMAX  | `max(r1, r2)`   |
| 15      | FMA   | `r1 + (r2 * r3)`|

The FMA operation performs the multiply step **without intermediate rounding**, storing the full-precision product before the addition. Final rounding to the target format happens only once, after the addition.

Status flags (`zero`, `sign`, `overflow`, `underflow`, `inexact`, `nan`) are set per operation result.

---

## Build

### Prerequisites

- GCC (C17) and G++ (C++14)
- SystemC 2.3.3 or 2.3.4 (set `SYSTEMC_HOME` before building)
- Optional: `cppcheck`, `valgrind`, `kcachegrind`

```bash
export SYSTEMC_HOME=/path/to/systemc
```

### Build Targets

| Command | Description |
|---|---|
| `make project` | **Release build** → `./project` (required by grader) |
| `make debug` | Debug build with AddressSanitizer + UBSan + LeakSan |
| `make run-valgrind ARGS="..."` | Valgrind memcheck run |
| `make callgrind ARGS="..."` | Callgrind profiler (view with `kcachegrind`) |
| `make cppcheck` | Static analysis |
| `make clean` | Remove all build artifacts |

---

## Usage

```
./project [OPTIONS] <file>
```

### Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--cycles <n>` | uint32 | 1000 | Number of simulation cycles |
| `--tf <path>` | string | *(none)* | Path for trace file output |
| `--size-exponent <n>` | uint8 | 8 | Exponent bit width |
| `--size-mantissa <n>` | uint8 | 23 | Mantissa bit width |
| `--round-mode <0–4>` | uint8 | 0 | Rounding mode (see below) |
| `--help` | flag | — | Print usage and exit |

**Constraints:** `size-exponent + size-mantissa + 1 (sign bit) must equal 32.`

### Rounding Modes

| Mode | Name |
|------|------|
| 0 | Round to nearest, ties to even *(default)* |
| 1 | Round to nearest, ties away from zero |
| 2 | Round toward zero |
| 3 | Round toward +INF |
| 4 | Round toward −INF |

### Example

```bash
./project --cycles 1000 --size-exponent 8 --size-mantissa 23 --round-mode 0 test/requests.csv
```

---

## Input File Format

CSV file, **no header row**, comma-separated:

```
op, r1, r2 [, r3]
```

Operands may be given as:
- **Hex literals:** `0x3f800000`
- **Float literals:** `1.0`, `3.14e-5`
- **Decimal integers:** bit pattern reinterpreted as float

```
8, 0x3f800000, 0x40000000,
15, 0x3ffffed, 0x3ffffffd, 0x3ffffffa
10, 0.125, 0.0,
13, 0.75, 1.5,
```

---

## Project Structure

```
.
├── Makefile
├── README.md
├── build.sh
├── src/
│   ├── main.c
│   ├── cli.c
│   ├── csv_reader.c
│   ├── operand_convert.c
│   ├── result_printer.c
│   └── floating_point_unit.cpp   # SystemC module (C++ team)
├── include/
│   └── project.h
└── test/
    └── requests.csv
```

---

## Group Contributions

<!-- ~100–150 words — fill in before submission -->

| Member | Contributions |
|--------|--------------|
| [Name 1] | [e.g. Framework (CLI, CSV parser, operand conversion)] |
| [Name 2] | [e.g. SystemC FPU module, FMA implementation] |
| [Name 3] | [e.g. Rounding logic, flag detection, testing] |

---

## Literature Review

### Key Terms

#### Fused Multiply-Add (FMA)
FMA computes `a + (b × c)` in a single hardware step, keeping the intermediate product of `b × c` at extended precision before adding `a`. Compared to performing a separate multiply then add, this eliminates **one rounding step**, which improves numerical accuracy — especially important in iterative algorithms where rounding errors accumulate. The IEEE 754-2008 standard formally defines FMA as a required operation.

#### Floating-Point Extensions
Modern ISAs extend their base instruction sets with dedicated floating-point instructions. RISC-V defines the **F** (single-precision), **D** (double-precision), and **Q** (quad-precision) standard extensions, each adding registers and instructions for float arithmetic. x86 CPUs provide SSE2 (scalar and packed floats), with AVX/AVX-512 adding wider SIMD registers and FMA3/FMA4 fused instructions. This project implements a simplified floating-point extension analogous to RISC-V's F extension.

#### FLOATING_POINT_UNIT (this module)
A hardware module that receives operands and an opcode each clock cycle, executes the requested floating-point operation, stores the result in `ro`, and sets status flags. This module could be connected to the ALU of a TinyRISC CPU by extending its control unit to decode the new opcodes and route operands to this unit.

---

### Research Questions

#### Why is FMA especially important for neural network computation?

Neural network inference and training rely almost entirely on **multiply-accumulate (MAC)** operations of the form `output += weight * input`, which appears in every linear layer and convolution. FMA executes this pattern in one instruction instead of two, giving two benefits simultaneously: **speed** (fewer instructions, better pipeline utilization) and **accuracy** (no intermediate rounding, which matters when accumulating millions of small products during backpropagation). Hardware accelerators like Google's TPU and NVIDIA's Tensor Cores are essentially large arrays of FMA units for exactly this reason.

#### Other uses of FMA

- **Scientific computing:** dot products and matrix-vector products (BLAS routines), polynomial evaluation via Horner's method (`a_n * x + a_{n-1}` repeated)
- **Digital signal processing:** FIR filters sum many `coefficient * sample` products — FMA is a natural fit
- **3D graphics:** 4×4 matrix multiplications for vertex transformations (used in every GPU shader)
- **Compensated summation:** Kahan summation algorithm uses FMA to correct floating-point addition errors with near-zero overhead

#### Min/Max representable values for different exponent/mantissa widths

For a custom format with `e` exponent bits and `m` mantissa bits (+ 1 sign bit = 32 total):

- **Bias** = 2^(e−1) − 1
- **Max positive value** = (2 − 2^−m) × 2^(bias)  ≈ 2^(bias+1)
- **Min positive normal** = 2^(1 − bias)  = 2^(2 − 2^(e−1))

| `--size-exponent` | `--size-mantissa` | Bias | Max value (approx) | Min positive normal (approx) |
|:-----------------:|:-----------------:|:----:|-------------------:|-----------------------------:|
| 8 | 23 | 127 | 3.40 × 10^38 | 1.18 × 10^−38 |
| 4 | 27 | 7 | 240 | 1.56 × 10^−2 |
| 6 | 25 | 31 | 6.55 × 10^9 | 4.66 × 10^−10 |
| 10 | 21 | 511 | 2^512 ≈ 10^154 | 2^−510 ≈ 10^−154 |

Wider exponent fields give a larger **dynamic range**; wider mantissa fields give higher **precision** for the same range.

---

### Sources

<!-- Add your sources here — they do not count toward the 800-word limit -->

- IEEE Std 754-2008, *IEEE Standard for Floating-Point Arithmetic*
- Patterson & Hennessy, *Computer Organization and Design RISC-V Edition*, Ch. 3
- Muller et al., *Handbook of Floating-Point Arithmetic*, Birkhäuser, 2010
- RISC-V ISA Specification v2.2, Chapter 8 (F Extension)