#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <csignal>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "server.h"

#define BACKLOG 256

#define CONFIG_FILE_PATH "settings/guron.conf"

int process_soft_stop = 0;
int process_hard_stop = 0;
int process_soft_reload = EMPTY_MASTER;
int process_hard_reload = EMPTY_MASTER;
int has_old_master_stopped = 0;

pid_t new_master_pid = 0;

Server::Server() {
    this->settings = MainServerSettings(CONFIG_FILE_PATH);
    this->count_workflows = this->settings.get_count_workflows();
    this->server = this->settings.get_server();

    error_log = Log(this->server.get_error_log_filename(), true, cast_types_logs_level(error_log_level));
    access_log = Log(this->server.get_access_log_filename(), true, cast_types_logs_level(access_log_level));

    vector_logs.push_back(&error_log);
    vector_logs.push_back(&access_log);

    write_to_logs("SERVER STARTING...", INFO);
}

bl::trivial::severity_level Server::cast_types_logs_level(std::string lvl) {
    if (lvl == "info") {
        return INFO;
    }
    if (lvl == "debug") {
        return DEBUG;
    }
    if (lvl == "trace") {
        return TRACE;
    }
    return ERROR;
}

void Server::write_to_logs(std::string message, bl::trivial::severity_level lvl) {
    for (auto i : this->vector_logs) {
        i->log(message, lvl);
    }
}

int Server::daemonize(status_server_action server_action) {
    if (server_action == START_SERVER) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        pid_t pid = fork();

        if (pid == NOT_OK) {
            return NOT_OK;
        }
        if (pid != 0) {  // Возвращается нуль процессу-потомку и пид чилда мастеру
            exit(0);
        }

        setsid();
        old_master_process = getpid();
        return 0;
    }

    if (server_action == RELOAD_SERVER) {
        pid_t pid_child_of_old_master = fork();

        if (pid_child_of_old_master == NOT_OK) {
            return NOT_OK;
        }

        // check it isn't old master
        if (getpid() == old_master_process) {
            return 0;
        }

        pid_t pid_to_finish = getpid();

        pid_t new_master_pid = fork();

        if (new_master_pid == NOT_OK) {
            return NOT_OK;
        }
        if (getpid() == pid_to_finish) {
            exit(0);
        }

        new_master_process = getpid();
        setsid();

        return (int) new_master_process;
    }
    return 0;
}

int Server::fill_pid_file() {
    std::ofstream stream_to_pid_file;
    stream_to_pid_file.open("pid_file.txt", std::ios::out);

    if (!stream_to_pid_file.is_open()) {
        return NOT_OK;
    }

    stream_to_pid_file << getpid() << std::endl;

    for (auto &i : this->workers_pid) {
        stream_to_pid_file << i << std::endl;
    }

    stream_to_pid_file.close();

    return 0;
}

int Server::delete_pid_file() {
    return remove("pid_file.txt");
}

int Server::add_work_processes() {
    int count_work_processes = this->settings.get_count_workflows();

    if (count_work_processes <= 0) {
        write_to_logs("COUNT WORKFLOWS NEED BE MORE 0", ERROR);
        return NOT_OK;
    }

    workers_pid.clear();

    for (int i = 0; i < count_work_processes; ++i) {
        pid_t pid = fork();
        if (pid == NOT_OK) {
            write_to_logs("ERROR FORK", ERROR);
            return NOT_OK;
        }
        if (pid != 0) {
            workers_pid.push_back(pid);
        } else {
            WorkerProcess worker(this->listen_sock, &server, vector_logs);
            worker.run();
            break;
        }
    }

    return 0;
}

