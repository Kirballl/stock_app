#include "common.hpp"

spdlog::level::level_enum set_logging_level(const Config& config) {
    spdlog::level::level_enum log_level;
    if (config.log_level == "debug") {
        log_level = spdlog::level::debug;
    } else if (config.log_level == "info") {
        log_level = spdlog::level::info;
    } else if (config.log_level == "warn") {
        log_level = spdlog::level::warn;
    } else if (config.log_level == "error") {
        log_level = spdlog::level::err;
    } else if (config.log_level == "critical") {
        log_level = spdlog::level::critical;
    } else {
        log_level = spdlog::level::debug; // default
    }
    return log_level;
}