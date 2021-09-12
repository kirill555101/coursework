#include "server.h"

int main()
{
    Server server;
    if (!server.start())
    {
        return -1;
    }

    return 0;
}
