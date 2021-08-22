#include <cstring>
#include <sstream>

#include "http_exceptions.h"
#include "http_request.h"

std::string HttpRequest::get_method() const {
    return this->method;
}

std::string HttpRequest::get_url() const {
    return this->url;
}

void HttpRequest::add_line(const std::string& line) {
    if (!this->first_line_added) {
        add_first_line(line);
        return;
    }
    this->request_ended = true;
}

void HttpRequest::add_first_line(const std::string& line) {
    size_t lf_pos = line.find('\n');
    if (lf_pos == std::string::npos) {
        throw DelimException("Line feed not found");
    }

    size_t start_pos = 0;
    size_t end_pos = line.find(' ');
    if (end_pos == std::string::npos) {
        throw DelimException("Space not found");
    }
    this->method = std::string(line, start_pos, end_pos - start_pos);

    start_pos = end_pos + 1;
    while (line[start_pos] == ' ') {
        ++start_pos;
    }
    end_pos = line.find(' ', start_pos);
    if (end_pos == std::string::npos) {
        throw DelimException("Space not found");
    }
    this->url = std::string(line, start_pos, end_pos - start_pos);
    if (url == "/") {
        url = DEFAULT_URL;
    }

    start_pos = end_pos + 1;
    while (line[start_pos] == ' ') {
        ++start_pos;
    }
    std::string protocol(line, start_pos, lf_pos - 1 - start_pos);
    if (sscanf(protocol.c_str(), "HTTP/%d.%d", &version_major, &version_minor) != 2) {
        throw ReadException("Error while reading from file descriptor");
    }

    first_line_added = true;
}

bool HttpRequest::requst_ended() const {
    return this->request_ended;
}
