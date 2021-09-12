#pragma once

#include <exception>
#include <string>

class HttpBaseException : public std::exception
{
public:
    HttpBaseException(const std::string &msg) : msg(std::move(msg)) {}

    const char *what() const noexcept override
    {
        return msg.c_str();
    }

private:
    std::string msg;
};

class ReadException : public HttpBaseException
{
public:
    ReadException(const std::string &msg) : HttpBaseException(msg) {}
};

class DelimException : public HttpBaseException
{
public:
    DelimException(const std::string &msg) : HttpBaseException(msg) {}
};

class ProtVersionException : public HttpBaseException
{
public:
    ProtVersionException(const std::string &msg) : HttpBaseException(msg) {}
};

class MethodException : public HttpBaseException
{
public:
    MethodException(const std::string &msg) : HttpBaseException(msg) {}
};

class WrongFileTypeException : public HttpBaseException
{
public:
    WrongFileTypeException(const std::string &msg) : HttpBaseException(msg) {}
};
