#ifndef UPLOADMGR_H
#define UPLOADMGR_H

#include "customtypes.h"
#include "restclient.h"

class uploadmgr {
  private:
    //
  public:
    uploadmgr();
    void upload_file(restclient*, const string, const throwfile_path_t&);
    ~uploadmgr();
};

#endif // UPLOADMGR_H
