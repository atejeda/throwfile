#include "customtypes.h"
#include "connector.h"

// token
restclient* connector::token_client;
void connector::token_handler(connection_t* nc, int ev, void* ev_data) {
    restclient* client = connector::token_client;
    client->handler(nc, ev, ev_data);
}
