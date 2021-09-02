#pragma once

#include <string>
#include <vector>
#include <map>

#include "server_settings.h"

class MainServerSettings {
public:
    MainServerSettings() = default;

    explicit MainServerSettings(const std::string &config_filename);

    ~MainServerSettings() = default;

    static const std::vector<std::string> valid_properties;

    int get_number_of_property(const std::string &property);

    void set_property(int number_of_property, const std::string &value);

    friend void parse_config(MainServerSettings &server);

    int get_count_workflows();

    ServerSettings get_server();

    std::string get_access_log_filename();

    std::string get_error_log_filename();

private:
    std::string config_filename;

    int count_workflows = 4;

    std::string access_log_file, error_log_file;

    ServerSettings server;

    typedef enum {
        COUNT_WORKFLOWS_NUMBER = 1,
        ACCESS_LOG_NUMBER = 2,
        ERROR_LOG_NUMBER = 3
    } numbers_of_properties;
};
