#include "config.hpp"

Config read_config(const std::string& filename) {
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(filename, pt);

    Config config;

    config.host = pt.get<std::string>("server.host");
    config.port = pt.get<short>("server.port");
    config.log_level = pt.get<std::string>("server.log_level");

    config.dbname = pt.get<std::string>("database.dbname");
    config.dbuser = pt.get<std::string>("database.user");
    config.dbpassword = pt.get<std::string>("database.password");
    config.dbhost = pt.get<std::string>("database.host");
    config.dbport = pt.get<short>("database.port");

    return config;
}
