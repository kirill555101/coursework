#include "http_exceptions.h"
#include "http_response.h"

HttpResponse::HttpResponse(std::unordered_map<std::string, std::string> headers,
                           int major,
                           int minor,
                           int status,
                           const std::string& message)
        : HttpBase(headers, major, minor),
          status(status),
          message(message) {}

std::string HttpResponse::get_string() {
    std::string str = "HTTP/" + std::to_string(this->get_major()) + "." + std::to_string(this->get_minor()) + " " +
                      std::to_string(this->status) + " " + this->message + "\n";
    for (const auto &header: this->headers) {
        str += header.first + ": " + header.second + "\n";
    }
    str += "\n\n";
    return str;
}

int HttpResponse::get_status() const {
    return status;
}
