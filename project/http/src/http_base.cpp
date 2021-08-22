#include <unistd.h>

#include "http_base.h"

HttpBase::HttpBase(const std::unordered_map<std::string, std::string> &headers, int major, int minor)
        : headers(headers), version_major(major), version_minor(minor) {}

int HttpBase::get_minor() const {
    return version_minor;
}

int HttpBase::get_major() const {
    return version_major;
}

std::unordered_map<std::string, std::string> &HttpBase::get_headers() {
    return headers;
}
