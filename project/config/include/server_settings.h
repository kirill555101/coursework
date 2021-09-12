#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <regex>

#include "server_settings_exceptions.h"
#include "config_file_exceptions.h"

typedef struct
{
    std::string url;
    std::string root;
    bool case_sensitive;
} location_t;

class ServerSettings
{
public:
    ServerSettings() = default;

    ~ServerSettings() = default;

    static const std::vector<std::string> valid_properties;

    static const std::vector<std::string> valid_location_properties;

    int get_number_of_property(const std::string &property);

    void set_property(int number_of_property, std::string value);

    int get_number_of_location_property(const std::string &property);

    void set_location_property(int number_of_property, const std::string &value, location_t &location);

    void add_exact_match_url(location_t &location);

    void add_preferential_prefix_url(location_t &location);

    void add_regex_match_url(location_t &location);

    void add_prefix_match_url(location_t &location);

    location_t *get_location(const std::string &url);

    int get_port();

    std::string get_servername();

    friend int parse_location(ServerSettings &server, std::string &config, int &pos);

private:
    int port = 80;

    std::string servername = "127.0.0.1";

    std::string root = "/";

    bool is_root = false;

    std::vector<location_t> exact_match_urls, preferential_prefix_urls, regex_match_urls, prefix_match_urls;

    typedef enum
    {
        LISTEN_NUMBER,
        ROOT_NUMBER,
        SERVERNAME_NUMBER
    } numbers_of_properties;

    typedef enum
    {
        ROOT_LOCATION_NUMBER,
        ADD_ROOT_NUMBER
    } numbers_of_location_properties;
};
