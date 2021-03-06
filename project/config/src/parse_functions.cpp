#include "parse_functions.h"

location_type_t get_prefix_status(const std::string &config, int &pos)
{
    location_type_t type_location;
    if (config[pos] == '=')
    {
        type_location = EXACT_MATCH;
        ++pos;
    }
    else if (config[pos] == '^' && config[pos + 1] == '~')
    {
        type_location = PREFERENTIAL_PREFIX;
        pos += 2;
    }
    else if (config[pos] == '~')
    {
        type_location = REGEX_MATCH;
        ++pos;
    }
    else
    {
        type_location = PREFIX_MATCH;
    }

    return type_location;
}

bool get_url(const std::string &config, int &pos, location_t &location)
{
    if (config[pos] != '/')
    {
        return false;
    }
    int pos_before = pos;
    while (!isspace(config[pos]) && config[pos] != ' ')
    {
        ++pos;
    }

    location.url = config.substr(pos_before, pos - pos_before);
    return true;
}

int parse_location(ServerSettings &server, std::string &config, int &pos)
{
    location_t location;
    location_type_t type_location;

    if (server.is_root)
    {
        location.root = server.root;
    }
    else
    {
        location.root = "\n"; // For the subsequent check that each location block will correspond to some root
    }

    while (isspace(config[pos]))
    {
        ++pos;
    }

    if (config[pos] == '*')
    {
        location.case_sensitive = false;
        ++pos;
    }
    else
    {
        location.case_sensitive = true;
    }

    type_location = get_prefix_status(config, pos);

    while (isspace(config[pos]))
    {
        ++pos;
    }

    if (!get_url(config, pos, location))
    {
        return L_ERR;
    }
    while (isspace(config[pos]))
    {
        ++pos;
    }
    if (config[pos++] != '{')
    {
        return L_ERR;
    }

    state_t state = S_KEY;
    lexeme_t lexeme;
    int location_property_number;
    int pos_before;
    while (state != S_END && state != S_ERR)
    {
        pos_before = pos;
        lexeme = static_cast<lexeme_t>(get_lexeme(config, pos, server.valid_location_properties));
        if (lexeme == L_NEW_LINE && state == S_KEY)
        {
            continue;
        }
        else if (lexeme == L_KEY && state == S_KEY)
        {
            state = S_VALUE;
            location_property_number = server.get_number_of_location_property(
                config.substr(pos_before, pos - pos_before));
        }
        else if (lexeme == L_VALUE && state == S_VALUE)
        {
            try
            {
                server.set_location_property(location_property_number, config.substr(pos_before, pos - pos_before),
                                             location);
                state = S_KEY;
            }
            catch (std::exception &e)
            {
                std::cout << e.what();
                state = S_ERR;
            }
        }
        else if (state == S_KEY && lexeme == L_BRACE_CLOSE)
        {
            state = S_END;
        }
        else
        {
            state = S_ERR;
        }
    }

    if (location.root == "\n")
    { // If the location block is not set to root
        return L_ERR;
    }

    if (state == S_END)
    {
        switch (type_location)
        {
        case EXACT_MATCH:
            server.add_exact_match_url(location);
            break;
        case PREFERENTIAL_PREFIX:
            server.add_preferential_prefix_url(location);
            break;
        case REGEX_MATCH:
            server.add_regex_match_url(location);
            break;
        case PREFIX_MATCH:
            server.add_prefix_match_url(location);
            break;
        }
        return L_END_LOCATION;
    }
    else
    {
        return L_ERR;
    }
}

int get_lexeme(const std::string &config, int &pos, const std::vector<std::string> &valid_properties)
{
    while (isspace(config[pos]) && config[pos] != '\n')
    {
        ++pos;
    }

    if (config[pos] == '\n')
    {
        ++pos;
        return L_NEW_LINE;
    }

    if (config[pos] == '{')
    {
        ++pos;
        return L_BRACE_OPEN;
    }

    if (config[pos] == '}')
    {
        ++pos;
        return L_BRACE_CLOSE;
    }

    for (int i = pos; i < config.size() && config[i] != '\n'; ++i)
    {
        if (config[i] == ':')
        {
            break;
        }
        if (config[i] == ';')
        {
            pos = i + 1; // +1 to skip the semicolon
            return L_VALUE;
        }
    }

    if (config.substr(pos, sizeof("http") - 1) == "http")
    {
        pos += sizeof("http") - 1;
        return L_PROTOCOL;
    }

    for (auto iter = valid_properties.begin(); iter != valid_properties.end(); ++iter)
    {
        if (*iter == config.substr(pos, iter->size()))
        {
            pos += iter->size() + 1; // +1 to skip a colon or space
            if (*iter == "server")
            {
                return L_SERVER_START;
            }
            if (*iter == "location")
            {
                return L_LOCATION;
            }
            return L_KEY;
        }
    }

    return L_ERR;
}