int Server::start() {
    if (daemonize(START_SERVER) != 0) {
        write_to_logs("ERROR IN SERVER DAEMONIZE", ERROR);
        return NOT_OK;
    }

    if (!this->bind_listen_sock()) {
        write_to_logs("ERROR IN BIND SOCKET", ERROR);
        return NOT_OK;
    }

    if (add_work_processes() != 0) {
        write_to_logs("ERROR IN ADDING WORK PROCESSES", ERROR);
        return NOT_OK;
    }

    if (fill_pid_file() == NOT_OK) {
        write_to_logs("ERROR IN FILL PID FILE", ERROR);
        return NOT_OK;
    }

    write_to_logs("Worker processes (" + std::to_string(this->workers_pid.size()) + ") successfully started", INFO);

    process_setup_signals();  // установка нужных обработчиков сигналов

    while (true) {
        if (process_soft_stop == 1) {
            server_stop(SOFT_LEVEL);
            return 0;
        }

        if (process_hard_stop == 1) {
            server_stop(HARD_LEVEL);
            return 0;
        }

        if (process_soft_reload == OLD_MASTER) {
            if (server_reload(SOFT_LEVEL) == NOT_OK) {
                write_to_logs("ERROR SOFT RELOAD", ERROR);
                server_stop(HARD_LEVEL);
                return NOT_OK;
            }
        }

        if (process_hard_reload == OLD_MASTER) {
            if (server_reload(HARD_LEVEL) == NOT_OK) {
                write_to_logs("ERROR HARD RELOAD", ERROR);
                server_stop(HARD_LEVEL);
                return NOT_OK;
            }
        }

        sleep(1);
    }

    return 0;
}

void Server::sighup_handler(int sig, siginfo_t* info, void* param) {
    process_soft_stop = 1;
    new_master_pid = info->si_pid;
}

void Server::sigint_handler(int sig, siginfo_t* info, void* param) {
    process_hard_stop = 1;
    new_master_pid = info->si_pid;
}

void Server::sigpipe_handler(int sig) {
    process_soft_reload = OLD_MASTER;
}

void Server::sigalrm_handler(int sig) {
    process_hard_reload = OLD_MASTER;
}

void Server::sigchld_handler(int sig) {
    has_old_master_stopped = 1;
}

int Server::process_setup_signals() {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    act.sa_handler = sigpipe_handler;
    sigaction(SIGPIPE, &act, nullptr);

    act.sa_flags = SA_SIGINFO;
    act.sa_handler = NULL;
    act.sa_sigaction = sighup_handler;
    sigaction(SIGHUP, &act, nullptr);

    act.sa_flags = SA_SIGINFO;
    act.sa_handler = NULL;
    act.sa_sigaction = sigint_handler;
    sigaction(SIGINT, &act, nullptr);

    act.sa_flags = 0;
    act.sa_sigaction = NULL;
    act.sa_handler = sigalrm_handler;
    sigaction(SIGALRM, &act, nullptr);

    act.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &act, nullptr);

    return 0;
}

int Server::server_stop(action_level_t level) {
    if (level == HARD_LEVEL) {
        write_to_logs("HARD SERVER STOP...", WARNING);
        if (process_hard_reload == OLD_MASTER) {
            close(this->listen_sock);
        }

        int status;
        for (auto &i : this->workers_pid) {
            kill(i, SIGINT);
        }
        for (auto &i : this->workers_pid) {
            waitpid(i, &status, 0);
        }

        if (process_hard_reload == NEW_MASTER) {
            write_to_logs("OLD MASTER STOPPED! PID " + std::to_string(getpid()), INFO);
            kill(new_master_pid, SIGCHLD);
        } else {
            write_to_logs("SERVER STOPPED!", INFO);
            delete_pid_file();
        }

        exit(0);
    }

    if (level == SOFT_LEVEL) {
        write_to_logs("SOFT SERVER STOP...", WARNING);

        int status;
        for (auto &i : this->workers_pid) {
            kill(i, SIGHUP);
        }
        for (auto &i : this->workers_pid) {
            waitpid(i, &status, 0);
        }

        if (process_soft_reload == NEW_MASTER) {
            write_to_logs("OLD MASTER STOPPED! PID " + std::to_string(getpid()), INFO);
            kill(new_master_pid, SIGCHLD);
        } else {
            close(this->listen_sock);
            delete_pid_file();
            write_to_logs("SERVER STOPPED!", INFO);
        }

        exit(0);
    }

    write_to_logs("ERROR! SERVER NOT STOPPED!", ERROR);

    return NOT_OK;
}

