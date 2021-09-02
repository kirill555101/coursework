#pragma once

#include <unistd.h>
#include <vector>
#include <string>
#include <map>

#include "server_settings.h"
#include "client_connection.h"
#include "log.h"
#include "log_defines.h"

extern bool is_hard_stop;
extern bool is_soft_stop;
extern bool is_soft_reload;

class WorkerProcess {
public:
    WorkerProcess() = default;

    explicit WorkerProcess(int listen_sock, class ServerSettings *server_settings, std::vector<Log*>& vector_logs);

    void run();

    ~WorkerProcess() = default;

    void setup_sighandlers();

    static void sighup_handler(int sig); // Handler for soft stop

    static void sigint_handler(int sig); // Handler for hard stop

    void write_to_logs(std::string message, bl::trivial::severity_level lvl);

private:
    typedef enum {
        INFO_HARD_STOP_DONE,
        INFO_SOFT_STOP_START,
        INFO_SOFT_STOP_DONE
    } log_messages_t;

    class ServerSettings *server_settings;

    int listen_sock;

    void write_to_log(log_messages_t log_type);

    void message_to_log(log_messages_t log_type);

    std::vector<Log*> vector_logs;
};