std::string get_string_from_file(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw NoSuchConfigFileException("No such file " + filename);
    }

    size_t file_size = 0;
    file.seekg(0, std::ios::end);
    file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string str;
    str.resize(file_size);
    for (char &i : str)
    {
        file.get(i);
    }
    file.close();

    return str;
}

void parse_config(MainServerSettings &main_server_settings)
{
    std::string config_text;

    config_text = get_string_from_file(main_server_settings.config_filename);
    int pos = 0;

    state_t stages[S_COUNT][L_COUNT] = {
        /*                  L_PROTOCOL     L_BRACE_OPEN L_BRACE_CLOSE L_NEW_LINE L_KEY    L_VALUE L_SERVER_START  L_LOCATION  L_END_LOCATION L_SERVER_END*/
        /*S_START*/ {S_BRACE_OPEN, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR},
        /*S_BRACE_OPEN*/ {S_ERR, S_KEY, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR},
        /*S_KEY*/ {S_ERR, S_ERR, S_END, S_KEY, S_VALUE, S_ERR, S_SERVER_START, S_LOCATION, S_ERR, S_KEY},
        /*S_VALUE*/ {S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_KEY, S_ERR, S_ERR, S_ERR, S_ERR},
        /*S_SERVER_START*/ {S_ERR, S_KEY, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR},
        /*S_LOCATION*/ {S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_ERR, S_KEY, S_ERR},
    };

    state_t state = S_START;
    lexeme_t lexeme;
    int pos_before;
    int property_number;

    bool is_server_adding = false, has_server_added = false;

    while (state != S_END && state != S_ERR)
    {
        pos_before = pos;
        if (is_server_adding)
        {
            lexeme = static_cast<lexeme_t>(get_lexeme(config_text, pos, main_server_settings.server.valid_properties));
        }
        else
        {
            lexeme = static_cast<lexeme_t>(get_lexeme(config_text, pos, main_server_settings.valid_properties));
        }
        if (lexeme == L_ERR)
        {
            state = S_ERR;
            continue;
        }

        if (stages[state][lexeme] == S_ERR)
        {
            state = S_ERR;
            continue;
        }

        if (lexeme == L_KEY)
        {
            if (is_server_adding)
            {
                property_number = main_server_settings.server.get_number_of_property(
                    config_text.substr(pos_before, pos - pos_before));
                if (property_number == -1)
                {
                    state = S_ERR;
                    continue;
                }
            }
            else
            {
                property_number = main_server_settings.get_number_of_property(
                    config_text.substr(pos_before, pos - pos_before));
                if (property_number == -1)
                {
                    state = S_ERR;
                    continue;
                }
            }
        }
        else if (lexeme == L_VALUE)
        {
            if (is_server_adding)
            {
                try
                {
                    main_server_settings.server.set_property(property_number,
                                                             config_text.substr(pos_before, pos - pos_before));
                }
                catch (std::exception &e)
                {
                    state = S_ERR;
                    continue;
                }
            }
            else
            {
                try
                {
                    main_server_settings.set_property(property_number, config_text.substr(pos_before, pos - pos_before));
                }
                catch (std::exception &e)
                {
                    state = S_ERR;
                    continue;
                }
            }
        }
        else if (lexeme == L_SERVER_START && stages[state][lexeme] == S_SERVER_START)
        {
            if (is_server_adding || has_server_added)
            {
                state = S_ERR;
                continue;
            }
            is_server_adding = true;
            has_server_added = true;
        }
        else if (lexeme == L_BRACE_CLOSE && is_server_adding)
        {
            is_server_adding = false;
            lexeme = L_SERVER_END;
        }
        else if (lexeme == L_LOCATION)
        {
            if (!is_server_adding)
            {
                state = S_ERR;
                continue;
            }
            state = S_LOCATION;
            lexeme = static_cast<lexeme_t>(parse_location(main_server_settings.server, config_text, pos));
        }

        state = stages[state][lexeme];
    }
    if (state == S_ERR)
    {
        throw InvalidConfigException("Invalid config file, pos = " + std::to_string(pos));
    }
}
