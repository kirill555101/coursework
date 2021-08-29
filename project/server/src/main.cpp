#include "server.h"

int main() {
    Server server;
    if (server.start() != 0) {
        return -1;
    }

    return 0;
}
