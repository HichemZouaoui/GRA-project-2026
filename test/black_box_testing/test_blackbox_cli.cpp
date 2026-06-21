#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "cli_parser.h"
#include "test_common.h"

#define ASSERT_CLI(cond, msg)
    do {
        if (!(cond)) {
            std::cerr << "[FAIL] test_blackbox_cli: " << msg << std::endl;
            std::exit(EXIT_FAILURE);
        }
    } while (0)

static bool run(
    const std::vector<std::string>& args,
    CLIOptions& opts,
    std::string& err
) {
    std::vector<char*> argv;

    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }

    return parse_cli_arguments(
        static_cast<int>(argv.size()),
        argv.data(),
        opts,
        err
    );
}

int main() {
    CLIOptions opts;
    std::string err;

    ASSERT_CLI(
        run({"./project", "--cycles", "1000", "requests.csv"}, opts, err),
        "valid minimal invocation failed"
    );

    ASSERT_CLI(
        opts.cycles == 1000 && opts.input_file == "requests.csv",
        "minimal invocation values are incorrect"
    );

    ASSERT_CLI(
        run({
            "./project",
            "--cycles", "250",
            "--tf", "trace",
            "--size-exponent", "8",
            "--size-mantissa", "23",
            "--round-mode", "0",
            "requests.csv"
        }, opts, err),
        "valid full invocation failed"
    );

    ASSERT_CLI(
        !run({"./project", "--cycles", "500"}, opts, err) && !err.empty(),
        "missing input file was accepted"
    );

    ASSERT_CLI(
        !run({"./project", "--cycles", "x", "requests.csv"}, opts, err),
        "invalid cycles value was accepted"
    );

    ASSERT_CLI(
        !run({"./project", "--round-mode", "9", "requests.csv"}, opts, err),
        "invalid rounding mode was accepted"
    );

    ASSERT_CLI(
        !run({
            "./project",
            "--size-exponent", "20",
            "--size-mantissa", "20",
            "requests.csv"
        }, opts, err),
        "invalid exponent/mantissa format was accepted"
    );

    ASSERT_CLI(
        !run({"./project", "a.csv", "b.csv"}, opts, err),
        "multiple input files were accepted"
    );

    ASSERT_CLI(
        run({"./project", "--help"}, opts, err) && opts.help,
        "--help option failed"
    );

    std::cout << "[SUCCESS] CLI Parser Tests passed!" << std::endl;
    return 0;
}