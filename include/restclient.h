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
    restclient();
    restclient(const connection_manager_t&);
    completion_t request(const mg_event_handler_t&, const string,
                         const vector<string>, const string);
    void handler(connection_t*, int, void*);
    ~restclient();
};

#endif // RESTCLIENT_H
