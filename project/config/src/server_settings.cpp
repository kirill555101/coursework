#include <iostream>
#include <algorithm>
#include <regex>

#include "server_settings.h"
#include "exceptions_config_file.h"
#include "server_settings_exceptions.h"
#include "main_server_settings.h"

#define STATIC_FOLDER_PATH "static"

const std::vector<std::string> ServerSettings::valid_properties = {"listen", "root", "servername", "location"};

const std::vector<std::string> ServerSettings::valid_location_properties = {"root", "add_root"};

int ServerSettings::get_number_of_property(const std::string &property)
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

void ServerSettings::set_property(int number_of_property, std::string value)
{
    int begin = 0;
    while (isspace(value[begin]))
    {
        ++begin;
    }
    int value_length = (value[value.length() - 1] == ';') ? value.length() - begin - 1 : value.length() - begin;
    switch (number_of_property)
    {
    case LISTEN_NUMBER:
        try
        {
            this->port = stoi(value.substr(begin, value_length));
        }
        catch (std::exception &e)
        {
            throw InvalidConfigException("listen port can be only integer");
        }
        break;
    case ROOT_NUMBER:
        this->root = STATIC_FOLDER_PATH + value.substr(begin, value_length);
        this->is_root = true;
        break;
    case SERVERNAME_NUMBER:
        this->servername = value.substr(begin, value_length);
        break;
    }
}

int ServerSettings::get_number_of_location_property(const std::string &property)
{
    int begin = 0;
    while (isspace(property[begin]))
    {
        ++begin;
    }
    int property_length = (property[property.length() - 1] == ':') ? property.length() - begin - 1 : property.length() - begin;

    for (auto iter = this->valid_location_properties.begin();
         iter != this->valid_location_properties.end(); ++iter)
    {
        if (property.substr(begin, property_length) == *iter)
        {
            return iter - this->valid_location_properties.begin();
        }
    }

    return -1;
}

void ServerSettings::set_location_property(int number_of_property, const std::string &value, location_t &location)
{
    int begin = 0;
    while (isspace(value[begin]))
    {
        ++begin;
    }
    int value_length;
    value_length = (value[value.length() - 1] == ';') ? value.length() - begin - 1 : value.length() - begin;
    switch (number_of_property)
    {
    case ROOT_LOCATION_NUMBER:
        location.root = STATIC_FOLDER_PATH + value.substr(begin, value_length);
        break;
    case ADD_ROOT_NUMBER:
        location.root = this->root + value.substr(begin, value_length);
        break;
    }
}

location_t *ServerSettings::get_location(const std::string &url)
{
    std::string not_case_sensitive_url = url;
    std::transform(not_case_sensitive_url.begin(), not_case_sensitive_url.end(),
                   not_case_sensitive_url.begin(), [](unsigned char c) -> unsigned char
                   { return std::tolower(c); });

    for (auto &exact_match_url : this->exact_match_urls)
    {
        if (!exact_match_url.case_sensitive && not_case_sensitive_url == exact_match_url.url)
        {
            return &exact_match_url;
        }
        if (url == exact_match_url.url)
        {
            return &exact_match_url;
        }
    }

    for (auto &preferential_match_url : this->preferential_prefix_urls)
    {
        if (!preferential_match_url.case_sensitive && not_case_sensitive_url.find(preferential_match_url.url) == 0)
        {
            return &preferential_match_url;
        }
        if (url.find(preferential_match_url.url) == 0)
        {
            return &preferential_match_url;
        }
    }

    for (auto &regex_match_url : this->regex_match_urls)
    {
        std::regex regex(regex_match_url.url);
        if (!regex_match_url.case_sensitive && std::regex_match(not_case_sensitive_url.cbegin(), not_case_sensitive_url.cend(), regex))
        {
            return &regex_match_url;
        }
        if (std::regex_match(url.cbegin(), url.cend(), regex))
        {
            return &regex_match_url;
        }
    }

    for (auto &prefix_match_url : this->prefix_match_urls)
    {
        if (!prefix_match_url.case_sensitive && not_case_sensitive_url.find(prefix_match_url.url) == 0)
        {
            return &prefix_match_url;
        }
        if (url.find(prefix_match_url.url) == 0)
        {
            return &prefix_match_url;
        }
    }

    throw RootNotFoundException("404");
}

void ServerSettings::add_exact_match_url(location_t &location)
{
    this->exact_match_urls.push_back(location);
}

void ServerSettings::add_preferential_prefix_url(location_t &location)
{
    this->preferential_prefix_urls.push_back(location);
}

void ServerSettings::add_regex_match_url(location_t &location)
{
    this->regex_match_urls.push_back(location);
}

void ServerSettings::add_prefix_match_url(location_t &location)
{
    this->prefix_match_urls.push_back(location);
}

int ServerSettings::get_port()
{
    return this->port;
}

std::string ServerSettings::get_servername()
{
    return this->servername;
}
