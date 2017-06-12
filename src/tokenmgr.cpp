#include <fstream>
#include <iostream>

#include "customtypes.h"
#include "restclient.h"
#include "tokenmgr.h"

using namespace std;

tokenmgr::tokenmgr() {}

completion_t tokenmgr::get_from_file() {
    const string path = "/home/atejeda/.throwfile";
    ifstream file(path);

    completion_t res;

    if ((res.completion = (file.is_open() && file.good()))) {
        string token;
        getline(file, token);
        res.completion = !token.empty();
        res.body = token;
        file.close();
    }

    return res;
}

completion_t tokenmgr::set_to_file(const string token) {
    const string path = "/home/atejeda/.throwfile";
    ofstream file(path);

    completion_t res;

    if ((res.completion = (file.is_open() && file.good()))) {
        file << token;
        file.close();
    }

    return res;
}

completion_t tokenmgr::get_from_request(const string app_auth, const string app_key,
                                        const string app_secret) {
    restclient* rest_client = new restclient();

    string endpoint = "https://api.dropboxapi.com/oauth2/token";

    vector<string> headers;
    headers.push_back("Content-Type: application/x-www-form-urlencoded;");

    string postdata = "code=" + app_auth;
    postdata += string("&grant_type=authorization_code");
    postdata += "&client_id=" + app_key + "&client_secret=" + app_secret;

    completion_map_t resreq = rest_client->request(endpoint, headers, postdata);

    completion_t res;

    map<string, string> dict = resreq.body;
    bool istoken = dict.find("access_token") != dict.end();
    bool iserror = dict.find("error_description") != dict.end();

    if ((res.completion = resreq.completion)) {
        if ((res.completion = istoken)) {
            res.body = dict["access_token"];
        } else if (iserror) {
            res.completion = false;
            res.body = dict["error_description"];
        } else {
            res.body = "unknown error (1)";
        }
    } else {
        if (iserror) {
            res.body = dict["error_description"];
        } else {
            res.body = "unknown error (2)";
        }
    }

    return res;
}

tokenmgr::~tokenmgr() {}
