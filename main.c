#include "project.h"

int main(int argc,char* argv[]){
    struct CliOptions options;
    parse_cli_arguments(argc,argv,&options);
    uint32_t num_requests = 0;
    struct Request* requests = parse_csv_file(
        options.input_file,
        &num_requests,
        options.size_exponent,
        options.size_mantissa
    );

    struct Result sim_result = runSimulation(
        options.cycles,
        options.tracefile,
        options.size_exponent,
        options.size_mantissa,
        options.round_mode,
        num_requests, 
        requests
    );

    print_simulation_results(&sim_result);
    free(requests);
    return 0;
}
