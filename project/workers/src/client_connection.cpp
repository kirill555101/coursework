#include "client_connection.h"

#define CLIENT_SEC_TIMEOUT 180
#define PAGE_404 "public/404.html"
#define LENGTH_LINE_FOR_RESERVE 256
#define FILE_BUFFER_LENGTH 4096

ClientConnection::ClientConnection(class ServerSettings *server_settings,
                                   std::vector<Log *> &vector_logs) : server_settings(server_settings),
                                                                      vector_logs(vector_logs),
                                                                      start_connection(clock())
{
}

ClientConnection::~ClientConnection()
{
    close(this->file_fd);
}

void ClientConnection::set_socket(int socket)
{
    this->sock = socket;
}

connection_status_t ClientConnection::connection_processing()
{
    if (this->stage == ERROR_STAGE)
    {
        return ERROR_WHILE_CONNECTION_PROCESSING;
    }

    if (this->stage == GET_REQUEST)
    {
        bool is_succeeded;
        try
        {
            is_succeeded = get_request();
        }
        catch (std::exception &e)
        {
            this->stage = BAD_REQUEST;
            this->message_to_log(ERROR_BAD_REQUEST);
            return ERROR_IN_REQUEST;
        }
        if (is_succeeded)
        {
            this->stage = this->process_location();
        }
        else if (this->is_timeout())
        {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (stage == ROOT_FOUND || stage == ROOT_NOT_FOUND)
    {
        try
        {
            this->make_response_header();
        }
        catch (std::exception &e)
        {
            this->stage = BAD_REQUEST;
            this->message_to_log(ERROR_BAD_REQUEST);
            return ERROR_IN_REQUEST;
        }
        this->stage = SEND_HTTP_HEADER_RESPONSE;
    }

    if (this->stage == SEND_HTTP_HEADER_RESPONSE)
    {
        if (this->send_header())
        {
            if (request.get_url() == HEAD_METHOD)
            {
                this->message_to_log(INFO_CONNECTION_FINISHED);
                return CONNECTION_FINISHED;
            }
            this->stage = SEND_FILE;
        }
        else if (this->is_timeout())
        {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (this->stage == SEND_FILE)
    {
        if (this->send_file())
        {
            this->message_to_log(INFO_CONNECTION_FINISHED);
            return CONNECTION_FINISHED;
        }
        else if (this->is_timeout())
        {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    return CONNECTION_PROCESSING;
}

bool ClientConnection::get_request()
{
    bool has_read_data = false;
    char last_char;
    std::string line;

    line.reserve(LENGTH_LINE_FOR_RESERVE);
    while (read(this->sock, &last_char, sizeof(char)) == sizeof(char))
    {
        line.push_back(last_char);
        if (last_char == '\n' && line.length() != 1)
        {
            this->request.add_line(line);
            line.clear();
            line.reserve(LENGTH_LINE_FOR_RESERVE);
        }
        has_read_data = true;
    }

    if (this->request.requst_ended())
    {
        return true;
    }

    if (has_read_data)
    {
        this->timeout = clock();
    }

    return false;
}

bool ClientConnection::make_response_header()
{
    if (this->stage == ROOT_FOUND)
    {
        this->response = http_handle(this->request, this->location->root);
        this->file_fd = open((this->location->root + this->request.get_url()).c_str(), O_RDONLY);
        if (this->file_fd == -1)
        {
            this->stage = ROOT_NOT_FOUND;
            this->file_fd = open(PAGE_404, O_RDONLY);
        }
    }
    else if (this->stage == ROOT_NOT_FOUND)
    {
        this->response = http_handle(this->request);
        this->file_fd = open(PAGE_404, O_RDONLY);
    }

    return true;
}

bool ClientConnection::send_header()
{
    bool has_written_data = false;
    std::string response_str = this->response.get_string();

    while (write(this->sock, response_str.c_str() + res_pos, sizeof(char)) == sizeof(char))
    {
        ++res_pos;
        if (res_pos == response_str.size() - 1)
        {
            return true;
        }
        has_written_data = true;
    }

    if (has_written_data)
    {
        this->timeout = clock();
    }

    return false;
}

bool ClientConnection::send_file()
{
    bool has_written_data = false;
    char buffer[FILE_BUFFER_LENGTH];
    int read_code = 0, write_result = 0;

    read_code = read(this->file_fd, &buffer, FILE_BUFFER_LENGTH);
    while (read_code > 0 && write(this->sock, &buffer, read_code) > 0)
    {
        read_code = read(this->file_fd, &buffer, FILE_BUFFER_LENGTH);
        has_written_data = true;
    }

    if (read_code == 0)
    {
        return true;
    }

    if (has_written_data)
    {
        this->timeout = clock();
    }

    return false;
}

void ClientConnection::message_to_log(log_messages_t log_type)
{
    switch (log_type)
    {
    case INFO_CONNECTION_FINISHED:
    {
        int status = this->response.get_status();
        if (status % 100 == 4)
        {
            this->write_to_logs("Connection [" + this->request.get_method() + "] [URL " + this->request.get_url() + "] [STATUS " + std::to_string(status) +
                                    "] [WRK PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " +
                                    std::to_string(this->sock) + "] [TIME " +
                                    std::to_string((clock() - start_connection) / (double)CLOCKS_PER_SEC * 1000) + " ms]",
                                ERROR);
        }
        else
        {
            this->write_to_logs("Connection [" + this->request.get_method() + "] [URL " + this->request.get_url() + "] [STATUS " + std::to_string(status) +
                                    "] [WRK PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " +
                                    std::to_string(this->sock) + "] [TIME " +
                                    std::to_string((clock() - start_connection) / (double)CLOCKS_PER_SEC * 1000) + " ms]",
                                INFO);
        }
        break;
    }
    case ERROR_READING_REQUEST:
        this->write_to_logs("Reading request error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " + std::to_string(this->sock) + "]", ERROR);
        break;
    case ERROR_SEND_HEADER:
        this->write_to_logs("Send header error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " + std::to_string(this->sock) + "]", ERROR);
        break;
    case ERROR_SEND_FILE:
        this->write_to_logs("Send file error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " + std::to_string(this->sock) + "]", ERROR);
        break;
    case ERROR_BAD_REQUEST:
        this->write_to_logs("Bad request error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " + std::to_string(this->sock) + "]", ERROR);
        break;
    case ERROR_TIMEOUT:
        this->write_to_logs("Timeout error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " + std::to_string(this->sock) + "]", ERROR);
        break;
    }
}

ClientConnection::connection_stages_t ClientConnection::process_location()
{
    std::string url = request.get_url();
    HttpResponse http_response;
    try
    {
        this->location = this->server_settings->get_location(url);
    }
    catch (std::exception &e)
    {
        return ROOT_NOT_FOUND;
    }
    return ROOT_FOUND;
}

void ClientConnection::write_to_logs(std::string message, bl::trivial::severity_level lvl)
{
    for (auto &vector_log : this->vector_logs)
    {
        vector_log->log(message, lvl);
    }
}

int ClientConnection::get_socket()
{
    return this->sock;
}

bool ClientConnection::is_timeout()
{
    return (clock() - this->timeout) / (double)CLOCKS_PER_SEC > CLIENT_SEC_TIMEOUT;
}
