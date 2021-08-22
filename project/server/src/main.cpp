#include "server.h"

int main() {
    Server server;
    if (server.start() != 0) {
        return -1;
    }

    /*
    MainServerSettings server("settings/guron.conf");
    server.get_server().print_properties();
    */

    return 0;
}
