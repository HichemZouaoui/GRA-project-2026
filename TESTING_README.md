# Testing Summary

## Overview

The testing part of the project is designed to verify the correctness, robustness, and specification compliance of the Floating Point Unit project. The tests are organized in several layers, starting from small unit tests for helper components and ending with black-box and integration tests that simulate realistic usage of the final executable.

The goal is not only to check simple correct results, but also to catch hidden-test-style errors such as wrong rounding behavior, incorrect flag handling, invalid CSV parsing, broken CLI options, stale flags between operations, and special floating-point edge cases.

## General Testing Strategy

The strategy follows a bottom-up approach:

First, the basic utility classes are tested independently. These include floating-point format handling, packing and unpacking values, special values such as zero, infinity and NaN, operand conversion, and rounding.

After that, the arithmetic operations are tested. Each operation is checked with normal values and edge cases. This includes `FADD`, `FSUB`, `FMUL`, `FMIN`, `FMAX`, and especially `FMA`.

Then, the simulation layer is tested through `runSimulation`, including cycle counting, output values, exception flags, and tracefile generation.

Finally, black-box tests check the complete program behavior through the command line, CSV input files, invalid inputs, help output, and runtime behavior.

## Test Structure

The tests are divided into the following main groups:

### 1. Common Structure Tests

These tests verify that the shared definitions are correct. They check the operation codes, rounding mode constants, and the structure layout of `Request` and `Result`.

This is important because many hidden tests depend on the exact expected field order and opcode values.

### 2. FPUtils Tests

These tests check the floating-point format helper functions. They verify:

* correct exponent bias calculation
* correct packing of sign, exponent, and mantissa
* positive and negative zero
* positive and negative infinity
* NaN detection
* maximum and minimum representable values
* custom formats such as 8/23, 5/10, and 4/8

These tests ensure that the implementation does not only work for standard 32-bit IEEE-like floats, but also for custom exponent and mantissa sizes.

### 3. Rounding Tests

The rounding tests verify all five required rounding modes:

* round to nearest, ties to even
* round to nearest, ties away from zero
* round toward zero
* round toward positive infinity
* round toward negative infinity

The tests include exact cases, tie cases, positive numbers, negative numbers, and inexact results. This is one of the most important parts because hidden tests often check rounding very strictly.

### 4. Operand Conversion Tests

These tests verify conversion from CSV/string input into the internal floating-point bit representation.

They check:

* hexadecimal raw bit input
* decimal integer raw bit input
* floating-point literals
* scientific notation
* `inf`, `-inf`, and `nan`
* signed zero
* custom floating-point formats

This ensures that input values are interpreted correctly before arithmetic even starts.

### 5. CSV Parser Tests

The CSV parser tests check both valid and invalid input rows.

They verify that the parser accepts correct operation lines and rejects malformed ones, such as:

* invalid opcodes
* missing operands
* missing FMA operands
* too many columns
* invalid numeric strings
* malformed CSV rows

This protects the project against hidden tests that use invalid or unusual input files.

### 6. Arithmetic Operation Tests

The arithmetic tests verify the required operations:

* `FADD`
* `FSUB`
* `FMUL`
* `FMIN`
* `FMAX`
* `FMA`

The tests check normal arithmetic results, special values, and exception flags. They include cases involving zero, negative zero, infinity, NaN, overflow, underflow, and inexact results.

Special attention is given to `FMA`, because the project requires the operation to compute `r1 + (r2 * r3)` without intermediate rounding.

### 7. FMA Precision Tests

The FMA tests are designed to detect whether the implementation incorrectly rounds the multiplication before adding `r1`.

These tests compare the final bit-pattern result against carefully chosen expected values. This helps catch one of the most common mistakes in FMA implementations: treating it as a normal multiplication followed by a normal addition.

### 8. Simulation Tests

The simulation tests verify the `runSimulation` function.

They check:

* correct cycle count
* correct result storage in `Request.ro`
* correct counting of zero, sign, overflow, underflow, inexact, and NaN flags
* behavior with multiple requests
* behavior with tracefile generation

This verifies that the arithmetic core is correctly connected to the simulation interface.

### 9. SystemC Integration Tests

The SystemC tests instantiate the `FLOATING_POINT_UNIT` module and drive it using SystemC signals.

They verify:

* correct output after clock cycles
* correct behavior of all main operations
* correct flag output
* reset of flags between operations
* ignoring unused operands when appropriate
* FMA behavior through the SystemC module

These tests are important because the project is not only a software arithmetic task, but also a SystemC module simulation.

### 10. Black-Box CLI Tests

The CLI tests check the final executable from the outside.

They verify:

* valid program execution
* `--help`
* default arguments
* invalid options
* invalid rounding modes
* invalid exponent or mantissa sizes
* missing input files
* invalid tracefile paths
* correct behavior with CSV input files

These tests simulate how the evaluator may run the program during grading.

## Edge Cases Covered

The test suite covers many important floating-point edge cases, including:

* positive zero and negative zero
* sign flag behavior
* infinity arithmetic
* NaN propagation
* `INF * 0`
* `INF - INF`
* `0 * NaN` according to the project rule
* overflow
* underflow
* subnormal-related behavior
* inexact results
* FMA precision without intermediate rounding
* all rounding modes
* custom exponent and mantissa sizes
* invalid CSV input
* invalid CLI input
* stale flag reset between operations

## Why This Test Suite Is Strong

The test suite combines unit tests, integration tests, and black-box tests. This means it can detect errors at different levels:

* small helper function bugs
* arithmetic logic bugs
* rounding bugs
* parsing bugs
* simulation interface bugs
* SystemC connection bugs
* final executable behavior bugs

The tests are also designed to be close to possible hidden tests. They do not only check easy examples, but also boundary cases and ambiguous cases where many implementations fail.

## Limitations

Although the test suite is extensive, no test suite can guarantee that every possible hidden test will pass. Some hidden tests may check exact output formatting, exact NaN payloads, professor-specific signed-zero behavior, or very strict subnormal arithmetic.

However, the current test suite covers the main project requirements and a large part of the realistic hidden-test categories.
