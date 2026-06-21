#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#define ASSERT_CLI_RUNTIME(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_blackbox_cli_runtime: " << msg
                      << " (" << #cond << ")" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

static bool fileExists(const std::string& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

static void writeValidCsv(const std::string& path) {
    std::ofstream file(path.c_str());
    file << "8,0x3f800000,0x40000000,\n";
}

static int runCommand(const std::string& command) {
    return std::system(command.c_str());
}

int main() {
    std::cout << "--- Starting BlackBox Runtime CLI Tests ---" << std::endl;

    const std::string csv_file = "runtime_cli_valid.csv";
    const std::string output_file = "runtime_cli_output.txt";

    std::remove(csv_file.c_str());
    std::remove(output_file.c_str());

    writeValidCsv(csv_file);

    ASSERT_CLI_RUNTIME(
        fileExists(csv_file),
        "Failed to create temporary valid CSV file"
    );

    // Default invocation:
    // Only positional input file, all other options should use defaults.

        int status = runCommand(
        "./project " + csv_file + " > " + output_file + " 2>&1"
    );

    ASSERT_CLI_RUNTIME(
        status == 0,
        "Project failed with default CLI options"
    );

    // --help should succeed and should not require an input file.

    status = runCommand(
        "./project --help > " + output_file + " 2>&1"
    );

    ASSERT_CLI_RUNTIME(
        status == 0,
        "--help did not terminate successfully"
    );

    // Nonexistent input file should fail.

    status = runCommand(
        "./project file_that_does_not_exist_123.csv > " + output_file + " 2>&1"
    );

    ASSERT_CLI_RUNTIME(
        status != 0,
        "Project accepted nonexistent input file"
    );

    // Invalid tracefile path should fail.

    status = runCommand(
        "./project --tf /directory_that_should_not_exist_12345/trace "
        + csv_file + " > " + output_file + " 2>&1"
    );

    ASSERT_CLI_RUNTIME(
        status != 0,
        "Project accepted invalid tracefile path"
    );

    // Invalid format options should fail.

    status = runCommand(
        "./project --size-exponent 0 "
        + csv_file + " > " + output_file + " 2>&1"
    );

    ASSERT_CLI_RUNTIME(
        status != 0,
        "Project accepted exponent size 0"
    );

    status = runCommand(
        "./project --size-mantissa 0 "
        + csv_file + " > " + output_file + " 2>&1"
    );

    ASSERT_CLI_RUNTIME(
        status != 0,
        "Project accepted mantissa size 0"
    );

    status = runCommand(
        "./project --size-exponent 20 --size-mantissa 20 "
        + csv_file + " > " + output_file + " 2>&1"
    );

    ASSERT_CLI_RUNTIME(
        status != 0,
        "Project accepted exponent/mantissa combination that is too large"
    );

    std::remove(csv_file.c_str());
    std::remove(output_file.c_str());

    std::cout << "[SUCCESS] BlackBox Runtime CLI Tests passed!" << std::endl;
    return 0;
}