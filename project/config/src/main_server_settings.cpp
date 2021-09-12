#include "main_server_settings.h"
#include "parse_functions.h"

const std::vector<std::string> MainServerSettings::valid_properties = {"http", "count_workflows", "access_log",
                                                                       "error_log", "server"};

MainServerSettings::MainServerSettings(const std::string &config_filename) : config_filename(std::move(config_filename))
{
    parse_config(*this);
}

int MainServerSettings::get_number_of_property(const std::string &property)
{
    int begin = 0;
    while (isspace(property[begin]))
    {
        ++begin;
    }
    int property_length = (property[property.length() - 1] == ':') ? property.length() - begin - 1 : property.length() - begin;

    for (auto iter = this->valid_properties.begin(); iter != this->valid_properties.end(); ++iter)
    {
        if (property.substr(begin, property_length) == *iter)
        {
            return iter - this->valid_properties.begin();
        }
    }

    return -1;
}

void MainServerSettings::set_property(int number_of_property, const std::string &value)
{
    int begin = 0;
    while (isspace(value[begin]))
    {
        ++begin;
    }
    int value_length = (value[value.length() - 1] == ';') ? value.length() - begin - 1 : value.length() - begin;
    switch (number_of_property)
    {
    case COUNT_WORKFLOWS_NUMBER:
    {
        try
        {
            this->count_workflows = stoi(value.substr(begin, value_length));
        }
        catch (std::exception &e)
        {
            throw InvalidConfigException("count_workflows can be only integer");
        }
        if (this->count_workflows <= 0)
        {
            throw InvalidConfigException("count_workflows can be only greater than zero");
        }
        break;
    }
    case ACCESS_LOG_NUMBER:
        this->access_log_filename = value.substr(begin, value_length);
        break;
    case ERROR_LOG_NUMBER:
        this->error_log_filename = value.substr(begin, value_length);
        break;
    }
}

int MainServerSettings::get_count_workflows()
{
    return this->count_workflows;
}

ServerSettings MainServerSettings::get_server()
{
    return this->server;
}

std::string MainServerSettings::get_access_log_filename()
{
    return this->access_log_filename;
}

std::string MainServerSettings::get_error_log_filename()
{
    return this->error_log_filename;
}
