#include <string>
#include <vector>

#include "customtypes.h"

#ifndef RESTCLIENT_H
#define RESTCLIENT_H

using namespace std;

// classes

class restclient {
private:
    connection_manager_t connection_manager;
    bool handler_flag;
    handler_response_t handler_response;
public:
    bool handler_flag2 = false;
    restclient();
    restclient(const connection_manager_t&);
    completion_t request(const mg_event_handler_t&, string, vector<string>, string);
    void handler(connection_t*, int, void*);
    ~restclient();
};

#endif // RESTCLIENT_H
