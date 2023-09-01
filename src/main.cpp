#include "common.hpp"
#include "firebase/firebase.hpp"
#include "git.h"
#include "mock_api/mock_api.hpp"
#include "pywrapper/pywrapper.hpp"

#include <argparse/argparse.hpp>
#include <pybind11/pybind11.h>

#include <iostream>
#include <string>
#include <tuple>

static std::tuple<uint8_t, std::string>
process_arguments(int argc, const char** argv)
{
    argparse::ArgumentParser program(
        "NUTC Linter", VERSION, argparse::default_arguments::help
    );

    program.add_argument("-U", "--uid")
        .help("set the user ID")
        .action([](const auto& value) { return std::string(value); })
        .required();

    program.add_argument("-V", "--version")
        .help("prints version information and exits")
        .action([&](const auto& /* unused */) {
            fmt::println("NUTC Linter v{}", VERSION);
            exit(0); // NOLINT(concurrency-*)
        })
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    uint8_t verbosity = 0;
    program.add_argument("-v", "--verbose")
        .help("increase output verbosity")
        .action([&](const auto& /* unused */) { ++verbosity; })
        .append()
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        exit(1); // NOLINT(concurrency-*)
    }

    return std::make_tuple(verbosity, program.get<std::string>("--uid"));
}

static void
log_build_info()
{
    log_i(main, "NUTC Linter: Linter for NUTC user-submitted algorithms");

    // Git info
    log_i(main, "Built from {} on {}", git_Describe(), git_Branch());
    log_d(main, "Commit: \"{}\" at {}", git_CommitSubject(), git_CommitDate());
    log_d(main, "Author: {} <{}>", git_AuthorName(), git_AuthorEmail());

    if (git_AnyUncommittedChanges())
        log_w(main, "Built from dirty commit!");
}

int
main(int argc, const char** argv)
{
    // Parse args
    auto [verbosity, uid] = process_arguments(argc, argv);

    // Start logging and print build info
    nutc::logging::init(verbosity);
    log_build_info();
    log_i(main, "Starting NUTC Client for UID {}", uid);

    bool hasAlgo = nutc::client::get_most_recent_algo(uid);
    if (!hasAlgo) {
        return 0;
    }

    pybind11::scoped_interpreter guard{};
    std::optional<std::string> e =
        nutc::pywrapper::create_api_module(nutc::mock_api::getMarketFunc());
  if(e.has_value()){
    log_e(main, "Failed to create API module: {}", e.value());
    return 1;
  }

    return 0;
}
