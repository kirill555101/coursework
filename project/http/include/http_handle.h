#pragma once

#include "http_request.h"
#include "http_response.h"

HttpResponse http_handle(const HttpRequest &request, const std::string &root = NO_ROOT);
