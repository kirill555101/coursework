#pragma once

#include <exception>
#include <string>

class ConfigFileBaseException : public std::exception {
public:
    ConfigFileBaseException(const std::string &msg) : msg(std::move(msg)) {}

    const char *what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

class NoSuchConfigFileException : public ConfigFileBaseException {
public:
    NoSuchConfigFileException(const std::string &msg) : ConfigFileBaseException(msg) {}
};

class InvalidConfigException : public ConfigFileBaseException {
public:
    InvalidConfigException(const std::string &msg) : ConfigFileBaseException(msg) {}
};
