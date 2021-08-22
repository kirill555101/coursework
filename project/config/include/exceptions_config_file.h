#pragma once

#include <exception>
#include <string>

class BaseConfigFileException : public std::exception {
public:
    BaseConfigFileException(const std::string &msg) : msg(std::move(msg)) {}

    const char *what() const noexcept override {
        return msg.c_str();
    }

protected:
    std::string msg;
};

class NoSuchConfigFileException : public BaseConfigFileException {
public:
    NoSuchConfigFileException(const std::string &msg) : BaseConfigFileException(msg) {}
};

class InvalidConfigException : public BaseConfigFileException {
public:
    InvalidConfigException(const std::string &msg) : BaseConfigFileException(msg) {}
};
