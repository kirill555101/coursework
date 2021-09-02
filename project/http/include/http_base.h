#pragma once

#include <string>
#include <unordered_map>

#include "http_defines.h"

class HttpBase {
public:
   HttpBase() = default;

   HttpBase(const std::unordered_map<std::string, std::string> &headers, int major = -1, int minor = -1);

   ~HttpBase() = default;

   int get_minor() const;
   int get_major() const;

   std::unordered_map<std::string, std::string> &get_headers();

protected:
   int version_major;
   int version_minor;
   std::unordered_map<std::string, std::string> headers;
};
