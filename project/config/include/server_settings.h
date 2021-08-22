#pragma once

#include <string>
#include <vector>

typedef struct {
    std::string url;
    std::string root;
    bool case_sensitive;
    bool is_access_log;
    bool is_error_log;
} location_t;

class ServerSettings {
public:
    ServerSettings() = default;

    ~ServerSettings() = default;

    static const std::vector<std::string> valid_properties;

    static const std::vector<std::string> valid_location_properties;

    int get_number_of_property(std::string property);

    void set_property(int number_of_property, std::string value);

    int get_number_of_location_property(std::string property);

    void set_location_property(int number_of_property, std::string value, location_t &location);

    void add_exact_match_url(location_t &location);

    void add_preferential_prefix_urls(location_t &location);

    void add_regex_match_urls(location_t &location);

    void add_prefix_match_urls(location_t &location);

    std::string get_access_log_filename();

    std::string get_error_log_filename();

    location_t *get_location(std::string &url);

    void print_properties();

    int get_port();

    std::string get_servername();

    friend int parse_location(ServerSettings &server, std::string &config, int &pos);

private:
    int port;

    std::string servername;

    std::string access_log_file, error_log_file;

    std::string root;

    bool is_root = false;

    std::vector<location_t> exact_match_urls, preferential_prefix_urls, regex_match_urls, prefix_match_urls;

    // the value of the elements enum is written the index of this key in the array of properties
    typedef enum {
        LISTEN_NUMBER = 0,
        ROOT_NUMBER = 1,
        ACCESS_LOG_NUMBER = 2,
        ERROR_LOG_NUMBER = 3,
        SERVERNAME_NUMBER = 5
    } numbers_of_properties;

    typedef enum {
        ROOT_LOCATION_NUMBER = 0,
        ADD_ROOT_NUMBER = 1,
        ACCESS_LOG_LOCATION_NUMBER = 2,
        ERROR_LOG_LOCATION_NUMBER = 3
    } numbers_of_location_properties;
};
