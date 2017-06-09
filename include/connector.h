#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "customtypes.h"
#include "restclient.h"

class connector {
  public:
    // token
    static restclient *token_client;
    static void token_handler(connection_t *, int, void *);
};

#endif // CONNECTOR_H
