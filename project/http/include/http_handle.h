#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "http_request.h"
#include "http_response.h"
#include "http_date.h"
#include "http_file_types.h"

HttpResponse http_handle(const HttpRequest &request, const std::string &root = NO_ROOT);
