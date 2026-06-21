#!/usr/bin/env bash
set -euo pipefail

run_test() {
    local test_bin="$1"
    if [[ ! -x "$test_bin" ]]; then
        echo "[FAIL] Missing or non-executable test binary: $test_bin"
        exit 1
    fi
    echo
    echo "----------------------------------------"
    echo "Running $test_bin"
    echo "----------------------------------------"
    "$test_bin"
}

echo "========================================"
echo "Cleaning project..."
echo "========================================"
make clean || true

echo "========================================"
echo "Building required project executable..."
echo "========================================"
make project

if [[ ! -x "./project" ]]; then
    echo "[FAIL] Required executable './project' was not created by 'make project'."
    exit 1
fi

echo "========================================"
echo "Building all tests..."
echo "========================================"
make tests

echo
echo "========================================"
echo "Running every test binary in ./bin"
echo "========================================"

mapfile -t tests < <(find ./bin -maxdepth 1 -type f -executable -name 'test_*' | sort)

if [[ ${#tests[@]} -eq 0 ]]; then
    echo "[FAIL] No test binaries were built in ./bin."
    exit 1
fi

for test_bin in "${tests[@]}"; do
    run_test "$test_bin"
done

echo
echo "========================================"
echo "All tests passed."
echo "========================================"
