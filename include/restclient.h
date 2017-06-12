#include <string>
#include <vector>

#include "customtypes.h"
#include "mongoose.h"

#ifndef RESTCLIENT_H
#define RESTCLIENT_H

using namespace std;

class restclient {
  private:
    connection_manager_t connection_manager;
    bool handler_flag;
    handler_response_t handler_res;

  public:
    static restclient* static_client;

    restclient();
    restclient(const connection_manager_t&);
    completion_map_t request(const string, const vector<string>, const string);
    void handler(connection_t*, int, void*);
    ~restclient();
    // static
    static void static_handler(connection_t*, int, void*);
};

#endif // RESTCLIENT_H