int Server::server_reload(action_level_t level) {
    this->settings = MainServerSettings(CONFIG_FILE_PATH);
    this->count_workflows = this->settings.get_count_workflows();
    this->server = this->settings.get_server();

    pid_t status_daemonize = this->daemonize(RELOAD_SERVER);

    if (status_daemonize == NOT_OK) {
        write_to_logs("ERROR DAEMONIZE", ERROR);
        return NOT_OK;
    }

    if (getpid() != this->old_master_process) {
        if (level == HARD_LEVEL) {
            write_to_logs("HARD SERVER RELOADING...New master process successfully started PID " +
                            std::to_string(getpid()), WARNING);
        } else if (level == SOFT_LEVEL) {
            write_to_logs("SOFT SERVER RELOADING...New master process successfully started PID " +
                            std::to_string(getpid()), WARNING);
        }
    } else {
        if (level == HARD_LEVEL) {
            process_hard_reload = NEW_MASTER;
        } else if (level == SOFT_LEVEL) {
            process_soft_reload = NEW_MASTER;
        }
        return 0;
    }

    if (apply_config(level) == NOT_OK) {
        write_to_logs("ERROR APPLY CONFIG", ERROR);
        return NOT_OK;
    }

    int status;
    if (level == HARD_LEVEL) {
        kill(old_master_process, SIGINT);
    } else if (level == SOFT_LEVEL){
        kill(old_master_process, SIGHUP);
    }
    write_to_logs("Send kill to old master process", WARNING);

    while (!has_old_master_stopped);
    has_old_master_stopped = 0;

    write_to_logs("OLD MASTER FINISHED ALL CONNECTIONS WITH STATUS: " +
                    std::to_string(WEXITSTATUS(status)) + " PID " + std::to_string(getpid()), INFO);

    fill_pid_file();

    old_master_process = new_master_process;
    new_master_process = 0;

    write_to_logs("SERVER RELOADED!", INFO);

    if (level == HARD_LEVEL) {
        process_hard_reload = EMPTY_MASTER;
    } else if (level == SOFT_LEVEL) {
        process_soft_reload = EMPTY_MASTER;
    }

    return 0;
}

int Server::apply_config(action_level_t level) {
    if (level == HARD_LEVEL) {
        write_to_logs("HARD SERVER RELOADING...NEW CONFIG APPLYING", WARNING);
    } else if (level == SOFT_LEVEL) {
        write_to_logs("SOFT SERVER RELOADING...NEW CONFIG APPLYING", WARNING);
    }

    count_workflows = this->settings.get_count_workflows();

    if (count_workflows <= 0) {
        write_to_logs("COUNT WORK PROCESSES MUST BE MORE 0", ERROR);
        return NOT_OK;
    }

    if (add_work_processes() == NOT_OK) {
        write_to_logs("ERROR ADD WORKERS", ERROR);
        return NOT_OK;
    }
    if (fill_pid_file() == NOT_OK) {
        write_to_logs("ERROR FILL PID FILE", ERROR);
        return NOT_OK;
    }

    write_to_logs("COUNT WORK PROCESSES WAS SUCCESSFULLY CHECKED", WARNING);
    write_to_logs("Worker processes (" + std::to_string(this->workers_pid.size()) + ") successfully started", INFO);

    return 0;
}

bool Server::bind_listen_sock() {
    this->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (this->listen_sock == NOT_OK) {
        return false;
    }

    int enable = 1;
    if (setsockopt(this->listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == NOT_OK) {
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(this->server.get_port());
    if (inet_aton(server.get_servername().c_str(), &addr.sin_addr) == NOT_OK) {
        return false;
    }

    if (bind(this->listen_sock, (struct sockaddr *) &addr, sizeof(addr)) == NOT_OK) {
        return false;
    }

    if (listen(this->listen_sock, BACKLOG) == NOT_OK) {
        return false;
    }

    return true;
}
