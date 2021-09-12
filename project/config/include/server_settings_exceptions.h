#pragma once

#include <exception>
#include <string>

class ServerSettingsBaseException : public std::exception
{
public:
    ServerSettingsBaseException(const std::string &msg) : msg(std::move(msg)) {}

    const char *what() const noexcept override
    {
        return msg.c_str();
    }

private:
    std::string msg;
};

class RootNotFoundException : public ServerSettingsBaseException
{
public:
    RootNotFoundException(const std::string &msg) : ServerSettingsBaseException(msg) {}
};
