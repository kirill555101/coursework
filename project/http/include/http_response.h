#pragma once

#include "http_base.h"

class HttpResponse : public HttpBase {
public:
   HttpResponse() = default;

   HttpResponse(std::unordered_map<std::string, std::string> headers,
               int major,
               int minor,
               int status,
               const std::string &message);

   ~HttpResponse() = default;

   std::string get_string();

   int get_status() const;

private:
   int status = 0;

   std::string message;

   bool first_line_added = false, response_ended = false;
};
