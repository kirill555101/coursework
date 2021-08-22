#pragma once

#include <string>
#include <ctime>

#include "http_request.h"
#include "http_response.h"
#include "server_settings.h"
#include "log.h"
#include "log_defines.h"

typedef enum {
    CONNECTION_PROCESSING,
    CONNECTION_TIMEOUT_ERROR,
    CONNECTION_FINISHED,
    ERROR_WHILE_CONNECTION_PROCESSING,
    ERROR_IN_REQUEST,
    CHECKOUT_CLIENT_FOR_READ,
    CHECKOUT_CLIENT_FOR_WRITE
} connection_status_t;

class ClientConnection {
public:
    ClientConnection() = default;

    ClientConnection(class ServerSettings *server_settings, std::vector<Log*>& vector_logs);

    ~ClientConnection();

    connection_status_t connection_processing();

    int get_socket();

    void write_to_logs(std::string message, bl::trivial::severity_level lvl);

    void set_socket(int socket);

private:
    int sock;

    clock_t timeout, start_connection;

    std::vector<Log*> vector_logs;

    typedef enum {
        GET_REQUEST,
        ROOT_FOUND,
        ROOT_NOT_FOUND,
        SEND_HTTP_HEADER_RESPONSE,
        SEND_FILE,
        BAD_REQUEST,
        ERROR_STAGE
    } connection_stages_t;

    typedef enum {
        INFO_CONNECTION_FINISHED,
        ERROR_READING_REQUEST,
        ERROR_SEND_HEADER,
        ERROR_SEND_FILE,
        ERROR_BAD_REQUEST,
        ERROR_TIMEOUT
    } log_messages_t;

    connection_stages_t stage = GET_REQUEST;

    class ServerSettings *server_settings;

    HttpRequest request;
    HttpResponse response;

    int res_pos = 0;

    location_t *location = nullptr;

    int file_fd;

    connection_stages_t process_location();

    // return true if their connection processing stage is finished correctly
    bool get_request();

    bool make_response_header();

    bool send_header();

    bool send_file();

    void message_to_log(log_messages_t log_type);

    bool is_timeout();
};
