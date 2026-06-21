#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define ASSERT_HELP_OUT(cond, msg) do { if (!(cond)) { std::cerr << "[FAIL] test_cli_help_output_contract: " << msg << " (" << #cond << ")" << std::endl; std::exit(EXIT_FAILURE); } } while (0)

static int runCommand(const std::string& command) {
    return std::system(command.c_str());
}

static std::string readFile(const std::string& path) {
    std::ifstream in(path.c_str());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool contains(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

int main() {
    std::cout << "--- Starting CLI Help/Output Contract Tests ---" << std::endl;

    const std::string helpOut = "cli_help_output_contract.txt";
    const std::string runOut = "cli_run_output_contract.txt";
    const std::string csv = "cli_output_contract_valid.csv";
    std::remove(helpOut.c_str());
    std::remove(runOut.c_str());
    std::remove(csv.c_str());

    int status = runCommand("./project --help > " + helpOut + " 2>&1");
    ASSERT_HELP_OUT(status == 0, "--help should exit successfully");

    std::string help = lower(readFile(helpOut));
    ASSERT_HELP_OUT(contains(help, "usage") || contains(help, "help"), "help output should describe usage/help");
    ASSERT_HELP_OUT(contains(help, "--cycles"), "help output should mention --cycles");
    ASSERT_HELP_OUT(contains(help, "--tf"), "help output should mention --tf");
    ASSERT_HELP_OUT(contains(help, "--size-exponent"), "help output should mention --size-exponent");
    ASSERT_HELP_OUT(contains(help, "--size-mantissa"), "help output should mention --size-mantissa");
    ASSERT_HELP_OUT(contains(help, "--round-mode"), "help output should mention --round-mode");
    ASSERT_HELP_OUT(contains(help, "input") || contains(help, "file") || contains(help, "csv"), "help output should mention input CSV file");

    {
        std::ofstream out(csv.c_str());
        out << "8,0x3f800000,0x40000000,\n";
        out << "9,0x3f800000,0x3f800000,\n";
        out << "10,0x7f800000,0x00000000,0\n";
    }

    status = runCommand("./project --cycles 3 " + csv + " > " + runOut + " 2>&1");
    ASSERT_HELP_OUT(status == 0, "valid runtime invocation should succeed");
    std::string out = lower(readFile(runOut));
    ASSERT_HELP_OUT(!out.empty(), "runtime output should not be empty");
    ASSERT_HELP_OUT(contains(out, "cycle"), "runtime output should mention cycles");
    ASSERT_HELP_OUT(contains(out, "zero"), "runtime output should mention zero count");
    ASSERT_HELP_OUT(contains(out, "sign"), "runtime output should mention sign count");
    ASSERT_HELP_OUT(contains(out, "overflow"), "runtime output should mention overflow count");
    ASSERT_HELP_OUT(contains(out, "underflow"), "runtime output should mention underflow count");
    ASSERT_HELP_OUT(contains(out, "inexact"), "runtime output should mention inexact count");
    ASSERT_HELP_OUT(contains(out, "nan"), "runtime output should mention NaN count");

    std::remove(helpOut.c_str());
    std::remove(runOut.c_str());
    std::remove(csv.c_str());

    std::cout << "[SUCCESS] CLI Help/Output Contract Tests passed!" << std::endl;
    return 0;
}
