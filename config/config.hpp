#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

struct Config {
    std::string host;
    short port;

    std::string log_level;

    std::string dbname;
    std::string dbuser;
    std::string dbpassword;
    std::string dbhost;
    short dbport;

    std::string jwt_secret_key;
};

Config read_config(const std::string& filename);

#endif // CONFIG_HPP
