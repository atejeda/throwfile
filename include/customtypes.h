#ifndef CUSTOMTYPES_H
#define CUSTOMTYPES_H

#include <map>
#include <string>

#include "mongoose.h"

using namespace std;

#define _150MB (1000 * 1000 * 150)

// structs

struct handler_response {
    int code;
    string message;
    string response;
};

template <class T> struct completion {
    bool completion;
    T body;
};

struct throwfile_path {
    string local_path;
    string remote_path;
    string file_unit;
    long file_size;
    long remaining;
    int chunks;
};

// typedefs

typedef struct mg_mgr connection_manager_t;
typedef struct mg_connection connection_t;
typedef struct http_message http_message_t;
typedef struct handler_response handler_response_t;
typedef struct completion<string> completion_t;
typedef struct completion<map<string, string>> completion_map_t;
typedef struct throwfile_path throwfile_path_t;

#endif // CUSTOMTYPES_H
