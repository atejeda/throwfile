#ifndef TOKENMGR_H
#define TOKENMGR_H

#include "customtypes.h"
#include "restclient.h"

class tokenmgr {
  private:
    //
  public:
    tokenmgr();
    completion_t get_from_file();
    completion_t set_to_file(const string);
    completion_t get_from_request(const string, const string, const string);
    ~tokenmgr();
};

#endif // TOKENMGR_H
