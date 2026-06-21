#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define ASSERT_RUNTIME_STRICT(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_cli_runtime_strict: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

static int runCommand(const std::string& cmd) {
    return std::system(cmd.c_str());
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path.c_str());
    f << content;
}

int main() {
    std::cout << "--- Starting Strict Runtime CLI Tests ---" << std::endl;

    const std::string valid = "strict_runtime_valid.csv";
    const std::string invalid = "strict_runtime_invalid.csv";
    const std::string out = "strict_runtime_output.txt";
    const std::string trace = "strict_runtime_trace";
    const std::string traceVcd = trace + ".vcd";

    std::remove(valid.c_str());
    std::remove(invalid.c_str());
    std::remove(out.c_str());
    std::remove(traceVcd.c_str());

    writeFile(valid,
              "8,0x3f800000,0x40000000,\n"
              "9,1.0,1.0,\n"
              "10,2.0,3.0,\n"
              "13,-1.0,2.0,\n"
              "14,-1.0,2.0,\n"
              "15,1.0,2.0,3.0\n");
    writeFile(invalid,
              "8,1.0,2.0,\n"
              "15,1.0,2.0,\n");

    ASSERT_RUNTIME_STRICT(fileExists(valid), "failed to create valid runtime fixture");
    ASSERT_RUNTIME_STRICT(runCommand("./project " + valid + " > " + out + " 2>&1") == 0, "default invocation failed");
    ASSERT_RUNTIME_STRICT(runCommand("./project --cycles 20 --round-mode 0 --size-exponent 8 --size-mantissa 23 " + valid + " > " + out + " 2>&1") == 0, "full explicit invocation failed");
    ASSERT_RUNTIME_STRICT(runCommand("./project --cycles 20 --tf " + trace + " " + valid + " > " + out + " 2>&1") == 0, "trace invocation failed");
    ASSERT_RUNTIME_STRICT(fileExists(traceVcd), "trace invocation did not create VCD file");

    ASSERT_RUNTIME_STRICT(runCommand("./project --help > " + out + " 2>&1") == 0, "--help should succeed without CSV file");
    ASSERT_RUNTIME_STRICT(runCommand("./project --cycles abc " + valid + " > " + out + " 2>&1") != 0, "non-numeric cycles accepted");
    ASSERT_RUNTIME_STRICT(runCommand("./project --cycles -1 " + valid + " > " + out + " 2>&1") != 0, "negative cycles accepted");
    ASSERT_RUNTIME_STRICT(runCommand("./project --round-mode 5 " + valid + " > " + out + " 2>&1") != 0, "invalid round mode accepted");
    ASSERT_RUNTIME_STRICT(runCommand("./project --size-exponent 0 " + valid + " > " + out + " 2>&1") != 0, "zero exponent size accepted");
    ASSERT_RUNTIME_STRICT(runCommand("./project --size-mantissa 0 " + valid + " > " + out + " 2>&1") != 0, "zero mantissa size accepted");
    ASSERT_RUNTIME_STRICT(runCommand("./project --size-exponent 20 --size-mantissa 20 " + valid + " > " + out + " 2>&1") != 0, "oversized format accepted");
    ASSERT_RUNTIME_STRICT(runCommand("./project " + invalid + " > " + out + " 2>&1") != 0, "malformed CSV accepted at runtime");
    ASSERT_RUNTIME_STRICT(runCommand("./project does_not_exist_hidden_style.csv > " + out + " 2>&1") != 0, "missing input file accepted");
    ASSERT_RUNTIME_STRICT(runCommand("./project --unknown-option " + valid + " > " + out + " 2>&1") != 0, "unknown option accepted");

    std::remove(valid.c_str());
    std::remove(invalid.c_str());
    std::remove(out.c_str());
    std::remove(traceVcd.c_str());

    std::cout << "[SUCCESS] Strict Runtime CLI Tests passed!" << std::endl;
    return 0;
}
