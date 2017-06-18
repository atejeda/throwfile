#include <iostream>

#include "uploadmgr.h"

#include "customtypes.h"
#include "restclient.h"
#include "utils.h"

using namespace std;

uploadmgr::uploadmgr() {}

// TODO:
// - return completion
// - iterate over the file chunks
// - use a session upload when is needed
void uploadmgr::upload_file(restclient* rest_client, const string token, const throwfile_path_t& throw_file) {
    const string endpoint = "https://content.dropboxapi.com/2/files/upload";

    vector<const char*>* file_chunks;
    long remaining = utils::split_file(throw_file.local_path, throw_file.file_size, _150MB, &file_chunks);

    vector<string> headers;
    headers.push_back("Authorization: Bearer " + token);
    headers.push_back("Content-Type: application/octet-stream");
    headers.push_back("Dropbox-API-Arg: {\"path\":\"" + throw_file.remote_path + "\", \"mode\":\"overwrite\"}");

    //{"name": "acsStartContainer_javaContainer_2017-06-03_21.53.08.711", "path_lower":
    //"/cross/tmp3/acsstartcontainer_javacontainer_2017-06-03_21.53.08.711", "path_display":
    //"/cross/tmp3/acsStartContainer_javaContainer_2017-06-03_21.53.08.711", "id": "id:DstbCk1UZpAAAAAAAABrmg",
    //"client_modified": "2017-06-18T08:07:23Z", "server_modified": "2017-06-18T08:07:24Z", "rev": "559550280a6a7",
    //"size": 1552301, "content_hash": "2b4bd110227f10643095ee29893f72b9022324d0e87e90f22ccc0bceab6672a9"}

    completion_map_t res = rest_client->request(endpoint, headers, file_chunks->at(0));
}

uploadmgr::~uploadmgr() {}
