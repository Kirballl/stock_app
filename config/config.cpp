#include "config.hpp"

Config read_config(const std::string& filename) {
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(filename, pt);

    Config config;
    config.host = pt.get<std::string>("server.host");
    config.port = pt.get<short>("server.port");
    config.log_level = pt.get<std::string>("server.log_level");

    return config;
}
